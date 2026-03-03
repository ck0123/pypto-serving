# 执行模式快速参考

## 三种执行模式

### 🔹 MOCK 模式
**用途**: 框架开发和测试  
**特点**: 
- 无需任何硬件
- 生成随机 logits
- 模拟延迟
- Mac/Linux 都支持

**使用**:
```bash
# 默认 (Mac 上)
./your_program

# 或显式指定
PTO2_MODE=mock ./your_program
```

---

### 🔸 SIMULATOR 模式
**用途**: 算子开发和验证  
**特点**:
- 无需 NPU 硬件
- CPU 模拟执行
- 真实算子逻辑
- 需要 g++-15 (Linux)

**使用**:
```bash
PTO2_MODE=simulator ./your_program
```

**状态**: ⚠️ 框架就绪,实际执行待实现

---

### 🔺 DEVICE 模式
**用途**: 生产部署  
**特点**:
- 需要 Ascend NPU
- 真实硬件执行
- 最高性能
- 仅 Linux + NPU

**使用**:
```bash
PTO2_MODE=device ./your_program
```

**状态**: ⚠️ 框架就绪,实际执行待实现

---

## 快速对比

| 特性 | MOCK | SIMULATOR | DEVICE |
|------|------|-----------|--------|
| 硬件需求 | 无 | 无 | Ascend NPU |
| 操作系统 | Mac/Linux | Linux | Linux |
| 编译器 | 任意 C++ | g++-15 | ccec |
| 算子逻辑 | ❌ 随机 | ✅ 真实 | ✅ 真实 |
| 执行速度 | 快 | 慢 | 最快 |
| 调试难度 | 容易 | 容易 | 困难 |
| 当前状态 | ✅ 完成 | ⚠️ 框架就绪 | ⚠️ 框架就绪 |

---

## 代码示例

### 检查当前模式
```cpp
#include "engine/engine.h"
#include "pto2/pto2_config.h"

auto engine = std::make_shared<Engine>(
    model_config, cache_config, scheduler_config,
    ModelRunnerType::PTO2);

// 方式 1: 简单检查
if (engine->is_using_pto2_device()) {
    std::cout << "Using real device" << std::endl;
} else {
    std::cout << "Using simulator/mock" << std::endl;
}

// 方式 2: 详细检查 (需要访问 runtime)
// TODO: 添加 get_execution_mode() 到 Engine
```

### 强制指定模式
```cpp
// 在初始化 PTO2Runtime 时
pto2::RuntimeConfig runtime_config;
pto2::PTO2Config pto2_config = pto2::PTO2Config::simulator();

runtime->init(runtime_config, pto2_config);
```

---

## 环境变量

### PTO2_MODE
```bash
# Mock 模式 (默认 Mac)
export PTO2_MODE=mock

# Simulator 模式 (默认 Linux)
export PTO2_MODE=simulator

# Device 模式
export PTO2_MODE=device

# 取消设置 (使用自动检测)
unset PTO2_MODE
```

---

## 开发流程

### 1. 框架开发 (Mac)
```bash
# 使用 MOCK 模式
./mock_llama
./simple_generate
```

### 2. 算子开发 (Linux)
```bash
# 使用 SIMULATOR 模式
PTO2_MODE=simulator ./simulator_demo
```

### 3. 性能优化 (Linux + NPU)
```bash
# 使用 DEVICE 模式
PTO2_MODE=device ./simulator_demo
```

---

## 下一步

### Simulator 实现 (Linux 环境)
1. 安装 g++-15
2. 集成 simpler RuntimeBuilder
3. 编译 simulator kernels
4. 实现 launch() 和 wait()
5. 验证简单算子 (vector add)

### Device 实现 (Linux + NPU 环境)
1. 检测 Ascend NPU
2. 使用 ccec 编译 kernels
3. 调用真实 device API
4. 端到端验证

---

## 常见问题

### Q: 为什么 Mac 上不能用 simulator?
A: simpler 的 simulator 需要 g++-15,Mac 上通常没有。可以在 Linux 上使用。

### Q: 如何切换到 simulator 模式?
A: 设置环境变量 `PTO2_MODE=simulator`。

### Q: Simulator 和 Mock 有什么区别?
A: 
- Mock: 生成随机结果,无真实计算
- Simulator: CPU 模拟真实算子逻辑

### Q: 当前 simulator 能用吗?
A: 框架已就绪,但实际执行逻辑待实现。当前会 fallback 到 mock。

---

## 相关文档

- [SIMULATOR_SETUP.md](SIMULATOR_SETUP.md) - 详细配置指南
- [PHASE1_SIMULATOR_READY.md](PHASE1_SIMULATOR_READY.md) - 完成报告
- [README.md](README.md) - 项目说明
