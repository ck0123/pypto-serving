# 👋 新 Agent 从这里开始

欢迎接手 pypto-serving 项目!

---

## 🎯 这是什么项目?

一个**纯 C++** 实现的高性能 LLM 推理引擎,基于 PTO2 runtime。

**关键特性**:
- Radix Tree KV Cache (SGLang 风格)
- Paged Attention (vLLM 风格)
- 三种执行模式 (Mock/Simulator/Device)

---

## 📊 当前进度

```
Phase 0: Mock 框架        ✅ 完成
Phase 1: PTO2 集成        ✅ 完成
Phase 2: Simulator 执行   ⚠️  框架就绪,待实现
Phase 3: LLaMA 算子       ⚠️  待实现
```

---

## 🚀 快速开始

### 方式 1: 一键启动 (推荐)
```bash
./quick_start.sh
```

这会自动完成:
1. 安装依赖 (simpler, PTOAS, pypto)
2. 配置环境
3. 编译项目
4. 运行测试
5. 显示下一步

### 方式 2: 手动步骤 (3 步)

#### 1️⃣ 安装依赖
```bash
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh
```

#### 2️⃣ 编译和验证
```bash
./verify.sh
```

如果看到 `✅ 环境验证完成!`,说明一切正常。

#### 3️⃣ 阅读交接文档
```bash
cat AGENT_HANDOFF.md
```

这是**最重要**的文档,包含:
- 项目背景和目标
- 当前状态和进度
- 下一步任务
- 关键代码位置
- 开发建议

#### 4️⃣ 运行示例
```bash
cd build
./mock_llama        # 了解框架功能
./simulator_demo    # 了解执行模式
```

---

## 📚 文档导航

### 必读 (按顺序)
1. **[AGENT_HANDOFF.md](AGENT_HANDOFF.md)** ⭐⭐⭐ - Agent 交接文档
2. **[MIGRATION_CHECKLIST.md](MIGRATION_CHECKLIST.md)** ⭐⭐ - 迁移检查清单
3. **[SIMULATOR_SETUP.md](SIMULATOR_SETUP.md)** ⭐⭐ - Simulator 配置指南

### 参考
- [README.md](README.md) - 项目概览
- [QUICKSTART.md](QUICKSTART.md) - 快速开始
- [EXECUTION_MODES.md](EXECUTION_MODES.md) - 执行模式参考
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - 完整迁移指南

### 进度报告
- [PHASE0_COMPLETE.md](PHASE0_COMPLETE.md) - Phase 0 完成报告
- [PHASE1_SIMULATOR_READY.md](PHASE1_SIMULATOR_READY.md) - Phase 1 完成报告

---

## 🎯 下一步任务

### 立即任务: 实现 Simulator 执行

**目标**: 让 `PTO2_MODE=simulator` 真正运行算子,而不是 fallback 到 mock。

**位置**: `src/pto2/pto2_runtime.cpp`

**步骤**:
1. 验证 simpler simulator 可用
2. 创建第一个 kernel (vector add)
3. 实现编译逻辑 (`setup_simulator()`)
4. 实现执行逻辑 (`launch()`)
5. 验证结果

**详细指南**: 见 `SIMULATOR_SETUP.md` 的 "Phase 2: Simulator 集成" 部分。

---

## 🔍 关键代码

### 需要实现的文件
1. `src/pto2/pto2_runtime.cpp` - **重点**: 实现 simulator 执行
2. `src/engine/model_runner_pto2.cpp` - **重点**: 实现真实推理

### 参考实现
1. `simpler/examples/host_build_graph/vector_example/` - 简单示例
2. `simpler/examples/host_build_graph/paged_attention/` - Paged Attention

---

## ✅ 成功标志

当你看到以下输出时,说明 simulator 集成成功:

```bash
PTO2_MODE=simulator ./simulator_demo

# 输出:
[INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
[INFO] Compiling simulator kernels...
[INFO] Loading orchestration .so...
[INFO] Launching runtime...
[INFO] Execution complete
Generated tokens: [真实计算的结果]
```

而不是:
```bash
[WARN] Simulator not yet implemented, using mock
```

---

## 💡 开发建议

1. **增量开发**: 先实现简单算子 (vector add),再实现复杂算子 (LLaMA)
2. **测试驱动**: 每实现一个算子,立即写单元测试
3. **对比验证**: 使用 simpler 的 golden.py 作为参考

---

## 🐛 遇到问题?

### 编译失败
查看 `MIGRATION_GUIDE.md` 的 "故障排查" 部分。

### 不理解某个概念
查看 `AGENT_HANDOFF.md` 的 "背景知识" 部分。

### 不知道从哪里开始
查看 `AGENT_HANDOFF.md` 的 "开始工作" 部分。

---

## 📞 需要更多信息?

### 查看日志
```bash
cat build/cmake.log
cat build/make.log
cat build/unit_test.log
```

### 调试代码
```bash
gdb build/simulator_demo
```

### 查看符号
```bash
nm -C build/libpypto_core.a | grep PTO2Runtime
```

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

## 📦 项目结构

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
└── docs/               # 10 个文档
```

---

## 🎬 开始吧!

```bash
# 1. 验证环境
./verify.sh

# 2. 阅读交接文档
cat AGENT_HANDOFF.md

# 3. 运行示例
cd build && ./simulator_demo

# 4. 开始开发
vim src/pto2/pto2_runtime.cpp
```

祝开发顺利! 🚀

---

## 📝 快速链接

- [AGENT_HANDOFF.md](AGENT_HANDOFF.md) - **最重要的文档**
- [MIGRATION_CHECKLIST.md](MIGRATION_CHECKLIST.md) - 迁移检查清单
- [SIMULATOR_SETUP.md](SIMULATOR_SETUP.md) - Simulator 配置
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - 完整迁移指南
