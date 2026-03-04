# LLaMA 模型配置参考

本文档列出常见 LLaMA 模型的配置参数，用于验证 pypto-serving 的实现。

---

## TinyLlama-1.1B (最小的 LLaMA 模型)

**模型**: `TinyLlama/TinyLlama-1.1B-Chat-v1.0`  
**参数量**: 1.1B  
**用途**: 测试和验证

### 配置参数

```json
{
  "architectures": ["LlamaForCausalLM"],
  "vocab_size": 32000,
  "hidden_size": 2048,
  "num_hidden_layers": 22,
  "num_attention_heads": 32,
  "num_key_value_heads": 4,
  "intermediate_size": 5632,
  "max_position_embeddings": 2048,
  "rms_norm_eps": 1e-05,
  "rope_theta": 10000.0,
  "hidden_act": "silu",
  "bos_token_id": 1,
  "eos_token_id": 2,
  "pad_token_id": null
}
```

### 计算参数

- **head_dim**: 2048 / 32 = **64**
- **GQA ratio**: 32 / 4 = **8** (每个 KV head 对应 8 个 Q head)
- **总参数量**: ~1.1B

### pypto-serving 配置

```cpp
ModelConfig config;
config.vocab_size = 32000;
config.hidden_size = 2048;
config.num_layers = 22;
config.num_heads = 32;
config.num_kv_heads = 4;           // GQA
config.head_dim = 64;               // 2048 / 32
config.intermediate_size = 5632;
config.rope_theta = 10000.0f;
config.max_position_embeddings = 2048;
config.eos_token_id = 2;
config.pad_token_id = 0;
```

---

## LLaMA-2-7B

**模型**: `meta-llama/Llama-2-7b-hf`  
**参数量**: 7B  
**用途**: 标准 LLaMA 模型

### 配置参数

```json
{
  "architectures": ["LlamaForCausalLM"],
  "vocab_size": 32000,
  "hidden_size": 4096,
  "num_hidden_layers": 32,
  "num_attention_heads": 32,
  "num_key_value_heads": 32,
  "intermediate_size": 11008,
  "max_position_embeddings": 4096,
  "rms_norm_eps": 1e-05,
  "rope_theta": 10000.0,
  "hidden_act": "silu",
  "bos_token_id": 1,
  "eos_token_id": 2
}
```

### 计算参数

- **head_dim**: 4096 / 32 = **128**
- **GQA ratio**: 32 / 32 = **1** (标准 MHA，无 GQA)
- **总参数量**: ~7B

### pypto-serving 配置

```cpp
ModelConfig config;
config.vocab_size = 32000;
config.hidden_size = 4096;
config.num_layers = 32;
config.num_heads = 32;
config.num_kv_heads = 32;          // 标准 MHA
config.head_dim = 128;             // 4096 / 32
config.intermediate_size = 11008;
config.rope_theta = 10000.0f;
config.max_position_embeddings = 4096;
config.eos_token_id = 2;
config.pad_token_id = 0;
```

---

## LLaMA-3.2-1B

**模型**: `meta-llama/Llama-3.2-1B`  
**参数量**: 1B  
**用途**: 最新的小型 LLaMA 模型

### 配置参数

```json
{
  "architectures": ["LlamaForCausalLM"],
  "vocab_size": 128256,
  "hidden_size": 2048,
  "num_hidden_layers": 16,
  "num_attention_heads": 32,
  "num_key_value_heads": 8,
  "intermediate_size": 8192,
  "max_position_embeddings": 131072,
  "rms_norm_eps": 1e-05,
  "rope_theta": 500000.0,
  "hidden_act": "silu",
  "bos_token_id": 128000,
  "eos_token_id": 128001
}
```

### 计算参数

- **head_dim**: 2048 / 32 = **64**
- **GQA ratio**: 32 / 8 = **4**
- **总参数量**: ~1B

### pypto-serving 配置

```cpp
ModelConfig config;
config.vocab_size = 128256;
config.hidden_size = 2048;
config.num_layers = 16;
config.num_heads = 32;
config.num_kv_heads = 8;           // GQA
config.head_dim = 64;              // 2048 / 32
config.intermediate_size = 8192;
config.rope_theta = 500000.0f;
config.max_position_embeddings = 131072;
config.eos_token_id = 128001;
config.pad_token_id = 128000;
```

---

## 模型对比表

| 参数 | TinyLlama-1.1B | LLaMA-2-7B | LLaMA-3.2-1B |
|------|----------------|------------|--------------|
| **参数量** | 1.1B | 7B | 1B |
| **vocab_size** | 32,000 | 32,000 | 128,256 |
| **hidden_size** | 2,048 | 4,096 | 2,048 |
| **num_layers** | 22 | 32 | 16 |
| **num_heads** | 32 | 32 | 32 |
| **num_kv_heads** | 4 (GQA) | 32 (MHA) | 8 (GQA) |
| **head_dim** | 64 | 128 | 64 |
| **intermediate_size** | 5,632 | 11,008 | 8,192 |
| **max_seq_len** | 2,048 | 4,096 | 131,072 |
| **rope_theta** | 10,000 | 10,000 | 500,000 |

---

## 推荐测试模型

