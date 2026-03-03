# pypto-serving 迁移和复现指南

**目标**: 在新机器上完整复现当前的开发环境和代码状态

---

## 1. 环境要求

### 最小要求 (开发模式)
- **操作系统**: Linux (推荐 Ubuntu 20.04+)
- **编译器**: 
  - g++ 或 clang++ (C++17 支持)
  - g++-15 (用于 simulator 模式)
- **CMake**: 3.20+
- **Python**: 3.8+ (用于 simpler 工具链)
- **Git**: 用于代码管理

### 完整要求 (生产模式)
- 上述所有 + Ascend NPU 硬件
- Ascend CANN 工具链 (ccec 编译器)

---

## 2. 代码迁移

### 方式 1: Git 仓库 (推荐)

#### 在当前机器 (Mac)
```bash
cd /Users/qiaolina/Code/oh-my-knowledge/code/pypto-serving

# 初始化 git (如果还没有)
git init
git add .
git commit -m "Phase 1 complete: PTO2 integration with simulator support"

# 推送到远程仓库
git remote add origin <your-repo-url>
git push -u origin main
```

#### 在新机器 (Linux)
```bash
# 克隆仓库
git clone <your-repo-url>
cd pypto-serving

# 检查状态
git log --oneline -5
git status
```

### 方式 2: 直接拷贝

#### 打包
```bash
cd /Users/qiaolina/Code/oh-my-knowledge/code
tar czf pypto-serving.tar.gz pypto-serving/
```

#### 传输
```bash
# 使用 scp
scp pypto-serving.tar.gz user@remote-machine:/path/to/destination/

# 或使用 rsync
rsync -avz pypto-serving/ user@remote-machine:/path/to/destination/pypto-serving/
```

#### 解压
```bash
cd /path/to/destination
tar xzf pypto-serving.tar.gz
cd pypto-serving
```

---

## 3. 依赖安装

### 3.1 基础工具 (Ubuntu/Debian)

```bash
# 更新包管理器
sudo apt update

# 安装基础编译工具
sudo apt install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl

# 安装 g++-15 (用于 simulator)
sudo apt install -y software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install -y g++-15

# 验证安装
g++ --version
g++-15 --version
cmake --version
```

### 3.2 Python 环境

```bash
# 安装 Python 3.8+
sudo apt install -y python3 python3-pip python3-venv

# 创建虚拟环境
python3 -m venv venv
source venv/bin/activate

# 安装 Python 依赖 (用于 simpler 工具链)
pip install torch numpy
```

### 3.3 PTO2 依赖 (用于 simulator/device 模式)

#### 方式 1: 使用自动安装脚本 (推荐)

```bash
cd pypto-serving

# 运行依赖安装脚本
./setup_dependencies.sh

# 这会自动下载和设置:
# - simpler (PTO2 runtime 和 kernels)
# - PTOAS (PTO 汇编器)
# - pypto (PyPTO 库)

# 设置环境变量
source pypto_workspace/../env_setup.sh
```

#### 方式 2: 手动安装

```bash
# 克隆 simpler 仓库
cd /path/to/your/workspace
git clone https://github.com/ChaoWao/simpler
cd simpler
git checkout eede5613f28f9fa2c1ac0b29b27fa6eacb2ef2db

# 设置环境变量
export SIMPLER_ROOT=$(pwd)

# 下载 PTOAS (根据架构选择)
# 参考 setup_dependencies.sh 中的 PTOAS 下载逻辑
```

---

## 4. 编译 pypto-serving

### 4.1 设置依赖 (首次)

```bash
cd /path/to/pypto-serving

# 运行依赖安装脚本
./setup_dependencies.sh

# 设置环境变量
source pypto_workspace/../env_setup.sh

# 验证环境变量
echo $SIMPLER_ROOT
echo $PTOAS_ROOT
```

### 4.2 首次编译

