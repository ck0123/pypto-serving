# 文档索引

pypto-serving 项目的所有文档导航。

---

## 🎯 我想...

### 快速开始
- 👉 **[START_HERE.md](START_HERE.md)** - 新 Agent 从这里开始
- 👉 **[QUICKSTART.md](QUICKSTART.md)** - 5 分钟快速开始

### 迁移到新机器
- 👉 **[MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)** - 完整迁移指南 (16K)
- 👉 **[MIGRATION_CHECKLIST.md](MIGRATION_CHECKLIST.md)** - 迁移检查清单 (5K)
- 👉 **[verify.sh](verify.sh)** - 自动验证脚本

### 交接给新 Agent
- 👉 **[AGENT_HANDOFF.md](AGENT_HANDOFF.md)** - Agent 交接文档 (12K) ⭐⭐⭐

### 了解项目
- 👉 **[README.md](README.md)** - 项目概览
- 👉 **[design goal.md](design%20goal.md)** - 设计目标

### 配置 Simulator
- 👉 **[SIMULATOR_SETUP.md](SIMULATOR_SETUP.md)** - Simulator 配置指南
- 👉 **[EXECUTION_MODES.md](EXECUTION_MODES.md)** - 执行模式参考

### 查看进度
- 👉 **[PHASE0_COMPLETE.md](PHASE0_COMPLETE.md)** - Phase 0 完成报告
- 👉 **[PHASE1_SIMULATOR_READY.md](PHASE1_SIMULATOR_READY.md)** - Phase 1 完成报告

### 理解设计
- 👉 **[implementation_plan_v2.md](implementation_plan_v2.md)** - 完整实现计划
- 👉 **[reference_sglang_vllm.md](reference_sglang_vllm.md)** - 参考实现分析

---

## 📁 按类型分类

### 🚀 入门文档 (新手必读)
| 文档 | 大小 | 用途 |
|------|------|------|
| [START_HERE.md](START_HERE.md) | 5K | 新 Agent 入口 ⭐⭐⭐ |
| [AGENT_HANDOFF.md](AGENT_HANDOFF.md) | 12K | Agent 交接 ⭐⭐⭐ |
| [QUICKSTART.md](QUICKSTART.md) | 5K | 快速开始 |
| [README.md](README.md) | 4K | 项目概览 |

### 🔧 操作指南
| 文档 | 大小 | 用途 |
|------|------|------|
| [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) | 16K | 完整迁移指南 ⭐⭐ |
| [MIGRATION_CHECKLIST.md](MIGRATION_CHECKLIST.md) | 5K | 迁移检查清单 ⭐⭐ |
| [SIMULATOR_SETUP.md](SIMULATOR_SETUP.md) | - | Simulator 配置 ⭐⭐ |
| [verify.sh](verify.sh) | 3K | 自动验证脚本 |

### 📊 进度报告
| 文档 | 大小 | 用途 |
|------|------|------|
| [PHASE0_COMPLETE.md](PHASE0_COMPLETE.md) | - | Phase 0 完成 |
| [PHASE1_SIMULATOR_READY.md](PHASE1_SIMULATOR_READY.md) | - | Phase 1 完成 |

### 📐 设计文档
| 文档 | 大小 | 用途 |
|------|------|------|
| [implementation_plan_v2.md](implementation_plan_v2.md) | - | 完整实现计划 |
| [design goal.md](design%20goal.md) | - | 设计目标 |
| [reference_sglang_vllm.md](reference_sglang_vllm.md) | - | 参考实现分析 |

### 🔍 参考文档
| 文档 | 大小 | 用途 |
|------|------|------|
| [EXECUTION_MODES.md](EXECUTION_MODES.md) | - | 执行模式参考 |
| [DOCS_INDEX.md](DOCS_INDEX.md) | - | 本文档 |

---

## 🎓 学习路径

### 路径 1: 新 Agent 接手项目
```
1. START_HERE.md          (了解项目和当前状态)
   ↓
2. AGENT_HANDOFF.md       (详细交接信息)
   ↓
3. MIGRATION_CHECKLIST.md (验证环境)
   ↓
4. SIMULATOR_SETUP.md     (开始开发)
```

### 路径 2: 迁移到新机器
```
1. MIGRATION_GUIDE.md     (完整迁移指南)
   ↓
2. MIGRATION_CHECKLIST.md (逐项检查)
   ↓
3. verify.sh              (自动验证)
   ↓
4. START_HERE.md          (开始开发)
```

### 路径 3: 理解项目设计
```
1. README.md              (项目概览)
   ↓
2. design goal.md         (设计目标)
   ↓
3. implementation_plan_v2.md (实现计划)
   ↓
4. reference_sglang_vllm.md (参考实现)
```

