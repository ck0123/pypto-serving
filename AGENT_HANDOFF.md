# Agent 交接文档

**交接日期**: 2026-03-03  
**项目**: pypto-serving (C++ LLM 推理引擎)

---

## 🎯 项目目标

实现一个**纯 C++** 的高性能 LLM 推理引擎,基于 PTO2 runtime,支持:
- Radix Tree KV Cache (SGLang 风格)
- Paged Attention (vLLM 风格)
- 多种执行模式 (Mock/Simulator/Device)

---

## 📊 当前进度

### ✅ 已完成

#### Phase 0: Mock 框架 (100%)
- Sequence 管理
- BlockManager (hash-based prefix caching)
- Scheduler (prefill/decode 调度)
- Sampler (greedy/temperature/top-k/top-p)
- Engine (主控制器)
- TestPath (异步测试接口)
- **27 个单元测试** + **8 个集成测试**

#### Phase 1: PTO2 集成 (100%)
- PTO2 类型定义
- PTO2 Runtime 封装
- PTO2 Model Runner
- 三种执行模式 (MOCK/SIMULATOR/DEVICE)
- 设备检测和自动 fallback
- 环境变量控制 (`PTO2_MODE`)

### ⚠️ 待实现

#### Phase 2: Simulator 执行 (0%)
- [ ] 集成 simpler RuntimeBuilder
- [ ] 编译 simulator kernels
- [ ] 实现 launch() 和 wait()
- [ ] 验证简单算子 (vector add)

#### Phase 3: LLaMA 算子 (0%)
- [ ] Embedding
- [ ] RMSNorm
- [ ] RoPE
- [ ] Attention (复用 PTO Paged Attention)
- [ ] MLP (复用 PTO GEMM)

---

## 🗂️ 项目结构

```
pypto-serving/
├── src/
│   ├── common/         # 基础类型、配置、日志
│   ├── engine/         # Engine、Scheduler、Sequence、ModelRunner
│   ├── radix/          # BlockManager、KV Pool
│   ├── sampling/       # Sampler
│   ├── frontend/       # TestPath
│   └── pto2/           # PTO2 集成 ⭐ 重点
│
├── tests/
│   ├── unit/           # 27 个单元测试
│   └── integration/    # 8 个集成测试
│
├── examples/           # 4 个示例程序
│
└── docs/               # 8 个文档
```

---

## 📚 必读文档 (优先级排序)

### 1. 快速上手
- **README.md** - 项目概览和快速开始
- **QUICKSTART.md** - 构建和运行指南

### 2. 当前状态
- **PHASE1_SIMULATOR_READY.md** - 最新进度报告 ⭐
- **EXECUTION_MODES.md** - 执行模式参考 ⭐

### 3. 开发指南
- **SIMULATOR_SETUP.md** - Simulator 配置详解 ⭐
- **MIGRATION_GUIDE.md** - 环境迁移指南

### 4. 设计文档
- **implementation_plan_v2.md** - 完整实现计划
- **design goal.md** - 设计目标

---

## 🔧 快速开始

### 1. 验证环境
```bash
cd pypto-serving
./verify.sh
```

### 2. 安装依赖
```bash
# 安装 PTO2 依赖 (simpler, PTOAS, pypto)
./setup_dependencies.sh

# 设置环境变量
source pypto_workspace/../env_setup.sh
```

### 3. 编译项目
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
make -j$(nproc)
```

### 4. 运行测试
```bash
./pypto_unit_tests          # 应该看到 [  PASSED  ] 27 tests.
./pypto_integration_tests   # 应该看到 [  PASSED  ] 8 tests.
```

### 5. 运行示例
```bash
./simulator_demo            # 查看当前执行模式
PTO2_MODE=simulator ./simulator_demo  # 测试 simulator 模式
```

---

## 🎯 下一步任务

### 立即任务: Simulator 执行实现

#### 目标
让 `PTO2_MODE=simulator` 模式真正运行算子,而不是 fallback 到 mock。

#### 位置
`src/pto2/pto2_runtime.cpp` 中的 `init()` 和 `launch()` 函数。

#### 步骤
1. **验证 simpler simulator 可用**
   ```bash
   cd /path/to/simpler
   python examples/scripts/run_example.py \
       --kernels examples/host_build_graph/vector_example/kernels \
       --golden examples/host_build_graph/vector_example/golden.py \
       --platform a2a3sim
   ```

2. **创建第一个 kernel** (vector add)
   ```bash
   mkdir -p src/kernels/vector_add/{orchestration,aic,aiv}
   # 参考 simpler/examples/host_build_graph/vector_example/
   ```

3. **实现编译逻辑**
   在 `PTO2Runtime::init()` 中:
   - 调用 simpler 的 RuntimeBuilder
   - 编译 orchestration + kernels
   - 加载 .so 文件

4. **实现执行逻辑**
   在 `PTO2Runtime::launch()` 中:
   - 调用 orchestration 函数构图
   - 启动 runtime 执行
   - 等待完成

5. **验证**
   ```bash
   PTO2_MODE=simulator ./simulator_demo
   # 应该看到真实的算子执行,而不是 mock
   ```

---

## 🔍 关键代码位置

### 需要实现的函数

#### 1. `src/pto2/pto2_runtime.cpp::init()`
```cpp
// 当前状态:
case ExecutionMode::SIMULATOR:
    LOG_INFO("PTO2Runtime: Simulator mode");
    LOG_WARNING("Simulator not yet implemented, using mock");
    setup_mock_host_api();
    break;