```bash
cd /path/to/pypto-serving

# 创建构建目录
mkdir build && cd build

# 配置 (Release 模式)
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON

# 编译 (使用所有 CPU 核心)
make -j$(nproc)

# 验证编译成功
ls -lh libpypto_core.a
ls -lh mock_llama simple_generate pto2_demo simulator_demo
```

### 4.2 运行测试

```bash
# 单元测试 (27 个)
./pypto_unit_tests

# 集成测试 (8 个)
./pypto_integration_tests

# 运行所有测试
ctest --output-on-failure

# 预期输出:
# [  PASSED  ] 27 tests. (单元测试)
# [  PASSED  ] 8 tests.  (集成测试)
```

### 4.3 运行示例

```bash
# Mock LLaMA 演示 (5 个 demo)
./mock_llama

# 简单生成示例
./simple_generate

# PTO2 演示
./pto2_demo

# Simulator 演示
./simulator_demo

# 测试不同模式
PTO2_MODE=mock ./simulator_demo
PTO2_MODE=simulator ./simulator_demo
PTO2_MODE=device ./simulator_demo
```

---

## 5. 验证清单

在新机器上完成以下验证:

### ✅ 编译验证
```bash
cd build
make clean
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
make -j$(nproc)

# 预期: 编译成功,无错误
```

### ✅ 测试验证
```bash
./pypto_unit_tests
# 预期: [  PASSED  ] 27 tests.

./pypto_integration_tests
# 预期: [  PASSED  ] 8 tests.
```

### ✅ 示例验证
```bash
./mock_llama
# 预期: 5 个 demo 全部运行成功

./simulator_demo
# 预期: 显示当前执行模式,生成 10 个 token
```

### ✅ 模式切换验证
```bash
PTO2_MODE=mock ./simulator_demo
# 预期: [INFO] PTO2Runtime: Mock mode

PTO2_MODE=simulator ./simulator_demo
# 预期: [INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
#       [WARN] Simulator not yet implemented, using mock

PTO2_MODE=device ./simulator_demo
# 预期: [INFO] PTO2Runtime: Device mode
```

---

## 6. 项目结构说明

```
pypto-serving/
├── CMakeLists.txt              # CMake 配置
├── README.md                   # 项目说明
├── QUICKSTART.md               # 快速开始
├── SIMULATOR_SETUP.md          # Simulator 配置
├── EXECUTION_MODES.md          # 执行模式参考
├── MIGRATION_GUIDE.md          # 本文档
├── PHASE0_COMPLETE.md          # Phase 0 报告
├── PHASE1_SIMULATOR_READY.md   # Phase 1 报告
├── implementation_plan_v2.md   # 实现计划
│
├── src/                        # 源代码
│   ├── common/                 # 通用组件
│   │   ├── types.h            # 基础类型定义
│   │   ├── config.h/cpp       # 配置管理
│   │   └── logger.h/cpp       # 日志系统
│   │
│   ├── engine/                 # 引擎核心
│   │   ├── sequence.h/cpp     # Sequence 管理
│   │   ├── scheduler.h/cpp    # 调度器
│   │   ├── engine.h/cpp       # 主引擎
│   │   ├── model_runner_mock.h/cpp    # Mock Runner
│   │   └── model_runner_pto2.h/cpp    # PTO2 Runner
│   │
│   ├── radix/                  # KV Cache 管理
│   │   ├── block_manager.h/cpp    # Block 分配
│   │   └── kv_pool_mock.h/cpp     # KV Pool Mock
│   │
│   ├── sampling/               # 采样器
│   │   └── sampler.h/cpp      # 多种采样策略
│   │
│   ├── frontend/               # 前端接口
│   │   └── test_path.h/cpp    # 测试接口
│   │
│   └── pto2/                   # PTO2 集成 ⭐
│       ├── pto2_types.h       # PTO2 类型定义
│       ├── pto2_config.h/cpp  # 执行模式配置
│       └── pto2_runtime.h/cpp # Runtime 封装
│
├── tests/                      # 测试
│   ├── unit/                   # 单元测试 (27 个)
│   │   ├── test_sequence.cpp
│   │   ├── test_block_manager.cpp
│   │   └── test_scheduler.cpp
│   │
│   └── integration/            # 集成测试 (8 个)
│       └── test_engine_integration.cpp
│
└── examples/                   # 示例程序
    ├── mock_llama/             # Mock LLaMA 演示
    ├── simple_generate/        # 简单生成
    ├── pto2_demo/              # PTO2 演示
    └── simulator_demo/         # Simulator 演示 ⭐
```

