# pypto-serving 实现计划 v2.0

**核心策略**: Mock-First 开发,先验证框架设计,再接入真实 PTO2 kernel。

---

## 参考资料

### 已完成的代码分析

1. **nano-vllm 架构分析** (`knowledge/inbox/nano-vllm-分析.md`)
   - Scheduler: waiting/running queue, prefill/decode 分离
   - BlockManager: hash-based 前缀缓存, PagedAttention
   - Sequence 管理: 请求生命周期, 状态机
   - 约 1400 行 Python, 转 C++ 后约 5000-8000 行

2. **SGLang RadixAttention 调用逻辑** (`knowledge/inbox/2026-03-03-sglang-RadixAttention调用逻辑分析.md`)
   - RadixCache: 显式 Radix Tree, 前缀匹配, 引用计数
   - 调度阶段: `match_prefix()` 查找前缀
   - 分配阶段: 只为未缓存部分分配 KV
   - 缓存阶段: `insert()` 插入树, 驱逐策略

3. **PTO2 构图与执行流程** (`knowledge/inbox/2026-03-03-simpler-PTO2构图与算子执行流程分析.md`)
   - Runtime: Task 数组, fanin/fanout 依赖图
   - Orchestration: `build_xxx_graph()` 构建任务图
   - 设备执行: AICPU 调度器 + AICore worker
   - Handshake 通信: AICPU ↔ AICore

4. **PTO 算子库分析** (`knowledge/inbox/2026-03-03-PTO算子库与LLM推理算子需求分析.md`)
   - 90+ 基础算子: Add, Mul, Matmul, Load, Store, Reduce, Broadcast
   - 高性能 Kernel: GEMM, Flash Attention, Paged Attention, TopK
   - LLaMA 所需: 10 个已有, 4 个需组合 (Softmax, RMSNorm, LayerNorm, RoPE)

---

## 设计目标 (来自 design goal.md)

1. C/C++ 高性能核心 (无 Python 在热路径)
2. OpenAI 风格前端 + 测试直通路径 (Test Path)
3. Radix Tree KV 历史管理 (GPU + persistent 文件池)
4. 使用 simpler 的 PTO2 执行模型
5. 禁止 Python 在自回归路径
6. 长期上下文无关 (仅 Radix Tree 持久化)
7. 通过测试路径注入数据
8. 复制并改造 simpler 的 golden.py

---

## 实施策略

### 核心原则: Mock-First

```
Phase 0: Mock 框架 (纯 C++, 无 PTO2)
  ├─> 验证架构设计
  ├─> 验证接口定义
  ├─> 验证数据流
  └─> 编译、测试、CI 流程

Phase 1: 接入 PTO2 (真实 kernel)
  ├─> 替换 Mock 实现
  ├─> 端到端验证
  └─> 性能调优

Phase 2: 生产特性 (批处理、QoS)
  └─> 扩展架构
```

---

## Phase 0: Mock 框架搭建 (2-3 周)

### 目标

**用 Mock 实现跑通完整的 LLaMA 推理流程**, 验证:
- ✅ C++ 架构设计合理
- ✅ Radix Cache 逻辑正确
- ✅ Scheduler 调度正确
- ✅ Test Path 接口可用
- ✅ 端到端数据流通畅

### 0.1 项目骨架

```
pypto-serving/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── architecture.md
│   ├── reference_nano_vllm.md       # nano-vllm 参考分析
│   ├── reference_sglang.md          # SGLang 参考分析
│   ├── reference_pto2.md            # PTO2 参考分析
│   └── reference_pto_ops.md         # PTO 算子库分析
├── src/
│   ├── common/
│   │   ├── types.h                  # 基础类型定义
│   │   ├── config.h/cpp             # 配置管理
│   │   └── logger.h/cpp             # 日志
│   ├── engine/
│   │   ├── engine.h/cpp             # 引擎主类
│   │   ├── scheduler.h/cpp          # 调度器 (参考 nano-vllm)
│   │   ├── sequence.h/cpp           # 请求序列管理
│   │   └── model_runner_mock.h/cpp  # Mock 模型执行器
│   ├── radix/
│   │   ├── radix_cache.h/cpp        # Radix Tree (参考 SGLang)
│   │   ├── block_manager.h/cpp      # KV 块管理 (参考 nano-vllm)
│   │   └── kv_pool_mock.h/cpp       # Mock KV 池 (先只用内存)
│   ├── frontend/
│   │   ├── test_path.h/cpp          # Test Path 接口
│   │   └── request.h/cpp            # 请求/响应结构
│   └── sampling/
│       ├── sampler.h/cpp            # 采样器
│       └── sampling_params.h        # 采样参数
├── tests/
│   ├── unit/
│   │   ├── test_radix_cache.cpp     # Radix Cache 单元测试
│   │   ├── test_block_manager.cpp   # Block Manager 单元测试
│   │   ├── test_scheduler.cpp       # Scheduler 单元测试
│   │   └── test_sampler.cpp         # Sampler 单元测试
│   └── integration/
│       ├── test_single_request.cpp  # 单请求端到端测试
│       └── test_batch_requests.cpp  # 批量请求测试
├── examples/
│   ├── mock_llama/
│   │   ├── mock_llama.cpp           # Mock LLaMA 模型
│   │   ├── golden.py                # Golden 测试 (可选)
│   │   └── run_test.cpp             # C++ 测试 runner
│   └── simple_generate/
│       └── simple_generate.cpp      # 简单生成示例
└── third_party/
    └── (预留给 simpler submodule)
```