### 路径 4: 查看进度和下一步
```
1. PHASE0_COMPLETE.md     (Phase 0 状态)
   ↓
2. PHASE1_SIMULATOR_READY.md (Phase 1 状态)
   ↓
3. SIMULATOR_SETUP.md     (Phase 2 任务)
```

---

## 🔍 按场景查找

### 场景: 我是新 Agent,第一次接触这个项目
**阅读顺序**:
1. START_HERE.md (5 分钟)
2. AGENT_HANDOFF.md (15 分钟)
3. 运行 `./verify.sh` (2 分钟)
4. 运行示例 `cd build && ./simulator_demo` (1 分钟)
5. 阅读 SIMULATOR_SETUP.md (10 分钟)

**总耗时**: ~30 分钟

### 场景: 我需要把代码迁移到新机器
**阅读顺序**:
1. MIGRATION_GUIDE.md (10 分钟)
2. 执行迁移操作 (10-30 分钟)
3. MIGRATION_CHECKLIST.md (逐项检查,10 分钟)
4. 运行 `./verify.sh` (2 分钟)

**总耗时**: ~30-60 分钟

### 场景: 我想实现 Simulator 执行
**阅读顺序**:
1. SIMULATOR_SETUP.md (重点阅读 Phase 2 部分)
2. EXECUTION_MODES.md (了解执行模式)
3. 查看代码: `src/pto2/pto2_runtime.cpp`
4. 参考: `simpler/examples/host_build_graph/vector_example/`

### 场景: 我想理解整体架构
**阅读顺序**:
1. README.md
2. design goal.md
3. implementation_plan_v2.md
4. reference_sglang_vllm.md

### 场景: 我想知道当前进度和下一步
**阅读顺序**:
1. PHASE1_SIMULATOR_READY.md (最新状态)
2. SIMULATOR_SETUP.md (下一步任务)
3. AGENT_HANDOFF.md (详细任务说明)

---

## 📊 文档统计

- **总文档数**: 14 个
- **入门文档**: 4 个
- **操作指南**: 4 个
- **进度报告**: 2 个
- **设计文档**: 3 个
- **参考文档**: 2 个

---

## 🎯 推荐阅读顺序 (首次接触)

### 最小阅读集 (必读)
1. **START_HERE.md** - 快速了解项目
2. **AGENT_HANDOFF.md** - 详细交接信息
3. **SIMULATOR_SETUP.md** - 下一步任务

**总耗时**: ~30 分钟

### 完整阅读集 (推荐)
1. START_HERE.md
2. AGENT_HANDOFF.md
3. README.md
4. PHASE1_SIMULATOR_READY.md
5. SIMULATOR_SETUP.md
6. EXECUTION_MODES.md
7. implementation_plan_v2.md

**总耗时**: ~60 分钟

---

## 🔗 外部参考

### 参考项目
- **nano-vllm**: `/path/to/nano-vllm/` - Scheduler, BlockManager 设计
- **SGLang**: `/path/to/sglang/` - RadixCache, prefix matching
- **simpler**: `/path/to/simpler/` - PTO2 runtime, orchestration, kernels

### 知识文档 (如果有访问权限)
- `knowledge/inbox/2026-03-03-sglang-RadixAttention调用逻辑分析.md`
- `knowledge/inbox/2026-03-03-simpler-PTO2构图与算子执行流程分析.md`
- `knowledge/inbox/2026-03-03-PTO算子库与LLM推理算子需求分析.md`

---

## 📝 文档维护

### 添加新文档时
1. 在本文档中添加链接
2. 更新 README.md 的文档列表
3. 更新相关文档的交叉引用

### 文档命名规范
- 全大写: 重要文档 (README.md, QUICKSTART.md)
- 下划线分隔: 多词文档 (AGENT_HANDOFF.md)
- 小写: 普通文档 (design goal.md)

---

## ❓ 找不到想要的信息?

### 查看代码注释
```bash
# 查看头文件注释
cat src/pto2/pto2_runtime.h
cat src/engine/engine.h
```

### 查看测试用例
```bash
# 测试用例是很好的使用示例
cat tests/unit/test_scheduler.cpp
cat tests/integration/test_engine_integration.cpp
```

### 查看示例程序
```bash
# 示例程序展示了完整的使用流程
cat examples/simulator_demo/simulator_demo.cpp
cat examples/mock_llama/mock_llama.cpp
```

---

## 🎉 开始吧!

从 [START_HERE.md](START_HERE.md) 开始你的旅程!