---

## 7. 关键配置文件

### CMakeLists.txt
- 定义编译目标和依赖
- GTest 自动下载
- 编译选项配置

### .gitignore
- build/ 目录
- 编译产物
- IDE 配置

---

## 8. 开发工作流

### 在新机器上继续开发

#### Step 1: 环境准备
```bash
# 安装依赖 (见第 3 节)
sudo apt install build-essential cmake g++-15 python3

# 克隆代码
git clone <repo-url>
cd pypto-serving
```

#### Step 2: 编译验证
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
make -j$(nproc)

# 运行测试
./pypto_unit_tests
./pypto_integration_tests
```

#### Step 3: 开始开发
```bash
# 修改代码
vim src/pto2/pto2_runtime.cpp

# 重新编译
cd build && make -j$(nproc)

# 运行测试
./pypto_unit_tests
```

---

## 9. Simulator 模式开发 (Linux 环境)

### 前置条件
- ✅ Linux 操作系统
- ✅ g++-15 已安装
- ✅ simpler 仓库已克隆

### 开发步骤

#### Step 1: 验证 simpler simulator
```bash
cd /path/to/simpler

# 测试 vector_example (simulator 模式)
python examples/scripts/run_example.py \
    --kernels examples/host_build_graph/vector_example/kernels \
    --golden examples/host_build_graph/vector_example/golden.py \
    --platform a2a3sim

# 预期: 编译成功,运行通过
```

#### Step 2: 集成到 pypto-serving
在 `src/pto2/pto2_runtime.cpp` 中实现:

```cpp
bool PTO2Runtime::init(const RuntimeConfig& config, const PTO2Config& pto2_config) {
    // ...
    
    if (pto2_config_.mode == ExecutionMode::SIMULATOR) {
        LOG_INFO("PTO2Runtime: Simulator mode");
        
        // TODO: 调用 simpler 的 RuntimeBuilder
        // 1. 编译 orchestration + kernels (a2a3sim 平台)
        // 2. 加载 .so 文件
        // 3. 获取函数指针
        
        setup_simulator();  // 待实现
    }
    
    // ...
}
```

#### Step 3: 实现简单算子
创建 `src/kernels/vector_add/` 作为第一个验证算子。

---

## 10. 故障排查

### 编译失败

#### 问题: GTest 下载失败
```bash
# 解决: 手动下载 GTest
cd build/_deps
git clone https://github.com/google/googletest.git googletest-src
cd googletest-src && git checkout v1.14.0
```

#### 问题: 找不到 g++-15
```bash
# 解决: 安装 g++-15
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install g++-15
```

#### 问题: CMake 版本太低
```bash
# 解决: 安装新版 CMake
wget https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-linux-x86_64.sh
sudo sh cmake-3.27.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

### 运行失败

#### 问题: 找不到 libgtest
```bash
# 解决: 使用静态链接 (已在 CMakeLists.txt 中配置)
# 重新编译即可
cd build && rm -rf * && cmake .. && make -j$(nproc)
```

#### 问题: Simulator 模式不工作
```bash
# 检查环境变量
echo $PTO2_MODE

# 检查日志输出
PTO2_MODE=simulator ./simulator_demo 2>&1 | grep PTO2

# 预期看到:
# [INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
```