**产出**: 
- CMakeLists.txt 可编译
- 目录结构清晰
- README 说明项目目标和架构

---

### 0.2 基础类型定义

```cpp
// src/common/types.h

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace pypto {

// Token ID 类型
using TokenId = int32_t;
using TokenIds = std::vector<TokenId>;

// Block ID 类型
using BlockId = int32_t;

// 请求 ID
using RequestId = std::string;

// 采样参数
struct SamplingParams {
    float temperature = 1.0f;
    int max_tokens = 256;
    TokenIds stop_tokens;
    int top_k = -1;
    float top_p = 1.0f;
    bool ignore_eos = false;
};

// 请求状态
enum class SequenceStatus {
    WAITING,
    RUNNING,
    FINISHED,
    ABORTED
};

// 完成原因
enum class FinishReason {
    NONE,
    EOS,
    STOP_TOKEN,
    MAX_TOKENS,
    ABORT
};

// 模型配置
struct ModelConfig {
    int vocab_size = 32000;
    int hidden_size = 4096;
    int num_layers = 32;
    int num_heads = 32;
    int num_kv_heads = 32;  // GQA
    int head_dim = 128;
    int intermediate_size = 11008;
    int max_position_embeddings = 2048;
    float rms_norm_eps = 1e-6f;
    TokenId eos_token_id = 2;
};

// KV Cache 配置
struct CacheConfig {
    int block_size = 16;
    int num_gpu_blocks = 1024;
    int num_cpu_blocks = 0;  // 我们不用 CPU 层
};

} // namespace pypto
```

**产出**: `src/common/types.h`

---

### 0.3 Sequence 管理 (参考 nano-vllm)

```cpp
// src/engine/sequence.h

#include "common/types.h"
#include <vector>
#include <string>

namespace pypto {

class Sequence {
public:
    Sequence(
        const RequestId& request_id,
        const TokenIds& prompt_token_ids,
        const SamplingParams& sampling_params
    );
    
    // 基础信息
    RequestId request_id() const { return request_id_; }
    const TokenIds& prompt_token_ids() const { return prompt_token_ids_; }
    const TokenIds& output_token_ids() const { return output_token_ids_; }
    TokenIds all_token_ids() const;  // prompt + output
    
    // 状态管理
    SequenceStatus status() const { return status_; }
    void set_status(SequenceStatus status) { status_ = status; }
    bool is_finished() const { return status_ == SequenceStatus::FINISHED; }
    
    // Token 操作
    void append_token(TokenId token_id);
    int num_prompt_tokens() const { return prompt_token_ids_.size(); }
    int num_output_tokens() const { return output_token_ids_.size(); }
    int num_total_tokens() const { return num_prompt_tokens() + num_output_tokens(); }
    
    // KV Cache 管理
    const std::vector<BlockId>& block_table() const { return block_table_; }
    std::vector<BlockId>& block_table() { return block_table_; }
    int num_blocks() const;
    int num_cached_tokens() const { return num_cached_tokens_; }
    void set_num_cached_tokens(int n) { num_cached_tokens_ = n; }
    
    // 采样参数
    const SamplingParams& sampling_params() const { return sampling_params_; }
    
    // 完成判定
    FinishReason finish_reason() const { return finish_reason_; }
    void set_finish_reason(FinishReason reason) { finish_reason_ = reason; }
    
private:
    RequestId request_id_;
    TokenIds prompt_token_ids_;
    TokenIds output_token_ids_;
    SamplingParams sampling_params_;
    
    SequenceStatus status_ = SequenceStatus::WAITING;
    FinishReason finish_reason_ = FinishReason::NONE;
    
    // KV Cache
    std::vector<BlockId> block_table_;
    int num_cached_tokens_ = 0;
};

} // namespace pypto
```

**产出**: `src/engine/sequence.h/cpp`

---

### 0.4 Block Manager (参考 nano-vllm)

