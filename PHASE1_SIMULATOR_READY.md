# Phase 1 完成报告 (Simulator Ready)

**日期**: 2026-03-03  
**状态**: ✅ 完成 + Simulator 框架就绪

---

## 更新内容

在 Phase 1 基础上,新增了 **Simulator 模式支持**。

---

## 新增组件

### 1. PTO2Config 系统 (`src/pto2/pto2_config.h/cpp`)

#### ExecutionMode 枚举
```cpp
enum class ExecutionMode {
    MOCK,       // 纯 Mock (无 PTO2)
    SIMULATOR,  // CPU 模拟器 (无需硬件)
    DEVICE      // 真实 Ascend NPU
};
```

#### PTO2Config 类
- ✅ `auto_detect()` - 自动检测最佳模式
- ✅ `simulator()` - 创建 simulator 配置
- ✅ `device()` - 创建 device 配置
- ✅ `mock()` - 创建 mock 配置

#### 环境变量支持
```bash
PTO2_MODE=mock       ./your_program
PTO2_MODE=simulator  ./your_program
PTO2_MODE=device     ./your_program
```

### 2. PTO2Runtime 更新

#### 模式初始化
```cpp
bool init(const RuntimeConfig& config,
          const PTO2Config& pto2_config = PTO2Config::auto_detect());
```

#### 模式查询
```cpp
ExecutionMode get_execution_mode() const;
bool is_device_available() const;
bool is_simulator() const;
bool is_mock() const;
```

### 3. 示例程序

#### simulator_demo
- 显示当前执行模式
- 支持环境变量切换
- 演示三种模式的使用

---

## 自动检测逻辑

```
PTO2Config::auto_detect()
    │
    ├─> 检查环境变量 PTO2_MODE
    │   ├─> "simulator" → SIMULATOR 模式
    │   ├─> "device"    → DEVICE 模式
    │   └─> "mock"      → MOCK 模式
    │
    └─> 平台自动检测
        ├─> Mac     → MOCK (simulator 需要 g++-15)
        └─> Linux   → SIMULATOR (默认)
```

---

## 使用示例

### 1. 默认模式 (自动检测)
```cpp
auto engine = std::make_shared<Engine>(
    model_config, cache_config, scheduler_config,
    ModelRunnerType::PTO2);
// Mac: 使用 MOCK
// Linux: 使用 SIMULATOR
```

### 2. 环境变量控制
```bash
# Mock 模式
./simulator_demo

# Simulator 模式
PTO2_MODE=simulator ./simulator_demo

# Device 模式
PTO2_MODE=device ./simulator_demo
```

### 3. 代码中指定
```cpp
// 在 PTO2Runtime 初始化时传递配置
pto2::PTO2Config config = pto2::PTO2Config::simulator();
runtime->init(runtime_config, config);
```

---

## 测试结果

### 编译
```bash
cd build && cmake .. && make -j8
[100%] Built target pypto_core
[100%] Built target simulator_demo
✅ 编译成功
```

### 单元测试
```
[==========] Running 27 tests from 3 test suites.
[  PASSED  ] 27 tests.
✅ 全部通过
```

### 集成测试
```
[==========] Running 8 tests from 1 test suite.
[  PASSED  ] 8 tests.
✅ 全部通过
```

### Simulator Demo
```bash
# 默认模式 (Mac)
./simulator_demo
[INFO] Mac detected, using mock mode
✅ 运行成功

# Simulator 模式
PTO2_MODE=simulator ./simulator_demo
[INFO] PTO2_MODE=simulator, using simulator mode
[INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
[WARN] Simulator not yet implemented, using mock
✅ 运行成功 (fallback to mock)
```

---

## 代码统计

### 新增文件
- `src/pto2/pto2_config.h` (55 行)
- `src/pto2/pto2_config.cpp` (40 行)
- `examples/simulator_demo/simulator_demo.cpp` (85 行)

### 修改文件
- `src/pto2/pto2_runtime.h` (+4 个查询方法)
- `src/pto2/pto2_runtime.cpp` (重构 init 和执行逻辑)
- `CMakeLists.txt` (+2 行)

**总计**: +180 行新代码

---

## Simulator 集成计划

### 当前状态 ✅
- 配置系统完成
- 模式切换机制完成
- 环境变量支持完成
- 编译和测试通过

### 下一步 (Simulator 实际执行)

#### 1. 集成 simpler RuntimeBuilder
```cpp
// 在 PTO2Runtime::init() 中
#include "simpler/python/runtime_builder.h"  // 需要 C++ 绑定

if (mode == ExecutionMode::SIMULATOR) {
    // 使用 simpler 编译 simulator kernels
    // platform = "a2a3sim"
}
```