// 需要改成:
case ExecutionMode::SIMULATOR:
    LOG_INFO("PTO2Runtime: Simulator mode");
    setup_simulator();  // 调用 simpler 编译和加载
    break;
```

#### 2. `src/pto2/pto2_runtime.cpp::launch()`
```cpp
// 当前状态:
bool PTO2Runtime::launch() {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        LOG_WARNING("Cannot launch in mock mode");
        return false;
    }
    
    // TODO: Call actual PTO2 launch
    last_error_ = "Real device/simulator launch not implemented";
    return false;
}

// 需要实现:
bool PTO2Runtime::launch() {
    if (pto2_config_.mode == ExecutionMode::SIMULATOR) {
        // 调用 orchestration 函数构图
        // 调用 launch_runtime()
        return true;
    }
    // ...
}
```

#### 3. `src/engine/model_runner_pto2.cpp::run_with_device()`
```cpp
// 当前状态:
std::vector<std::vector<float>> PTO2ModelRunner::run_with_device(...) {
    // TODO: Implement actual PTO2 device execution
    LOG_DEBUG("Device execution not yet implemented, using mock");
    return run_with_mock(seqs, is_prefill);
}

// 需要实现:
// 1. 准备输入数据
// 2. 调用 runtime_->launch()
// 3. 获取输出结果
// 4. 返回 logits
```

---

## 📖 参考资料

### simpler 示例
- `simpler/examples/host_build_graph/vector_example/` - 简单示例
- `simpler/examples/host_build_graph/paged_attention/` - Paged Attention 实现

### 关键文件
- `simpler/python/runtime_builder.py` - RuntimeBuilder 类
- `simpler/python/toolchain.py` - Gxx15Toolchain (simulator)
- `simpler/src/runtime/host_build_graph/runtime/runtime.h` - Runtime 接口

### 知识文档
如果有访问权限,查看:
- `knowledge/inbox/2026-03-03-simpler-PTO2构图与算子执行流程分析.md`
- `knowledge/inbox/2026-03-03-PTO算子库与LLM推理算子需求分析.md`

---

## 🐛 已知问题

### 1. Mac 上不支持 simulator
**原因**: 需要 g++-15,Mac 上通常没有  
**解决**: 在 Linux 上开发 simulator 模式

### 2. Simulator 执行未实现
**状态**: 框架就绪,但 `launch()` 返回 false  
**下一步**: 实现 `setup_simulator()` 和 `launch()`

### 3. 真实算子未实现
**状态**: 所有模式都 fallback 到 mock  
**下一步**: 实现 LLaMA 算子

---

## 💡 开发建议

### 1. 增量开发
不要一次实现所有算子,按顺序:
1. Vector add (验证 simulator 工作)
2. Matrix multiply (验证 GEMM)
3. Paged attention (复用 simpler 实现)
4. 完整 LLaMA layer

### 2. 测试驱动
每实现一个算子,立即写单元测试:
```cpp
// tests/unit/test_vector_add.cpp
TEST(VectorAddTest, BasicOperation) {
    // 测试 vector add 算子
}
```

### 3. 对比验证
使用 simpler 的 golden.py 作为参考:
```python
# 在 Python 中计算期望结果
def compute_golden(tensors, params):
    tensors["out"] = tensors["a"] + tensors["b"]

# 在 C++ 中验证结果
EXPECT_NEAR(output[i], expected[i], 1e-5);
```

---

## 🚀 成功标准

### Simulator 模式成功的标志
```bash
PTO2_MODE=simulator ./simulator_demo