```cpp
// src/radix/block_manager.h

#include "common/types.h"
#include <unordered_map>
#include <deque>
#include <vector>

namespace pypto {

struct Block {
    BlockId block_id;
    int ref_count = 0;
    uint64_t hash = 0;
    TokenIds token_ids;
    
    void reset() {
        ref_count = 1;
        hash = 0;
        token_ids.clear();
    }
};

class BlockManager {
public:
    BlockManager(int num_blocks, int block_size);
    
    // 查询
    bool can_allocate(const Sequence& seq) const;
    bool can_append(const Sequence& seq) const;
    
    // 分配
    void allocate(Sequence& seq);
    void deallocate(Sequence& seq);
    void may_append(Sequence& seq);
    
    // 统计
    int num_free_blocks() const { return free_block_ids_.size(); }
    int num_used_blocks() const { return used_block_ids_.size(); }
    
private:
    int block_size_;
    std::vector<Block> blocks_;
    std::unordered_map<uint64_t, BlockId> hash_to_block_id_;
    std::deque<BlockId> free_block_ids_;
    std::unordered_set<BlockId> used_block_ids_;
    
    uint64_t compute_hash(const TokenIds& token_ids, uint64_t prefix_hash = 0) const;
};

} // namespace pypto
```

**产出**: `src/radix/block_manager.h/cpp`

---

### 0.5 Scheduler (参考 nano-vllm)

```cpp
// src/engine/scheduler.h

#include "common/types.h"
#include "engine/sequence.h"
#include "radix/block_manager.h"
#include <deque>
#include <memory>

namespace pypto {

class Scheduler {
public:
    Scheduler(
        const ModelConfig& model_config,
        const CacheConfig& cache_config,
        int max_num_seqs = 256,
        int max_num_batched_tokens = 2048
    );
    
    // 请求管理
    void add_request(std::shared_ptr<Sequence> seq);
    bool is_finished() const;
    
    // 调度
    struct ScheduleOutput {
        std::vector<std::shared_ptr<Sequence>> seqs;
        bool is_prefill;
    };
    ScheduleOutput schedule();
    
    // 后处理
    void postprocess(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        const std::vector<TokenId>& token_ids
    );
    
private:
    ModelConfig model_config_;
    CacheConfig cache_config_;
    int max_num_seqs_;
    int max_num_batched_tokens_;
    TokenId eos_token_id_;
    
    std::unique_ptr<BlockManager> block_manager_;
    std::deque<std::shared_ptr<Sequence>> waiting_;
    std::deque<std::shared_ptr<Sequence>> running_;
    
    void preempt(std::shared_ptr<Sequence> seq);
};

} // namespace pypto
```

**产出**: `src/engine/scheduler.h/cpp`

---

### 0.6 Mock Model Runner

```cpp
// src/engine/model_runner_mock.h

#include "common/types.h"
#include "engine/sequence.h"
#include <vector>
#include <random>

namespace pypto {

class MockModelRunner {
public:
    MockModelRunner(const ModelConfig& model_config);
    
    // 执行推理 (Mock 实现)
    std::vector<TokenId> run(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill
    );
    
private:
    ModelConfig model_config_;
    std::mt19937 rng_;
    
    // Mock 实现: 随机生成 token
    TokenId mock_generate_token();
    
    // Mock 实现: 模拟计算延迟
    void mock_compute_delay(int num_tokens, bool is_prefill);
};

// 实现
MockModelRunner::MockModelRunner(const ModelConfig& config)
    : model_config_(config), rng_(std::random_device{}()) {}

std::vector<TokenId> MockModelRunner::run(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill
) {
    std::vector<TokenId> output_tokens;
    
    if (is_prefill) {
        // Prefill: 每个 seq 生成第一个 token
        int total_tokens = 0;
        for (const auto& seq : seqs) {
            total_tokens += seq->num_prompt_tokens();
        }
        mock_compute_delay(total_tokens, true);
        
        for (size_t i = 0; i < seqs.size(); i++) {
            output_tokens.push_back(mock_generate_token());
        }
    } else {
        // Decode: 每个 seq 生成下一个 token
        mock_compute_delay(seqs.size(), false);
        
        for (size_t i = 0; i < seqs.size(); i++) {
            output_tokens.push_back(mock_generate_token());
        }
    }
    
    return output_tokens;
}

TokenId MockModelRunner::mock_generate_token() {
    // 随机生成 token (避免 EOS,让序列能跑完)
    std::uniform_int_distribution<TokenId> dist(3, model_config_.vocab_size - 1);
    return dist(rng_);
}

void MockModelRunner::mock_compute_delay(int num_tokens, bool is_prefill) {
    // 模拟计算延迟
    // Prefill: ~10 us/token
    // Decode: ~100 us/token
    int delay_us = is_prefill ? num_tokens * 10 : num_tokens * 100;
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
}

} // namespace pypto
```