### 1. TinyLlama-1.1B ⭐ 推荐

**优点**:
- ✅ 最小（1.1B 参数）
- ✅ 完全兼容 LLaMA-2 架构
- ✅ 容易下载和运行
- ✅ 使用 GQA（验证 GQA 实现）
- ✅ 模型文件小（~2.2GB）

**用途**: 
- 验证网络结构
- 测试推理流程
- 性能基准测试

### 2. LLaMA-3.2-1B

**优点**:
- ✅ 最新架构
- ✅ 更大的词表（128K）
- ✅ 超长上下文（131K）

**缺点**:
- ⚠️ 需要处理更大的词表
- ⚠️ rope_theta 不同

---

## 验证步骤

### 1. 创建 TinyLlama 配置

```cpp
// examples/tinyllama_test/tinyllama_config.h

#include "common/types.h"

namespace pypto {

inline ModelConfig create_tinyllama_config() {
    ModelConfig config;
    
    // TinyLlama-1.1B 配置
    config.vocab_size = 32000;
    config.hidden_size = 2048;
    config.num_layers = 22;
    config.num_heads = 32;
    config.num_kv_heads = 4;           // GQA (8:1 ratio)
    config.head_dim = 64;              // 2048 / 32
    config.intermediate_size = 5632;
    config.rope_theta = 10000.0f;
    config.max_position_embeddings = 2048;
    config.eos_token_id = 2;
    config.pad_token_id = 0;
    
    return config;
}

} // namespace pypto
```

### 2. 下载模型权重

```bash
# 使用 huggingface-cli
pip install huggingface-hub
huggingface-cli download TinyLlama/TinyLlama-1.1B-Chat-v1.0 \
    --local-dir models/tinyllama-1.1b

# 或使用 Python
python3 << EOF
from transformers import AutoModelForCausalLM, AutoTokenizer

model = AutoModelForCausalLM.from_pretrained(
    "TinyLlama/TinyLlama-1.1B-Chat-v1.0",
    torch_dtype="float32"
)
tokenizer = AutoTokenizer.from_pretrained("TinyLlama/TinyLlama-1.1B-Chat-v1.0")

model.save_pretrained("models/tinyllama-1.1b")
tokenizer.save_pretrained("models/tinyllama-1.1b")
EOF
```

### 3. 加载权重到 pypto-serving

需要实现权重加载器：

```cpp
// src/llama/weight_loader.h

class WeightLoader {
public:
    // 从 PyTorch safetensors 或 bin 文件加载
    static void load_weights(
        LLaMAModel& model,
        const std::string& model_path
    );
    
private:
    // 解析 safetensors 格式
    static Tensor load_tensor(const std::string& path, const std::string& key);
};
```

### 4. 对比输出

```python
# 使用 PyTorch 运行 TinyLlama
import torch
from transformers import AutoModelForCausalLM

model = AutoModelForCausalLM.from_pretrained("TinyLlama/TinyLlama-1.1B-Chat-v1.0")
input_ids = torch.tensor([[1, 10, 20, 30]])
outputs = model(input_ids)
logits = outputs.logits
print("PyTorch logits:", logits[0, -1, :10])
```

```cpp
// 使用 pypto-serving 运行
ModelConfig config = create_tinyllama_config();
LLaMAModel model(config);
// load_weights(model, "models/tinyllama-1.1b");

std::vector<int> token_ids = {1, 10, 20, 30};
std::vector<int> positions = {0, 1, 2, 3};
Tensor logits = model.forward(token_ids, 1, 4, positions, true);
// 对比 logits 输出
```

---

## 当前状态

### ✅ 已实现
- 完整的 LLaMA 网络结构
- 所有层的算子实现
- 正确的配置参数映射
- GQA 支持

### ⚠️ 待实现（权重加载）
- [ ] SafeTensors 解析器
- [ ] 权重加载到各层
- [ ] 数值精度验证
- [ ] 与 PyTorch 输出对比

---

## 快速测试（不需要真实权重）

可以使用随机初始化的权重验证网络结构：

```bash
cd build

# 使用 TinyLlama 配置
./llama_test

# 输出应该显示:
# - 正确的 shape
# - 正确的前向传播
# - 正确的 KV cache 更新
```

---

## 下一步

### 选项 1: 验证网络结构（不需要权重）

```bash
# 创建 TinyLlama 配置测试
# 验证所有 shape 正确
# 验证前向传播不报错
```

### 选项 2: 加载真实权重

```bash
# 1. 下载 TinyLlama 模型
# 2. 实现权重加载器
# 3. 对比 PyTorch 输出
# 4. 验证数值精度
```

### 选项 3: 使用更小的测试模型

```bash
# 创建超小模型（用于快速验证）
# vocab_size: 1000
# hidden_size: 256
# num_layers: 2
# 快速迭代开发
```

---

## 推荐方案

**立即可做**（不需要下载模型）:

1. 创建 TinyLlama 配置的测试程序
2. 验证所有层的 shape 正确
3. 验证前向传播流程
4. 确保 GQA 逻辑正确

**后续可做**（需要模型权重）:

1. 实现 SafeTensors 加载器
2. 加载 TinyLlama 权重
3. 对比 PyTorch 输出
4. 数值精度验证
