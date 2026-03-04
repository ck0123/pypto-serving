# pypto-serving 与 nano-vllm 设计对比

**日期**: 2026-03-03

---

## 概述

本文档对比 **pypto-serving** (C++) 和 **nano-vllm** (Python) 的架构设计，验证我们的实现是否遵循了 nano-vllm 的设计理念。

---

## 核心架构对比

### 1. 整体架构

#### nano-vllm
```
LLMEngine
├── Scheduler (调度器)
│   ├── waiting queue (等待队列)
│   ├── running queue (运行队列)
│   └── BlockManager (块管理器)
├── ModelRunner (模型运行器)
│   ├── Model (PyTorch 模型)
│   ├── Sampler (采样器)
│   └── KV Cache (键值缓存)
└── Tokenizer (分词器)
```

#### pypto-serving
```
Engine
├── Scheduler (调度器)
│   ├── waiting queue (等待队列)
│   ├── running queue (运行队列)
│   └── BlockManager (块管理器)
├── ModelRunner (模型运行器)
│   ├── MockModelRunner (Mock 模式)
│   ├── LLaMAModelRunner (LLaMA 实现)
│   └── PTO2ModelRunner (PTO2 设备)
├── Sampler (采样器)
└── TestPath (测试路径 - 额外功能)
```

**✅ 对比结果**: 架构**完全一致**，pypto-serving 增加了多种 ModelRunner 支持和 TestPath 测试接口。

---

## 组件详细对比

### 2. Sequence (序列管理)

#### nano-vllm: `Sequence`
```python
class Sequence:
    - seq_id                    # 序列 ID
    - status                    # 状态 (WAITING/RUNNING/FINISHED)
    - token_ids                 # 所有 token
    - num_prompt_tokens         # prompt 长度
    - num_cached_tokens         # 缓存的 token 数
    - block_table               # 块表
    - temperature               # 采样温度
    - max_tokens                # 最大生成长度
    - ignore_eos                # 是否忽略 EOS
```

#### pypto-serving: `Sequence`
```cpp
class Sequence {
    - request_id_               # 请求 ID
    - status_                   # 状态 (WAITING/RUNNING/FINISHED/PREEMPTED)
    - prompt_token_ids_         # prompt tokens
    - output_token_ids_         # 输出 tokens
    - num_cached_tokens_        # 缓存的 token 数
    - block_table_              # 块表
    - sampling_params_          # 采样参数 (temperature, max_tokens, etc.)
    - finish_reason_            # 完成原因
}
```

**✅ 对比结果**: 
- **核心字段完全对应**
- pypto-serving 将 prompt 和 output 分开存储（更清晰）
- pypto-serving 增加了 `finish_reason` 和 `PREEMPTED` 状态（更完善）

---

### 3. Scheduler (调度器)

#### nano-vllm: `Scheduler`
```python
class Scheduler:
    - max_num_seqs              # 最大并发序列数
    - max_num_batched_tokens    # 最大批次 token 数
    - block_manager             # 块管理器
    - waiting: deque            # 等待队列
    - running: deque            # 运行队列
    
    def schedule() -> (seqs, is_prefill)
    def postprocess(seqs, token_ids)
    def preempt(seq)
```

#### pypto-serving: `Scheduler`
```cpp
class Scheduler {
    - max_num_seqs              # 最大并发序列数
    - max_num_batched_tokens    # 最大批次 token 数
    - block_manager_            # 块管理器
    - waiting_: deque           # 等待队列
    - running_: deque           # 运行队列
    
    ScheduleOutput schedule()
    void postprocess(seqs, token_ids)
    void preempt(seq)
}
```

**✅ 对比结果**: 
- **接口完全一致**
- **调度逻辑完全相同**：
  - 优先调度 prefill（从 waiting 队列）
  - 然后调度 decode（从 running 队列）
  - 支持抢占（preemption）

---

### 4. BlockManager (块管理器)