**产出**: `src/engine/model_runner_mock.h/cpp`

---

### 0.7 Engine 主类

```cpp
// src/engine/engine.h

#include "common/types.h"
#include "engine/scheduler.h"
#include "engine/model_runner_mock.h"
#include "engine/sequence.h"
#include "sampling/sampler.h"
#include <memory>
#include <vector>

namespace pypto {

class Engine {
public:
    Engine(
        const ModelConfig& model_config,
        const CacheConfig& cache_config
    );
    
    // 添加请求
    void add_request(
        const RequestId& request_id,
        const TokenIds& prompt_token_ids,
        const SamplingParams& sampling_params
    );
    
    // 执行一步
    struct StepOutput {
        std::vector<RequestId> finished_request_ids;
        std::vector<TokenIds> output_token_ids;
    };
    StepOutput step();
    
    // 检查是否完成
    bool is_finished() const { return scheduler_->is_finished(); }
    
    // 生成 (主接口)
    struct GenerateOutput {
        TokenIds output_token_ids;
        FinishReason finish_reason;
    };
    std::vector<GenerateOutput> generate(
        const std::vector<TokenIds>& prompts,
        const SamplingParams& sampling_params
    );
    
private:
    ModelConfig model_config_;
    CacheConfig cache_config_;
    
    std::unique_ptr<Scheduler> scheduler_;
    std::unique_ptr<MockModelRunner> model_runner_;
    std::unique_ptr<Sampler> sampler_;
    
    int next_request_id_ = 0;
};

} // namespace pypto
```

**产出**: `src/engine/engine.h/cpp`

---

### 0.8 Test Path 接口

```cpp
// src/frontend/test_path.h

#include "common/types.h"
#include "engine/engine.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace pypto {

struct TestRequest {
    RequestId request_id;
    TokenIds prompt_token_ids;
    SamplingParams sampling_params;
};

struct TestResponse {
    RequestId request_id;
    TokenIds output_token_ids;
    FinishReason finish_reason;
    bool ready = false;
};

class TestPath {
public:
    TestPath(std::shared_ptr<Engine> engine);
    
    // 注入请求 (线程安全)
    RequestId inject_request(
        const TokenIds& prompt_token_ids,
        const SamplingParams& sampling_params
    );
    
    // 获取响应 (阻塞直到完成)
    TestResponse get_response(const RequestId& request_id);
    
    // 启动处理线程
    void start();
    void stop();
    
private:
    std::shared_ptr<Engine> engine_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<RequestId, TestResponse> responses_;
    
    bool running_ = false;
    std::thread worker_thread_;
    
    void worker_loop();
};

} // namespace pypto
```

**产出**: `src/frontend/test_path.h/cpp`

---

### 0.9 Sampler (采样器)

```cpp
// src/sampling/sampler.h

#include "common/types.h"
#include <vector>
#include <random>

namespace pypto {

class Sampler {
public:
    Sampler(int vocab_size);
    
    // 采样单个 token
    TokenId sample(
        const std::vector<float>& logits,
        const SamplingParams& params
    );
    
    // 批量采样
    std::vector<TokenId> sample_batch(
        const std::vector<std::vector<float>>& logits_batch,
        const std::vector<SamplingParams>& params_batch
    );
    
private:
    int vocab_size_;
    std::mt19937 rng_;
    
    TokenId greedy_sample(const std::vector<float>& logits);
    TokenId temperature_sample(const std::vector<float>& logits, float temperature);
    TokenId top_k_sample(const std::vector<float>& logits, int top_k, float temperature);
    TokenId top_p_sample(const std::vector<float>& logits, float top_p, float temperature);
};

} // namespace pypto
```

**产出**: `src/sampling/sampler.h/cpp`

---

### 0.10 单元测试

```cpp
// tests/unit/test_scheduler.cpp

#include <gtest/gtest.h>
#include "engine/scheduler.h"
#include "engine/sequence.h"

using namespace pypto;

TEST(SchedulerTest, BasicPrefillDecode) {
    ModelConfig model_config;
    CacheConfig cache_config;
    Scheduler scheduler(model_config, cache_config);
    
    // 添加请求
    auto seq = std::make_shared<Sequence>(
        "req-1",
        TokenIds{1, 2, 3, 4, 5},
        SamplingParams{}
    );
    scheduler.add_request(seq);
    
    // 调度 Prefill
    auto output = scheduler.schedule();
    EXPECT_EQ(output.seqs.size(), 1);
    EXPECT_TRUE(output.is_prefill);
    EXPECT_EQ(output.seqs[0]->request_id(), "req-1");
    
    // 后处理 (添加第一个输出 token)
    scheduler.postprocess(output.seqs, {100});
    
    // 调度 Decode
    output = scheduler.schedule();
    EXPECT_EQ(output.seqs.size(), 1);
    EXPECT_FALSE(output.is_prefill);
    
    // 继续 Decode 直到完成
    for (int i = 0; i < 10; i++) {
        scheduler.postprocess(output.seqs, {100 + i});
        if (scheduler.is_finished()) break;
        output = scheduler.schedule();
    }
}

TEST(SchedulerTest, MultipleRequests) {
    // 测试多请求调度
    // ...
}

TEST(SchedulerTest, Preemption) {
    // 测试抢占逻辑
    // ...
}
```