# 应该看到:
[INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
[INFO] Compiling simulator kernels...
[INFO] Loading orchestration .so...
[INFO] Launching runtime...
[INFO] Execution complete
Generated tokens: [真实计算的结果]

# 而不是:
[WARN] Simulator not yet implemented, using mock
```

### 第一个算子成功的标志
```bash
# 运行 vector add 测试
./test_vector_add

# 应该看到:
[INFO] Running vector add on simulator
[INFO] Input: [1.0, 2.0, 3.0, 4.0]
[INFO] Output: [2.0, 4.0, 6.0, 8.0]
[INFO] Expected: [2.0, 4.0, 6.0, 8.0]
✅ Test passed!
```

---

## 📞 需要帮助时

### 查看日志
```bash
# 编译日志
cat build/cmake.log
cat build/make.log

# 测试日志
cat build/unit_test.log
cat build/integration_test.log

# 运行日志 (设置 DEBUG 级别)
# 在代码中: Logger::instance().set_level(LogLevel::DEBUG);
```

### 常用调试命令
```bash
# 查看符号
nm -C build/libpypto_core.a | grep PTO2Runtime

# 查看依赖
ldd build/simulator_demo

# 运行 gdb
gdb build/simulator_demo
```

---

## 📦 迁移打包清单

### 必须传输
- [ ] `pypto-serving/` 整个目录 (排除 build/)
- [ ] 本文档 (AGENT_HANDOFF.md)

### 推荐传输
- [ ] `simpler/` 仓库 (用于 simulator/device 模式)
- [ ] 知识文档 (如果有访问权限)

### 无需传输
- [ ] `build/` 目录 (重新编译)
- [ ] `.git/` (如果使用 git clone)

---

## 🎓 背景知识

### 必需
- C++17 编程
- CMake 构建系统
- 基本的 LLM 概念 (token, attention, KV cache)

### 有帮助
- vLLM/SGLang 架构
- PTO2 runtime 概念
- Ascend NPU 编程

### 可以边做边学
- Radix Tree 实现细节
- Paged Attention 算法
- PTO Tile 算子库

---

## 🔗 快速链接

### 关键文档
1. [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - 完整迁移指南
2. [SIMULATOR_SETUP.md](SIMULATOR_SETUP.md) - Simulator 配置
3. [EXECUTION_MODES.md](EXECUTION_MODES.md) - 执行模式参考

### 代码入口
1. `src/pto2/pto2_runtime.cpp` - **重点**: 需要实现 simulator
2. `src/engine/model_runner_pto2.cpp` - **重点**: 需要实现推理
3. `examples/simulator_demo/simulator_demo.cpp` - 测试入口

### 参考实现
1. `simpler/examples/host_build_graph/vector_example/` - 简单示例
2. `simpler/examples/host_build_graph/paged_attention/` - Paged Attention

---

## ✅ 验证新环境

在新机器上运行:
```bash
cd pypto-serving
./verify.sh
```

应该看到:
```
✅ 环境验证完成!
  - 编译: ✅
  - 测试: ✅ (35/35 通过)
  - 示例: ✅
  - 模式切换: ✅
```

---

## 💬 与前任 Agent 的对话摘要

### 用户需求
1. 实现 C++ 版本的 nano-vllm,基于 PTO2
2. 使用 SGLang 的 Radix Attention
3. 纯 C++,无 Python
4. Mock-First 开发策略

### 关键决策
1. **Mock-First**: 先验证框架,再接入真实算子
2. **三种模式**: MOCK/SIMULATOR/DEVICE 分离
3. **轻量级集成**: 不强制依赖 simpler 完整编译
4. **自动 fallback**: 优雅降级,保证可用性

### 技术挑战
1. ✅ C++ 架构设计 - 已解决
2. ✅ Prefix caching - 已实现 (hash-based)
3. ⚠️ Simulator 集成 - 框架就绪,待实现
4. ⚠️ 真实算子 - 待实现

---

## 🎬 开始工作

### 第一步: 熟悉代码
```bash
# 1. 阅读 README
cat README.md

# 2. 运行示例
cd build
./mock_llama        # 了解框架功能
./simulator_demo    # 了解执行模式

# 3. 阅读关键代码
vim src/pto2/pto2_runtime.cpp       # 重点
vim src/engine/model_runner_pto2.cpp # 重点
```

### 第二步: 验证 simpler
```bash
cd /path/to/simpler

# 运行 vector_example (simulator 模式)
python examples/scripts/run_example.py \
    --kernels examples/host_build_graph/vector_example/kernels \
    --golden examples/host_build_graph/vector_example/golden.py \
    --platform a2a3sim

# 如果成功,说明 simulator 环境正常
```

### 第三步: 开始集成
参考 `SIMULATOR_SETUP.md` 的 "Phase 2: Simulator 集成" 部分。

---

## 📝 开发日志

建议在开发过程中记录:
- 遇到的问题和解决方案
- 关键决策和原因
- 性能数据和优化点

可以在项目中创建 `DEVLOG.md`。

---

## 🎉 期望成果

### 短期目标 (1-2 周)
- [ ] Simulator 模式真正运行
- [ ] Vector add 算子验证通过
- [ ] Matrix multiply 算子验证通过

### 中期目标 (3-4 周)
- [ ] Paged Attention 集成
- [ ] 完整 LLaMA layer 实现
- [ ] 端到端推理验证

### 长期目标 (2-3 月)
- [ ] Device 模式实现
- [ ] 性能优化
- [ ] 生产部署

---

## 联系方式

如果需要澄清任何问题:
- 查看项目文档 (8 个 .md 文件)
- 查看代码注释
- 参考 simpler 示例

祝开发顺利! 🚀