---

## 11. 与 Agent 沟通

### 告诉新 Agent 的关键信息

#### 项目背景
```
这是一个 C++ 实现的 LLM 推理引擎 (pypto-serving),基于 PTO2 runtime。

参考实现:
- nano-vllm: Scheduler, BlockManager 设计
- SGLang: RadixCache 前缀缓存
- simpler: PTO2 runtime 和算子库

当前进度:
- Phase 0: Mock 框架完成 ✅
- Phase 1: PTO2 集成完成 ✅
- Simulator 框架就绪 ✅
```

#### 当前状态
```
代码可以编译和运行,但算子层面还是 Mock:
- 框架层面: ✅ 完整实现
- 算子层面: ⚠️ 待实现

支持三种执行模式:
- MOCK: 纯 Mock (已实现)
- SIMULATOR: CPU 模拟器 (框架就绪,执行待实现)
- DEVICE: 真实 NPU (框架就绪,执行待实现)
```

#### 下一步任务
```
在 Linux 环境中:
1. 验证 simpler 的 simulator 可以运行
2. 实现 PTO2Runtime 的 simulator 执行逻辑
3. 实现第一个验证算子 (vector add)
4. 逐步实现 LLaMA 算子
```

#### 关键文件
```
必读文档:
- README.md - 项目概览
- SIMULATOR_SETUP.md - Simulator 配置
- EXECUTION_MODES.md - 执行模式参考
- PHASE1_SIMULATOR_READY.md - 当前状态

关键代码:
- src/pto2/pto2_runtime.cpp - 需要实现 simulator 执行
- src/engine/model_runner_pto2.cpp - 需要实现真实推理
```

---

## 12. 快速验证脚本

创建 `verify.sh` 脚本用于快速验证环境:

```bash
#!/bin/bash
# verify.sh - 验证 pypto-serving 环境

set -e

echo "=========================================="
echo "  pypto-serving 环境验证"
echo "=========================================="

# 1. 检查编译器
echo -e "\n[1/6] 检查编译器..."
g++ --version | head -1
g++-15 --version | head -1 || echo "⚠️ g++-15 未安装 (simulator 需要)"

# 2. 检查 CMake
echo -e "\n[2/6] 检查 CMake..."
cmake --version | head -1

# 3. 检查 Python
echo -e "\n[3/6] 检查 Python..."
python3 --version

# 4. 编译项目
echo -e "\n[4/6] 编译项目..."
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON > /dev/null
make -j$(nproc) > /dev/null
echo "✅ 编译成功"

# 5. 运行测试
echo -e "\n[5/6] 运行测试..."
./pypto_unit_tests 2>&1 | grep "PASSED"
./pypto_integration_tests 2>&1 | grep "PASSED"

# 6. 运行示例
echo -e "\n[6/6] 运行示例..."
./simulator_demo > /dev/null 2>&1 && echo "✅ simulator_demo 运行成功"

echo -e "\n=========================================="
echo "  ✅ 环境验证完成!"
echo "=========================================="
```

使用方式:
```bash
chmod +x verify.sh
./verify.sh
```

---

## 13. 目录对照表

### 当前机器 (Mac)
```
/Users/qiaolina/Code/oh-my-knowledge/code/pypto-serving/
```

### 新机器 (Linux) - 建议路径
```
/home/<username>/workspace/pypto-serving/
或
/data/projects/pypto-serving/
```

### 相关仓库
```
simpler:
  当前: /Users/qiaolina/Code/oh-my-knowledge/code/simpler/
  新机器: /home/<username>/workspace/simpler/

nano-vllm:
  当前: /Users/qiaolina/Code/oh-my-knowledge/code/nano-vllm/
  新机器: (可选,仅供参考)

sglang:
  当前: /Users/qiaolina/Code/oh-my-knowledge/code/sglang/
  新机器: (可选,仅供参考)
```

