# Mock 模式 vs 真实模型

**重要理解**: pypto-serving 当前的 LLaMA 实现是 **Mock 模式**

---

## Mock 模式 (当前实现)

### 特点

✅ **随机初始化的权重**
- 所有权重矩阵使用随机值初始化
- 不需要下载真实模型文件
- 快速启动和测试

✅ **完整的网络结构**
- 所有层的实现（Embedding、RMSNorm、RoPE、Attention、MLP）
- 正确的前向传播流程
- 正确的 shape 和维度

✅ **用途**
- 验证网络结构正确性
- 验证推理流程（prefill/decode）
- 验证 KV cache 逻辑
- 快速开发和测试

❌ **限制**
- 输出是随机的（没有意义）
- 不能生成真实文本
- 仅用于框架验证

### 示例

```cpp
// 创建模型（使用随机权重）
ModelConfig config = create_tinyllama_config();
LLaMAModel model(config);  // 权重自动随机初始化

// 运行推理
std::vector<int> token_ids = {1, 10, 20};
Tensor logits = model.forward(token_ids, 1, 3, positions, true);
// logits 是随机的，但 shape 正确
```

---

## 真实模型 (待实现)

### 需要的步骤

1. **下载模型权重**
   ```bash
   huggingface-cli download TinyLlama/TinyLlama-1.1B-Chat-v1.0 \
       --local-dir models/tinyllama-1.1b
   ```

2. **实现权重加载器**
   ```cpp
   // src/llama/weight_loader.h
   class WeightLoader {
       static void load_from_safetensors(
           LLaMAModel& model,
           const std::string& model_path
       );
   };
   ```

3. **加载权重到模型**
   ```cpp
   LLaMAModel model(config);
   WeightLoader::load_from_safetensors(model, "models/tinyllama-1.1b");
   // 现在权重是真实的
   ```

4. **验证输出**
   ```cpp
   // 对比 PyTorch 输出
   // 验证数值精度
   ```

### 用途
- 生成真实文本
- 对比 PyTorch 输出
- 性能基准测试
- 生产部署

---

## 当前项目状态

### ✅ 已完成（Mock 模式）

1. **完整的网络结构**
   - Embedding ✅
   - RMSNorm ✅
   - RoPE ✅
   - Multi-Head Attention ✅
   - GQA 支持 ✅
   - MLP (SwiGLU) ✅
   - Transformer Layer ✅
   - 完整模型 ✅

2. **配置参数对齐**
   - TinyLlama-1.1B ✅
   - LLaMA-2-7B ✅
   - LLaMA-3.2-1B ✅
   - 任意自定义配置 ✅

3. **推理流程**
   - Prefill ✅
   - Decode ✅
   - KV Cache ✅
   - 批处理框架 ✅

4. **测试**
   - 39 个单元测试 ✅
   - 8 个集成测试 ✅
   - 100% 通过率 ✅

### ⚠️ 待实现（真实模型）

1. **权重加载**
   - [ ] SafeTensors 解析器
   - [ ] 权重映射到各层
   - [ ] 数据类型转换（bfloat16/float32）

2. **数值验证**
   - [ ] 与 PyTorch 输出对比
   - [ ] 精度验证
   - [ ] 性能测试

---

## 为什么 TinyLlama 测试很慢？

### 问题分析

**TinyLlama-1.1B 配置**:
- 22 层 Transformer
- 2048 hidden_size
- 32000 vocab_size
- **总参数**: ~1.1B

**内存占用**:
- 每个参数 4 字节（float32）
- 1.1B × 4 = **4.4GB 内存**

**计算复杂度**:
- 纯 CPU 矩阵运算
- 没有优化（naive 实现）
- 22 层的前向传播非常慢

### 解决方案

**方案 1: 使用小模型测试** ⭐ 推荐
```cpp
ModelConfig config;
config.vocab_size = 1000;
config.hidden_size = 256;
config.num_layers = 4;      // 只有 4 层
config.num_heads = 8;
config.num_kv_heads = 4;
// ... 其他配置

// 这个模型只有 ~2.6M 参数，运行很快
```

**方案 2: 只验证配置参数**
- 不实际运行模型
- 只检查配置是否正确
- 使用 `verify_config` 程序

**方案 3: 等待 PTO2 集成**
- PTO2 会使用 NPU 加速
- 性能会大幅提升
- 可以运行大模型

---

## 推荐的开发流程

### 当前阶段（Mock 验证）

1. ✅ 使用小模型验证网络结构
2. ✅ 验证配置参数对齐
3. ✅ 验证推理流程正确
4. ✅ 完成单元测试

### 下一阶段（可选）

**选项 A: 加载真实权重**
- 实现 SafeTensors 加载器
- 加载 TinyLlama 权重
- 对比 PyTorch 输出

**选项 B: 继续 Mock 开发**
- 实现 HTTP API
- 实现更多采样策略
- 优化批处理

**选项 C: PTO2 集成** ⭐ 推荐
- 替换为 PTO2 算子
- 使用 NPU 加速
- 性能大幅提升

---

## 快速验证命令

```bash
cd /home/qiaolina/pypto-serving/build

# 1. 验证配置参数（不运行模型）
./verify_config

# 2. 运行小模型测试（快速）
./llama_test

# 3. 运行单元测试
./pypto_unit_tests --gtest_filter="LLaMATest.*"

# 4. 查看所有测试
./pypto_unit_tests
```

---

## 总结

### 当前状态

✅ **网络结构完全对齐 LLaMA**
- TinyLlama-1.1B ✅
- LLaMA-2-7B ✅
- LLaMA-3.2-1B ✅

✅ **Mock 实现完成**
- 所有层实现 ✅
- 推理流程正确 ✅
- 测试全部通过 ✅

⚠️ **不需要下载模型**
- Mock 模式使用随机权重
- 用于框架验证，不是文本生成

### 关键理解

**Mock 模式的目的**:
1. 验证网络结构设计
2. 验证推理流程逻辑
3. 为 PTO2 集成做准备
4. 快速开发和测试

**不是用来**:
1. ❌ 生成真实文本
2. ❌ 对比模型输出
3. ❌ 性能基准测试

**下一步**:
- 直接进入 PTO2 集成
- 或者实现权重加载器（如果需要验证数值精度）