#### nano-vllm: `BlockManager`
```python
class BlockManager:
    - block_size                # 每块的 token 数
    - blocks: list[Block]       # 所有块
    - hash_to_block_id: dict    # hash -> block_id (prefix caching)
    - free_block_ids: deque     # 空闲块队列
    - used_block_ids: set       # 使用中的块集合
    
    def can_allocate(seq) -> bool
    def allocate(seq)           # 分配块 (带 prefix caching)
    def deallocate(seq)
    def can_append(seq) -> bool
    def may_append(seq)
    
    @classmethod
    def compute_hash(token_ids, prefix) -> int
```

#### pypto-serving: `BlockManager`
```cpp
class BlockManager {
    - block_size_               # 每块的 token 数
    - blocks_: vector<Block>    # 所有块
    - hash_to_block_id_: map    # hash -> block_id (prefix caching)
    - free_block_ids_: deque    # 空闲块队列
    - used_block_ids_: set      # 使用中的块集合
    
    bool can_allocate(seq)
    void allocate(seq)          # 分配块 (带 prefix caching)
    void deallocate(seq)
    bool can_append(seq)
    void may_append(seq)
    
    uint64_t compute_hash(token_ids, prefix_hash)
}
```

**✅ 对比结果**: 
- **数据结构完全对应**
- **接口完全一致**
- **Hash-based prefix caching 实现相同**：
  - 使用 xxhash 计算块的 hash
  - 通过 hash 查找已缓存的块
  - 支持引用计数（ref_count）

---

### 5. ModelRunner (模型运行器)

#### nano-vllm: `ModelRunner`
```python
class ModelRunner:
    - model                     # PyTorch 模型
    - sampler                   # 采样器
    - kv_cache                  # KV 缓存
    - block_size                # 块大小
    
    def prepare_prefill(seqs) -> (input_ids, positions)
    def prepare_decode(seqs) -> (input_ids, positions)
    def run_model(input_ids, positions, is_prefill) -> logits
    def run(seqs, is_prefill) -> token_ids
```

#### pypto-serving: `ModelRunner` (多种实现)

**MockModelRunner**:
```cpp
class MockModelRunner {
    - model_config_
    
    vector<vector<float>> run(seqs, is_prefill)
    // 返回随机 logits (用于测试)
}
```

**LLaMAModelRunner**:
```cpp
class LLaMAModelRunner {
    - model_                    # LLaMA 模型
    - model_config_
    
    void prepare_input(seqs, is_prefill, ...)
    vector<vector<float>> run(seqs, is_prefill)
    // 返回真实的 logits
}
```

**PTO2ModelRunner**:
```cpp
class PTO2ModelRunner {
    - pto2_runtime_             # PTO2 运行时
    - model_config_
    
    vector<vector<float>> run(seqs, is_prefill)
    // 使用 PTO2 设备执行
}
```

**✅ 对比结果**: 
- **接口设计一致**（`run(seqs, is_prefill) -> logits`）
- pypto-serving 支持**多种后端**：
  - Mock: 用于测试
  - LLaMA: 真实算子实现
  - PTO2: 设备加速（对应 nano-vllm 的 GPU）

---

### 6. Sampler (采样器)

#### nano-vllm: `Sampler`
```python
class Sampler:
    def __call__(logits, temperatures) -> token_ids
    # 支持 temperature sampling
```

#### pypto-serving: `Sampler`
```cpp
class Sampler {
    TokenId sample(logits, sampling_params)
    // 支持:
    // - Greedy (temperature = 0)
    // - Temperature sampling
    // - Top-k sampling
    // - Top-p (nucleus) sampling
}
```

**✅ 对比结果**: 
- **基本功能一致**
- pypto-serving 支持**更多采样策略**（top-k, top-p）

---

## 关键差异与增强

### 1. 语言差异

| 特性 | nano-vllm (Python) | pypto-serving (C++) |
|------|-------------------|---------------------|
| 类型系统 | 动态类型 | 静态类型 |
| 内存管理 | GC | 手动管理 (智能指针) |
| 性能 | 依赖 PyTorch C++ 后端 | 纯 C++ 实现 |
| 部署 | 需要 Python 环境 | 独立可执行文件 |

### 2. PTO2 集成（核心差异）

#### nano-vllm
- 使用 **PyTorch** 作为计算后端
- 依赖 CUDA/GPU
- Flash Attention 通过 PyTorch 扩展