**产出**: `tests/unit/test_*.cpp` (5-10 个测试文件)

---

### 0.11 集成测试

```cpp
// tests/integration/test_single_request.cpp

#include <gtest/gtest.h>
#include "engine/engine.h"
#include "frontend/test_path.h"

using namespace pypto;

TEST(IntegrationTest, SingleRequestGenerate) {
    // 1. 创建引擎
    ModelConfig model_config;
    CacheConfig cache_config;
    auto engine = std::make_shared<Engine>(model_config, cache_config);
    
    // 2. 创建 Test Path
    TestPath test_path(engine);
    test_path.start();
    
    // 3. 注入请求
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params;
    params.max_tokens = 20;
    params.temperature = 0.8f;
    
    auto request_id = test_path.inject_request(prompt, params);
    
    // 4. 获取响应
    auto response = test_path.get_response(request_id);
    
    // 5. 验证
    EXPECT_TRUE(response.ready);
    EXPECT_EQ(response.output_token_ids.size(), 20);
    EXPECT_NE(response.finish_reason, FinishReason::NONE);
    
    test_path.stop();
}

TEST(IntegrationTest, BatchRequests) {
    // 测试批量请求
    // ...
}

TEST(IntegrationTest, PrefixCaching) {
    // 测试前缀缓存
    // ...
}
```

**产出**: `tests/integration/test_*.cpp`

---

### 0.12 Mock LLaMA 示例

```cpp
// examples/mock_llama/mock_llama.cpp

#include "engine/engine.h"
#include "frontend/test_path.h"
#include <iostream>

using namespace pypto;

int main() {
    // 1. 配置 (LLaMA-7B 参数)
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    model_config.hidden_size = 4096;
    model_config.num_layers = 32;
    model_config.num_heads = 32;
    model_config.num_kv_heads = 32;
    model_config.head_dim = 128;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    // 2. 创建引擎
    auto engine = std::make_shared<Engine>(model_config, cache_config);
    
    // 3. 创建 Test Path
    TestPath test_path(engine);
    test_path.start();
    
    // 4. 测试用例 1: 单个请求
    std::cout << "=== Test Case 1: Single Request ===" << std::endl;
    {
        TokenIds prompt = {1, 2, 3, 4, 5, 6, 7, 8};
        SamplingParams params;
        params.max_tokens = 50;
        params.temperature = 0.8f;
        
        auto request_id = test_path.inject_request(prompt, params);
        auto response = test_path.get_response(request_id);
        
        std::cout << "Prompt length: " << prompt.size() << std::endl;
        std::cout << "Output length: " << response.output_token_ids.size() << std::endl;
        std::cout << "Finish reason: " << static_cast<int>(response.finish_reason) << std::endl;
    }
    
    // 5. 测试用例 2: 前缀缓存
    std::cout << "\n=== Test Case 2: Prefix Caching ===" << std::endl;
    {
        TokenIds prompt1 = {1, 2, 3, 4, 5, 6, 7, 8};
        TokenIds prompt2 = {1, 2, 3, 4, 5, 9, 10, 11};  // 共享前缀 [1,2,3,4,5]
        
        SamplingParams params;
        params.max_tokens = 20;
        
        auto req1 = test_path.inject_request(prompt1, params);
        auto req2 = test_path.inject_request(prompt2, params);
        
        auto resp1 = test_path.get_response(req1);
        auto resp2 = test_path.get_response(req2);
        
        std::cout << "Request 1 output: " << resp1.output_token_ids.size() << " tokens" << std::endl;
        std::cout << "Request 2 output: " << resp2.output_token_ids.size() << " tokens" << std::endl;
        std::cout << "Prefix caching should reuse blocks for [1,2,3,4,5]" << std::endl;
    }
    
    // 6. 测试用例 3: 批量请求
    std::cout << "\n=== Test Case 3: Batch Requests ===" << std::endl;
    {
        std::vector<RequestId> request_ids;
        for (int i = 0; i < 5; i++) {
            TokenIds prompt(10 + i, i + 1);  // 不同长度的 prompt
            SamplingParams params;
            params.max_tokens = 30;
            request_ids.push_back(test_path.inject_request(prompt, params));
        }
        
        for (const auto& rid : request_ids) {
            auto response = test_path.get_response(rid);
            std::cout << "Request " << rid << ": " 
                      << response.output_token_ids.size() << " tokens" << std::endl;
        }
    }
    
    test_path.stop();
    
    std::cout << "\n=== All Tests Passed ===" << std::endl;
    return 0;
}
```