---

## 14. 配置 Agent 工作环境

### 创建 .cursor/rules 或 AGENTS.md

在新机器的项目根目录创建 `AGENTS.md`:

```markdown
# pypto-serving Agent 指南

## 项目概述
C++ 实现的高性能 LLM 推理引擎,基于 PTO2 runtime。

## 当前状态
- Phase 0: Mock 框架 ✅
- Phase 1: PTO2 集成 ✅
- Simulator 框架就绪 ✅

## 执行模式
- MOCK: 纯 Mock (已实现)
- SIMULATOR: CPU 模拟器 (框架就绪)
- DEVICE: 真实 NPU (框架就绪)

## 下一步任务
1. 实现 simulator 执行逻辑
2. 实现 LLaMA 算子
3. 端到端验证

## 关键文档
- SIMULATOR_SETUP.md
- EXECUTION_MODES.md
- PHASE1_SIMULATOR_READY.md

## 编译命令
cd build && cmake .. && make -j$(nproc)

## 测试命令
./pypto_unit_tests
./pypto_integration_tests
```

---

## 15. 常见问题

### Q: 需要传输哪些文件?
A: 传输整个 `pypto-serving/` 目录,但可以排除:
- `build/` (重新编译)
- `.git/` (如果用 git clone)

### Q: 需要传输 simpler 吗?
A: 
- **开发 MOCK 模式**: 不需要
- **开发 SIMULATOR 模式**: 需要
- **开发 DEVICE 模式**: 需要

### Q: Mac 上的 build 产物能用吗?
A: 不能。需要在 Linux 上重新编译。

### Q: 如何确认迁移成功?
A: 运行 `verify.sh` 脚本,或手动执行第 5 节的验证清单。

### Q: 新 Agent 需要什么背景知识?
A: 
- C++17 编程
- CMake 构建系统
- LLM 推理基础 (attention, KV cache)
- PTO2 runtime 概念 (可从文档学习)

---

## 16. 迁移检查清单

在新机器上逐项检查:

- [ ] 代码已传输/克隆
- [ ] g++ 已安装
- [ ] g++-15 已安装 (用于 simulator)
- [ ] CMake 3.20+ 已安装
- [ ] Python 3.8+ 已安装
- [ ] 项目编译成功
- [ ] 单元测试通过 (27/27)
- [ ] 集成测试通过 (8/8)
- [ ] mock_llama 运行成功
- [ ] simulator_demo 运行成功
- [ ] 环境变量 PTO2_MODE 工作正常
- [ ] 阅读了关键文档
- [ ] (可选) simpler 仓库已克隆

---

## 17. 联系和支持

### 文档位置
所有文档都在项目根目录:
- 技术文档: `*.md`
- 代码文档: `src/` 中的头文件注释

### 参考资料
- nano-vllm: `/path/to/nano-vllm/`
- SGLang: `/path/to/sglang/`
- simpler: `/path/to/simpler/`

### 知识库
如果有之前的分析文档:
- `knowledge/inbox/2026-03-03-sglang-RadixAttention调用逻辑分析.md`
- `knowledge/inbox/2026-03-03-simpler-PTO2构图与算子执行流程分析.md`
- `knowledge/inbox/2026-03-03-PTO算子库与LLM推理算子需求分析.md`

---

## 总结

按照本指南,可以在新机器上完整复现 pypto-serving 的开发环境:

1. ✅ **代码迁移** - Git 或 rsync
2. ✅ **依赖安装** - g++, g++-15, CMake, Python
3. ✅ **编译验证** - 所有目标编译成功
4. ✅ **测试验证** - 35 个测试全部通过
5. ✅ **模式验证** - MOCK/SIMULATOR/DEVICE 切换正常

新 Agent 可以从 Phase 2 (Simulator 实际执行) 继续开发!