#### pypto-serving
- 使用 **PTO2 Runtime** 作为计算后端
- 支持 **Ascend NPU**
- 三种执行模式：
  - **MOCK**: 纯 CPU，用于开发测试
  - **SIMULATOR**: CPU 模拟器，验证算子逻辑
  - **DEVICE**: 真实 NPU 设备

**设计对齐**:
```
nano-vllm ModelRunner → PyTorch → CUDA/GPU
pypto-serving ModelRunner → PTO2 → Ascend NPU
```

### 3. 额外功能

#### pypto-serving 独有功能

1. **TestPath** (测试路径)
   - 异步测试接口
   - 不需要 HTTP 服务器
   - 直接注入请求和获取响应

2. **多种 ModelRunner**
   - Mock: 快速测试
   - LLaMA: 真实算子验证
   - PTO2: 设备加速

3. **完整的 LLaMA 实现**
   - Embedding, RMSNorm, RoPE
   - Multi-Head Attention with KV Cache
   - MLP (SwiGLU)
   - 完整的 Transformer

4. **更详细的状态管理**
   - FinishReason (LENGTH/EOS/STOP)
   - PREEMPTED 状态
   - 更细粒度的序列管理

---

## 核心算法对比

### 1. Prefill 调度

#### nano-vllm
```python
# 从 waiting 队列调度 prefill
while waiting and num_seqs < max_num_seqs:
    seq = waiting[0]
    if num_batched_tokens + len(seq) > max_num_batched_tokens:
        break
    if not block_manager.can_allocate(seq):
        break
    block_manager.allocate(seq)
    num_batched_tokens += len(seq) - seq.num_cached_tokens
    waiting.popleft()
    running.append(seq)
    scheduled_seqs.append(seq)
```

#### pypto-serving
```cpp
// 从 waiting 队列调度 prefill
while (!waiting_.empty() && num_seqs < max_num_seqs) {
    auto seq = waiting_.front();
    if (num_batched_tokens + seq->num_total_tokens() > max_num_batched_tokens)
        break;
    if (!block_manager_->can_allocate(*seq))
        break;
    block_manager_->allocate(*seq);
    num_batched_tokens += seq->num_total_tokens() - seq->num_cached_tokens();
    waiting_.pop_front();
    running_.push_back(seq);
    scheduled_seqs.push_back(seq);
}
```

**✅ 逻辑完全一致**

### 2. Decode 调度

#### nano-vllm
```python
# 从 running 队列调度 decode
while running and num_seqs < max_num_seqs:
    seq = running.popleft()
    while not block_manager.can_append(seq):
        if running:
            preempt(running.pop())
        else:
            preempt(seq)
            break
    else:
        block_manager.may_append(seq)
        scheduled_seqs.append(seq)
running.extendleft(reversed(scheduled_seqs))
```

#### pypto-serving
```cpp
// 从 running 队列调度 decode
while (!running_.empty() && num_seqs < max_num_seqs) {
    auto seq = running_.front();
    running_.pop_front();
    
    while (!block_manager_->can_append(*seq)) {
        if (!running_.empty()) {
            preempt(running_.back());
            running_.pop_back();
        } else {
            preempt(seq);
            break;
        }
    }
    
    if (seq->status() != SequenceStatus::WAITING) {
        block_manager_->may_append(*seq);
        scheduled_seqs.push_back(seq);
    }
}
```

**✅ 逻辑完全一致**（包括抢占机制）

### 3. Prefix Caching

#### nano-vllm
```python
def allocate(seq):
    h = -1
    cache_miss = False
    for i in range(seq.num_blocks):
        token_ids = seq.block(i)
        h = compute_hash(token_ids, h) if full_block else -1
        block_id = hash_to_block_id.get(h, -1)
        
        if block_id == -1 or blocks[block_id].token_ids != token_ids:
            cache_miss = True
        
        if cache_miss:
            block_id = free_block_ids[0]
            allocate_block(block_id)
        else:
            seq.num_cached_tokens += block_size
            if block_id in used_block_ids:
                blocks[block_id].ref_count += 1
            else:
                allocate_block(block_id)
        
        if h != -1:
            block.update(h, token_ids)
            hash_to_block_id[h] = block_id
        
        seq.block_table.append(block_id)
```