#### 2. 编译 Simulator Kernels
```cpp
// 使用 Gxx15Toolchain 编译
// 添加 -D__CPU_SIM 标志
// 生成 .so 文件
```

#### 3. 加载和执行
```cpp
// dlopen() 加载 orchestration .so
// 调用 build_graph_func()
// 调用 launch_runtime()
```

#### 4. 实现简单算子验证
- Vector add kernel
- 在 simulator 中运行
- 验证结果正确性

---

## Simulator vs Device vs Mock

| 特性 | MOCK | SIMULATOR | DEVICE |
|------|------|-----------|--------|
| 硬件需求 | 无 | 无 | Ascend NPU |
| 编译器 | 任意 C++ | g++-15 | ccec |
| 平台标志 | - | `__CPU_SIM` | - |
| 执行位置 | Host | CPU (模拟) | NPU |
| 速度 | 快 (无计算) | 慢 (CPU) | 快 (硬件) |
| 调试 | 容易 | 容易 | 困难 |
| 用途 | 框架开发 | 算子验证 | 生产部署 |
| Mac 支持 | ✅ | ❌ (需 g++-15) | ❌ |
| Linux 支持 | ✅ | ✅ | ✅ (有硬件) |

---

## 环境变量

### PTO2_MODE
控制执行模式:
```bash
export PTO2_MODE=mock        # 强制 Mock 模式
export PTO2_MODE=simulator   # 强制 Simulator 模式
export PTO2_MODE=device      # 强制 Device 模式
```

### 自动检测规则
- **未设置** `PTO2_MODE`:
  - Mac → MOCK
  - Linux → SIMULATOR
- **设置了** `PTO2_MODE`: 使用指定模式

---

## 下一步行动

### 立即可做 (在 Linux 环境)
1. **安装 g++-15**
   ```bash
   # Ubuntu/Debian
   sudo apt install g++-15
   ```

2. **测试 simpler 的 simulator**
   ```bash
   cd /path/to/simpler
   python examples/scripts/run_example.py \
       --kernels examples/host_build_graph/vector_example/kernels \
       --golden examples/host_build_graph/vector_example/golden.py \
       --platform a2a3sim
   ```

3. **集成到 pypto-serving**
   - 在 `PTO2Runtime::init()` 中调用 simpler 的编译流程
   - 加载生成的 .so 文件
   - 实现 `launch()` 和 `wait()`

### 需要实现的算子
1. **Vector Add** (验证 simulator 工作)
2. **Matrix Multiply** (GEMM)
3. **Paged Attention** (复用 simpler 的实现)
4. **RMSNorm** (组合算子)
5. **RoPE** (组合算子)

---

## 关键文件

### 配置和模式
- `src/pto2/pto2_config.h` - 执行模式配置
- `src/pto2/pto2_config.cpp` - 自动检测逻辑

### Runtime
- `src/pto2/pto2_runtime.h` - Runtime 接口
- `src/pto2/pto2_runtime.cpp` - 模式分发逻辑

### 示例
- `examples/simulator_demo/simulator_demo.cpp` - Simulator 演示

### 文档
- `SIMULATOR_SETUP.md` - Simulator 配置指南

---

## 验证清单

✅ **编译**
- Mac 上编译成功
- 所有目标构建成功

✅ **测试**
- 27 个单元测试通过
- 8 个集成测试通过

✅ **模式切换**
- MOCK 模式正常
- SIMULATOR 模式识别正确 (fallback to mock)
- DEVICE 模式识别正确 (fallback to mock)

✅ **环境变量**
- `PTO2_MODE=mock` 正常
- `PTO2_MODE=simulator` 正常
- `PTO2_MODE=device` 正常

✅ **示例程序**
- mock_llama 正常
- simple_generate 正常
- pto2_demo 正常
- simulator_demo 正常 ⭐ NEW

---

## 总结

Phase 1 已完全完成,并新增了 **Simulator 模式框架**:

1. ✅ **三种执行模式** - MOCK / SIMULATOR / DEVICE
2. ✅ **自动检测** - 根据平台选择最佳模式
3. ✅ **环境变量控制** - `PTO2_MODE` 灵活切换
4. ✅ **优雅降级** - Simulator 未实现时 fallback to mock
5. ✅ **编译通过** - Mac 上完全可编译
6. ✅ **所有测试通过** - 35 个测试全部通过

**下一步**: 在 Linux 环境中实现 Simulator 的实际执行逻辑,集成 simpler 的 a2a3sim 平台。

**关键成果**: 代码已经为 Simulator 集成做好了完整的准备,只需要在 Linux 上添加实际的 simpler 调用即可!
