# pypto-serving 快速开始

## 构建

```bash
cd /path/to/pypto-serving

# 创建构建目录
mkdir build && cd build

# 配置 (Release 模式,启用测试和示例)
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON

# 编译 (使用 8 个线程)
make -j8
```

## 运行测试

```bash
# 单元测试 (27 个测试)
./pypto_unit_tests

# 集成测试 (8 个测试)
./pypto_integration_tests

# 运行所有测试
ctest --output-on-failure
```

## 运行示例

### 1. Mock LLaMA 完整演示

```bash
./mock_llama
```

输出包含 5 个 demo:
- Demo 1: 单请求生成
- Demo 2: 批量生成
- Demo 3: 异步接口 (Test Path)
- Demo 4: Prefix Caching
- Demo 5: Temperature Sampling

### 2. 简单生成示例

```bash
./simple_generate
```

## 基本用法

### C++ API

```cpp
#include "engine/engine.h"

using namespace pypto;

int main() {
    // 1. 配置
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    // 2. 创建 Engine
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
    // 3. 准备输入
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params;
    params.temperature = 0.0f;  // Greedy
    params.max_tokens = 10;
    
    // 4. 生成
    auto outputs = engine->generate({prompt}, params);
    
    // 5. 获取结果
    TokenIds generated = outputs[0].output_token_ids;
    
    return 0;
}
```

### 异步接口 (Test Path)

```cpp
#include "frontend/test_path.h"

// 创建 TestPath
auto test_path = std::make_shared<TestPath>(engine);
test_path->start();

// 注入请求
RequestId req_id = test_path->inject_request(prompt, params);

// 获取响应 (阻塞直到完成)
TestResponse response = test_path->get_response(req_id);

// 停止
test_path->stop();
```

## 配置选项

### ModelConfig
```cpp
ModelConfig config;
config.vocab_size = 32000;          // 词表大小
config.hidden_size = 4096;          // 隐藏层维度
config.num_layers = 32;             // Transformer 层数
config.num_heads = 32;              // 注意力头数
config.eos_token_id = 2;            // EOS token ID
```

### CacheConfig
```cpp
CacheConfig config;
config.block_size = 16;             // 每个 block 的 token 数
config.num_gpu_blocks = 1024;       // GPU 上的 block 总数
config.enable_prefix_caching = true; // 启用前缀缓存
```

### SchedulerConfig
```cpp
SchedulerConfig config;
config.max_num_seqs = 256;          // 最大并发序列数
config.max_num_batched_tokens = 2048; // 每批最大 token 数
config.max_model_len = 2048;        // 最大序列长度
```

### SamplingParams
```cpp
SamplingParams params;
params.temperature = 1.0f;          // 温度 (0 = greedy)
params.top_k = -1;                  // Top-k (-1 = disabled)
params.top_p = 1.0f;                // Top-p (nucleus sampling)
params.max_tokens = 16;             // 最大生成 token 数
params.ignore_eos = false;          // 是否忽略 EOS
```

## 调试

### 启用 Debug 日志

```cpp
#include "common/logger.h"

Logger::instance().set_level(LogLevel::DEBUG);
```

### 启用 AddressSanitizer

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
make -j8
```

## 性能分析

### 构建 Release 版本

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
```

### 查看编译命令

```bash
# CMake 会生成 compile_commands.json
cat build/compile_commands.json
```

## 项目结构

```
pypto-serving/
├── src/                    # 源代码
│   ├── common/            # 通用类型、配置、日志
│   ├── engine/            # Engine、Scheduler、Sequence
│   ├── radix/             # BlockManager、KV Pool
│   ├── sampling/          # Sampler
│   └── frontend/          # TestPath
├── tests/                 # 测试
│   ├── unit/             # 单元测试
│   └── integration/      # 集成测试
├── examples/              # 示例程序
│   ├── mock_llama/       # Mock LLaMA 演示
│   └── simple_generate/  # 简单生成示例
├── CMakeLists.txt        # CMake 配置
└── README.md             # 项目说明
```

## 下一步

查看以下文档了解更多:
- [Phase 0 完成报告](PHASE0_COMPLETE.md)
- [实现计划 v2](implementation_plan_v2.md)
- [设计目标](design%20goal.md)

## 常见问题

### Q: 如何修改 vocab_size?
A: 在 `ModelConfig` 中设置 `vocab_size` 字段。

### Q: 如何调整内存大小?
A: 修改 `CacheConfig` 中的 `num_gpu_blocks` 和 `block_size`。

### Q: 如何启用前缀缓存?
A: 在 `CacheConfig` 中设置 `enable_prefix_caching = true` (默认已启用)。

### Q: 测试失败怎么办?
A: 运行 `ctest --output-on-failure` 查看详细错误信息。

### Q: 如何添加新的采样策略?
A: 在 `src/sampling/sampler.h/cpp` 中添加新方法。
