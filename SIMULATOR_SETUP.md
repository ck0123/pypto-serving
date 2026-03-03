# Simulator 配置指南

## 概述

pypto-serving 现在支持三种执行模式:

1. **MOCK** - 纯 Mock 实现(无 PTO2,用于开发和测试)
2. **SIMULATOR** - CPU 模拟器模式(无需硬件,使用 simpler 的 a2a3sim 平台)
3. **DEVICE** - 真实 Ascend NPU 设备

## 当前状态

### ✅ 已完成
- PTO2Config 配置系统
- ExecutionMode 枚举 (MOCK/SIMULATOR/DEVICE)
- 环境变量控制 (`PTO2_MODE`)
- 自动模式检测
- 编译通过,所有测试通过

### ⚠️ 待实现
- Simulator 的实际执行逻辑
- 与 simpler 的 a2a3sim 平台集成
- Simulator kernel 编译和加载

## 使用方式

### 方式 1: 环境变量控制

```bash
# Mock 模式 (默认)
./simulator_demo

# Simulator 模式
PTO2_MODE=simulator ./simulator_demo

# Device 模式
PTO2_MODE=device ./simulator_demo
```

### 方式 2: 代码中指定

```cpp
#include "pto2/pto2_config.h"

// Mock 模式
auto config = pto2::PTO2Config::mock();

// Simulator 模式
auto config = pto2::PTO2Config::simulator();

// Device 模式
auto config = pto2::PTO2Config::device();

// 传递给 PTO2Runtime
runtime->init(runtime_config, config);
```

### 方式 3: 自动检测

```cpp
// 自动检测最佳模式
auto config = pto2::PTO2Config::auto_detect();
```

**自动检测规则**:
- Mac: 使用 MOCK (simulator 暂不支持 Mac)
- Linux: 默认使用 SIMULATOR
- 可通过 `PTO2_MODE` 环境变量覆盖

## Simulator 实现计划

### Phase 1: 基础框架 ✅
- [x] PTO2Config 系统
- [x] ExecutionMode 枚举
- [x] 环境变量支持
- [x] 自动检测逻辑

### Phase 2: Simulator 集成 (待实现)

#### 2.1 编译 Simulator Kernels
```cpp
// 在 PTO2Runtime::init() 中
if (mode == ExecutionMode::SIMULATOR) {
    // 1. 使用 simpler 的 RuntimeBuilder
    RuntimeBuilder builder("a2a3sim");  // a2a3sim 平台
    
    // 2. 编译 orchestration + kernels
    auto [host_bin, aicpu_bin, aicore_bin] = builder.build("host_build_graph");
    
    // 3. 加载 shared library
    dlopen(host_bin, RTLD_NOW);
}
```

#### 2.2 运行 Simulator
```cpp
// 在 PTO2Runtime::launch() 中
if (mode == ExecutionMode::SIMULATOR) {
    // 1. 调用 orchestration 函数构图
    build_graph_func(runtime, args, arg_count);
    
    // 2. 启动 simulator 执行
    // simpler 的 simulator 是 CPU 模拟,直接调用即可
    launch_runtime(runtime);
}
```

### Phase 3: 端到端验证
- [ ] 实现简单的 vector add kernel
- [ ] 在 simulator 中运行
- [ ] 验证结果正确性
- [ ] 性能分析

## Simulator vs Device

| 特性 | Simulator | Device |
|------|-----------|--------|
| 硬件需求 | 无 | Ascend NPU |
| 编译器 | g++/clang++ | ccec |
| 平台 | a2a3sim | a2a3 |
| 执行速度 | 慢 (CPU模拟) | 快 (NPU硬件) |
| 调试 | 容易 (gdb等) | 困难 |
| 用途 | 开发/调试 | 生产部署 |

## simpler 的 Simulator 支持

simpler 已经支持 simulator:

### 编译器配置
```python
# python/toolchain.py
class Gxx15Toolchain(Toolchain):
    """g++-15 compiler for simulation kernels."""
    
    def get_compile_flags(self, **kwargs) -> List[str]:
        return [
            "-shared", "-O2", "-fPIC",
            "-std=c++23",
            "-D__CPU_SIM",  # Simulator 标志
            "-DPTO_CPU_MAX_THREADS=1",
            "-include", "pto_sim_types.h",  # PTO 类型 stub
        ]
```

### 平台选择
```python
# python/runtime_builder.py
class RuntimeBuilder:
    def __init__(self, platform: str = "a2a3"):
        """
        Args:
            platform: Target platform ("a2a3" or "a2a3sim")
        """
```

## 下一步

### 立即可做
1. **在 Linux 上测试** - Mac 不支持 g++-15
2. **实现简单 kernel** - vector add 作为验证
3. **集成 simpler RuntimeBuilder** - 编译 simulator kernels

### 需要准备
1. **Linux 环境** (Mac 上 g++-15 不可用)
2. **simpler 依赖** (Python, g++-15)
3. **测试 kernel** (简单的算子验证)

## 示例代码

### 运行 Simulator Demo
```bash
cd build

# 默认模式 (Mac 上是 mock)
./simulator_demo

# 强制 simulator 模式
PTO2_MODE=simulator ./simulator_demo

# 输出:
# [INFO] PTO2_MODE=simulator, using simulator mode
# [INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
# [WARN] Simulator not yet implemented, using mock
```

### 检查执行模式
```cpp
auto engine = std::make_shared<Engine>(
    model_config, cache_config, scheduler_config,
    ModelRunnerType::PTO2);

if (engine->is_using_pto2_device()) {
    std::cout << "Using real device" << std::endl;
} else {
    std::cout << "Using simulator/mock" << std::endl;
}
```

## 总结

✅ **已完成**: Simulator 模式的配置框架和接口  
⚠️ **待实现**: Simulator 的实际执行逻辑  
📝 **下一步**: 在 Linux 环境中集成 simpler 的 simulator 编译和执行

当前代码已经为 simulator 集成做好了准备,只需要在 `PTO2Runtime::init()` 和 `PTO2Runtime::launch()` 中添加实际的 simpler 调用即可。