**产出**: `examples/mock_llama/mock_llama.cpp`

---

### 0.13 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(pypto-serving CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 选项
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)

# 依赖
find_package(Threads REQUIRED)

# 源文件
set(PYPTO_SOURCES
    src/common/config.cpp
    src/common/logger.cpp
    src/engine/sequence.cpp
    src/engine/scheduler.cpp
    src/engine/model_runner_mock.cpp
    src/engine/engine.cpp
    src/radix/block_manager.cpp
    src/radix/radix_cache.cpp
    src/sampling/sampler.cpp
    src/frontend/test_path.cpp
)

# 库
add_library(pypto_core STATIC ${PYPTO_SOURCES})
target_include_directories(pypto_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(pypto_core PUBLIC Threads::Threads)

# 测试
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    
    file(GLOB_RECURSE TEST_SOURCES tests/unit/*.cpp tests/integration/*.cpp)
    add_executable(pypto_tests ${TEST_SOURCES})
    target_link_libraries(pypto_tests PRIVATE pypto_core GTest::gtest GTest::gtest_main)
    
    add_test(NAME pypto_tests COMMAND pypto_tests)
endif()

# 示例
if(BUILD_EXAMPLES)
    add_executable(mock_llama examples/mock_llama/mock_llama.cpp)
    target_link_libraries(mock_llama PRIVATE pypto_core)
endif()
```

**产出**: `CMakeLists.txt`

---

### 0.14 验收标准

```bash
# 编译
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
make -j

# 运行单元测试
./pypto_tests

# 运行集成测试
./pypto_tests --gtest_filter="IntegrationTest.*"

# 运行 Mock LLaMA 示例
./mock_llama

# 预期输出:
# === Test Case 1: Single Request ===
# Prompt length: 8
# Output length: 50
# Finish reason: 2
#
# === Test Case 2: Prefix Caching ===
# Request 1 output: 20 tokens
# Request 2 output: 20 tokens
# Prefix caching should reuse blocks for [1,2,3,4,5]
#
# === Test Case 3: Batch Requests ===
# Request req-2: 30 tokens
# Request req-3: 30 tokens
# ...
#
# === All Tests Passed ===
```

---

## Phase 1: 接入真实 PTO2 (2-3 周)

### 前置条件

Phase 0 完成,Mock 框架跑通。

### 1.1 集成 simpler

```bash
# 添加 simpler 为 submodule
cd pypto-serving
git submodule add ../simpler third_party/simpler
git submodule update --init --recursive
```

```cmake
# CMakeLists.txt 更新

# 添加 simpler
add_subdirectory(third_party/simpler EXCLUDE_FROM_ALL)

# 链接 PTO2 Runtime
target_link_libraries(pypto_core PUBLIC pto_runtime)
target_include_directories(pypto_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/simpler/src/runtime/host_build_graph
)
```

**产出**: simpler 集成到构建系统

---

### 1.2 替换 Mock Model Runner

```cpp
// src/engine/model_runner_pto.h

#include "common/types.h"
#include "engine/sequence.h"
#include "runtime.h"  // PTO2 Runtime
#include <vector>
#include <memory>

namespace pypto {

class PtoModelRunner {
public:
    PtoModelRunner(
        const ModelConfig& model_config,
        const CacheConfig& cache_config
    );
    
    ~PtoModelRunner();
    
    // 执行推理 (真实 PTO2)
    std::vector<TokenId> run(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill
    );
    
private:
    ModelConfig model_config_;
    CacheConfig cache_config_;
    
    Runtime* runtime_;
    
    // Kernel binaries
    std::vector<uint8_t> qk_matmul_kernel_;
    std::vector<uint8_t> pv_matmul_kernel_;
    std::vector<uint8_t> softmax_kernel_;
    std::vector<uint8_t> online_update_kernel_;
    
    // Orchestration
    void build_prefill_graph(
        const std::vector<std::shared_ptr<Sequence>>& seqs
    );
    
    void build_decode_graph(
        const std::vector<std::shared_ptr<Sequence>>& seqs
    );
    
    void load_kernels();
    void register_kernels();
};

} // namespace pypto
```

**产出**: `src/engine/model_runner_pto.h/cpp`

---

### 1.3 复用 simpler 的 Kernels

```bash
# 复制 paged_attention kernels
cp -r ../simpler/examples/host_build_graph/paged_attention/kernels/ \
      pypto-serving/src/kernels/paged_attention/
```

**产出**: `src/kernels/paged_attention/` (复制的 kernel 代码)

---

### 1.4 实现 Orchestration

```cpp
// src/orchestration/attention_orch.cpp

#include "runtime.h"
#include <iostream>

#define FUNC_QK_MATMUL       0
#define FUNC_SOFTMAX_PREPARE 1
#define FUNC_PV_MATMUL       2
#define FUNC_ONLINE_UPDATE   3

extern "C" {

int build_attention_graph(Runtime* runtime, uint64_t* args, int arg_count) {
    // 参考 paged_attention_orch.cpp 实现
    // 但参数从 Engine 传入,不是从 golden.py
    
    void* dev_query = reinterpret_cast<void*>(args[0]);
    void* dev_key_cache = reinterpret_cast<void*>(args[1]);
    void* dev_value_cache = reinterpret_cast<void*>(args[2]);
    // ... 其他参数
    
    // 构建任务图
    // ...
    
    return 0;
}

} // extern "C"
```

**产出**: `src/orchestration/attention_orch.cpp`

---

### 1.5 端到端测试

```cpp
// tests/integration/test_pto_integration.cpp

TEST(PtoIntegrationTest, SingleAttentionPass) {
    // 1. 创建引擎 (使用 PtoModelRunner)
    ModelConfig model_config;
    CacheConfig cache_config;
    auto engine = std::make_shared<Engine>(model_config, cache_config);
    
    // 2. 单个 attention 计算
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params;
    params.max_tokens = 10;
    
    auto outputs = engine->generate({prompt}, params);
    
    // 3. 验证输出
    EXPECT_EQ(outputs.size(), 1);
    EXPECT_EQ(outputs[0].output_token_ids.size(), 10);
}
```

**产出**: 端到端测试通过

---

## Phase 2: 完整 LLaMA 推理 (3-4 周)

### 2.1 实现缺失算子

基于 PTO Tile Lib 组合实现:

```
src/kernels/llama/
├── rmsnorm.cpp        # RMSNorm (组合: RowSum + Sqrt + RowExpandDiv + RowExpandMul)
├── rope.cpp           # RoPE (组合: Extract + Mul + Add/Sub + Insert)
├── silu.cpp           # SiLU (PTO 已有)
├── embedding.cpp      # Embedding (Gather)
└── lm_head.cpp        # LM Head (Matmul)
```

### 2.2 完整 LLaMA Layer

```cpp
// src/orchestration/llama_layer_orch.cpp

extern "C" int build_llama_layer_graph(
    Runtime* runtime,
    uint64_t* args,
    int arg_count
) {
    // 1. RMSNorm (input)
    int t_norm1 = runtime->add_task(..., FUNC_RMSNORM, CoreType::AIV);
    
    // 2. QKV Projection (3 个 Matmul)
    int t_q = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    int t_k = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    int t_v = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    
    // 3. RoPE (Q 和 K)
    int t_rope_q = runtime->add_task(..., FUNC_ROPE, CoreType::AIV);
    int t_rope_k = runtime->add_task(..., FUNC_ROPE, CoreType::AIV);
    
    // 4. Paged Attention (复用 simpler 的实现)
    int t_attn = runtime->add_task(..., FUNC_PAGED_ATTENTION, CoreType::AIC);
    
    // 5. O Projection
    int t_o = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    
    // 6. Residual Add
    int t_add1 = runtime->add_task(..., FUNC_ADD, CoreType::AIV);
    
    // 7. RMSNorm (FFN input)
    int t_norm2 = runtime->add_task(..., FUNC_RMSNORM, CoreType::AIV);
    
    // 8. FFN (Gate + Up)
    int t_gate = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    int t_up = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    
    // 9. SiLU
    int t_silu = runtime->add_task(..., FUNC_SILU, CoreType::AIV);
    
    // 10. Element-wise Mul
    int t_mul = runtime->add_task(..., FUNC_MUL, CoreType::AIV);
    
    // 11. Down Projection
    int t_down = runtime->add_task(..., FUNC_MATMUL, CoreType::AIC);
    
    // 12. Residual Add
    int t_add2 = runtime->add_task(..., FUNC_ADD, CoreType::AIV);
    
    // 建立依赖
    runtime->add_successor(t_norm1, t_q);
    runtime->add_successor(t_norm1, t_k);
    runtime->add_successor(t_norm1, t_v);
    runtime->add_successor(t_q, t_rope_q);
    runtime->add_successor(t_k, t_rope_k);
    runtime->add_successor(t_rope_q, t_attn);
    runtime->add_successor(t_rope_k, t_attn);
    runtime->add_successor(t_v, t_attn);
    // ... 更多依赖
    
    return 0;
}
```

### 2.3 验收

- [ ] 单层 LLaMA 推理通过
- [ ] 多层 LLaMA 推理通过
- [ ] 与 PyTorch 实现的输出对比 (RTOL < 1e-2)

---

## Phase 3: 性能优化 (2-3 周)

### 3.1 Pipeline 优化

参考 GEMM Kernel 的 Double Buffering 和 Event 同步。

### 3.2 Batch 优化

实现 batch prefill 和 batch decode。

### 3.3 自回归循环优化

探索设备内循环的实现方式。

---

## 关键里程碑

| 里程碑 | 时间 | 验收标准 |
|--------|------|----------|
| **M0: Mock 框架** | Week 2-3 | Mock LLaMA 跑通, 所有单元测试通过 |
| **M1: PTO2 集成** | Week 5-6 | 单层 Attention 用真实 kernel 跑通 |
| **M2: 完整推理** | Week 8-10 | 完整 LLaMA 推理, 与 PyTorch 对齐 |
| **M3: 性能优化** | Week 12-14 | 吞吐量达到目标 |

---

## 立即行动: Phase 0 任务清单

### Week 1: 基础框架

- [ ] 0.1 创建目录结构
- [ ] 0.2 实现 types.h (基础类型)
- [ ] 0.3 实现 Sequence 类
- [ ] 0.4 实现 BlockManager (hash-based)
- [ ] 0.5 实现 Scheduler
- [ ] 0.6 编写 Scheduler 单元测试

### Week 2: Engine 和 Test Path

- [ ] 0.7 实现 MockModelRunner
- [ ] 0.8 实现 Sampler
- [ ] 0.9 实现 Engine 主类
- [ ] 0.10 实现 Test Path
- [ ] 0.11 编写集成测试

### Week 3: 示例和文档

- [ ] 0.12 实现 Mock LLaMA 示例
- [ ] 0.13 编写 README 和架构文档
- [ ] 0.14 CI/CD 配置 (GitHub Actions)
- [ ] 0.15 代码 Review 和重构

---

## 参考实现对照表

| 组件 | 参考 | 文件 |
|------|------|------|
| Sequence | nano-vllm | `nanovllm/engine/sequence.py` |
| BlockManager | nano-vllm | `nanovllm/engine/block_manager.py` |
| Scheduler | nano-vllm | `nanovllm/engine/scheduler.py` |
| RadixCache | SGLang | `sglang/srt/mem_cache/radix_cache.py` |
| Engine | nano-vllm | `nanovllm/engine/llm_engine.py` |
| Runtime | simpler | `simpler/src/runtime/host_build_graph/runtime/runtime.h` |
| Orchestration | simpler | `simpler/examples/.../paged_attention_orch.cpp` |
| Kernels | simpler + PTO-ISA | `simpler/examples/.../kernels/` + `pto-isa/kernels/manual/` |

---

## 关键设计决策

### 1. Mock 策略

**Mock 什么**:
- ✅ Model Runner: 随机生成 token, 模拟延迟
- ✅ KV Pool: 只用内存, 不用文件
- ❌ Scheduler: 真实实现 (核心逻辑)
- ❌ BlockManager: 真实实现 (核心逻辑)
- ❌ RadixCache: 真实实现 (核心逻辑)

**为什么**:
- Scheduler/BlockManager/RadixCache 是框架的核心, 必须真实实现
- Model Runner 可以 Mock, 因为只是生成 token, 不影响调度逻辑
- KV Pool 先用内存, 验证逻辑, 后续再加文件持久化

### 2. 不用 Python Golden

**Phase 0 完全用 C++ 测试**:
- C++ 单元测试 (GTest)
- C++ 集成测试
- C++ 示例程序

**Phase 1 之后可选加 Python Golden**:
- 用于与 PyTorch 参考实现对比
- 但不是必须的

### 3. 渐进式实现

```
Phase 0: Mock 框架
  └─> 验证: 架构 + 接口 + 数据流

Phase 1: 真实 Attention
  └─> 验证: PTO2 集成 + Kernel 调用

Phase 2: 完整 LLaMA
  └─> 验证: 端到端推理 + 正确性

Phase 3: 性能优化
  └─> 验证: 吞吐量 + 延迟
```

---

## 下一步行动

1. **创建 GitHub repo** (或本地 git init)
2. **创建目录结构** (按 0.1)
3. **实现 types.h** (按 0.2)
4. **实现 Sequence** (按 0.3, 参考 nano-vllm)
5. **实现 BlockManager** (按 0.4, 参考 nano-vllm)

**预计 3 周完成 Phase 0, 跑通 Mock 框架!**