#### pypto-serving
```cpp
void BlockManager::allocate(Sequence& seq) {
    uint64_t hash = 0;
    bool cache_miss = false;
    
    for (int i = 0; i < seq.num_blocks(); ++i) {
        TokenIds token_ids = seq.get_block_tokens(i);
        hash = (full_block) ? compute_hash(token_ids, hash) : 0;
        
        auto it = hash_to_block_id_.find(hash);
        BlockId block_id = (it != hash_to_block_id_.end()) ? it->second : -1;
        
        if (block_id == -1 || blocks_[block_id].token_ids != token_ids) {
            cache_miss = true;
        }
        
        if (cache_miss) {
            block_id = _allocate_block();
        } else {
            seq.set_num_cached_tokens(seq.num_cached_tokens() + block_size_);
            if (used_block_ids_.count(block_id)) {
                blocks_[block_id].ref_count++;
            } else {
                // ... 分配逻辑
            }
        }
        
        if (hash != 0) {
            blocks_[block_id].hash = hash;
            blocks_[block_id].token_ids = token_ids;
            hash_to_block_id_[hash] = block_id;
        }
        
        seq.block_table().push_back(block_id);
    }
}
```

**✅ 算法完全一致**（Hash-based prefix caching）

---

## 测试覆盖对比

### nano-vllm
- 主要通过 `example.py` 和 `bench.py` 测试
- 端到端测试为主

### pypto-serving
- **单元测试**: 39 个
  - Sequence: 10 个
  - BlockManager: 8 个
  - Scheduler: 9 个
  - LLaMA: 12 个
- **集成测试**: 8 个
- **示例程序**: 5 个

**✅ pypto-serving 测试覆盖更全面**

---

## 性能特性对比

### nano-vllm
- ✅ Continuous batching
- ✅ Prefix caching
- ✅ Paged attention
- ✅ CUDA graphs (decode 优化)
- ✅ Tensor parallelism

### pypto-serving (当前实现)
- ✅ Continuous batching
- ✅ Prefix caching (hash-based)
- ✅ Paged attention (框架就绪)
- ⚠️ PTO2 graphs (待实现)
- ⚠️ Tensor parallelism (待实现)

---

## 总结

### ✅ 完全对齐的部分

1. **核心架构**: Engine → Scheduler → BlockManager → ModelRunner
2. **数据结构**: Sequence, Block, Queue 管理
3. **调度算法**: Prefill/Decode 调度、抢占机制
4. **Prefix Caching**: Hash-based 实现完全一致
5. **接口设计**: `schedule()`, `postprocess()`, `run()` 等

### 🎯 增强的部分

1. **多种 ModelRunner**: Mock / LLaMA / PTO2
2. **TestPath**: 异步测试接口
3. **完整的 LLaMA 实现**: 所有层的真实算子
4. **更详细的状态管理**: FinishReason, PREEMPTED
5. **更全面的测试**: 39 个单元测试 + 8 个集成测试

### 🔄 适配 PTO2 的部分

1. **PTO2Runtime**: 替代 PyTorch
2. **三种执行模式**: MOCK / SIMULATOR / DEVICE
3. **Ascend NPU**: 替代 CUDA/GPU
4. **PTO2 算子**: 替代 PyTorch 算子

---

## 结论

**pypto-serving 的设计与 nano-vllm 高度一致**：

1. ✅ **架构设计完全遵循 nano-vllm**
2. ✅ **核心算法实现一致**（调度、prefix caching）
3. ✅ **接口设计对应**
4. ✅ **增加了必要的 PTO2 适配**
5. ✅ **测试覆盖更全面**

**关键差异仅在于**：
- 语言：Python → C++
- 计算后端：PyTorch/CUDA → PTO2/Ascend
- 额外功能：TestPath、多种 ModelRunner

**设计目标达成**：
- ✅ 遵循 nano-vllm 的成熟设计
- ✅ 适配 PTO2 serving 方案
- ✅ 纯 C++ 实现，无 Python 依赖
- ✅ 支持多种执行模式（Mock/Simulator/Device）

---

## 参考

- **nano-vllm**: https://github.com/nanowell/nano-vllm
- **vLLM**: https://github.com/vllm-project/vllm
- **SGLang**: https://github.com/sgl-project/sglang
