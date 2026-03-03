# 迁移检查清单

在新机器上逐项完成以下检查。

---

## 📦 代码传输

- [ ] 代码已传输到新机器
  - 方式 1: `git clone <repo-url>` (推荐)
  - 方式 2: `rsync -avz pypto-serving/ user@remote:/path/`
  - 方式 3: `scp pypto-serving.tar.gz user@remote:/path/`

- [ ] 验证文件完整性
  ```bash
  cd pypto-serving
  ls -la src/ tests/ examples/ docs/
  ```

---

## 🛠️ 环境安装

### 基础工具

- [ ] g++ 已安装
  ```bash
  g++ --version
  # 应该显示版本号
  ```

- [ ] g++-15 已安装 (用于 simulator)
  ```bash
  g++-15 --version
  # 如果未安装:
  # sudo add-apt-repository ppa:ubuntu-toolchain-r/test
  # sudo apt update && sudo apt install g++-15
  ```

- [ ] CMake 3.20+ 已安装
  ```bash
  cmake --version
  # 应该 >= 3.20
  ```

- [ ] Python 3.8+ 已安装
  ```bash
  python3 --version
  ```

### PTO2 依赖 (用于 simulator/device 模式)

- [ ] 运行依赖安装脚本
  ```bash
  cd pypto-serving
  ./setup_dependencies.sh
  # 应该看到: ✅ Setup Complete!
  ```

- [ ] 环境变量已设置
  ```bash
  source pypto_workspace/../env_setup.sh
  echo $SIMPLER_ROOT
  echo $PTOAS_ROOT
  # 应该显示正确的路径
  ```

---

## 🔨 编译验证

- [ ] 创建构建目录
  ```bash
  mkdir build && cd build
  ```

- [ ] CMake 配置成功
  ```bash
  cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
  # 应该看到: -- Build files have been written to...
  ```

- [ ] 编译成功
  ```bash
  make -j$(nproc)
  # 应该看到: [100%] Built target ...
  ```

- [ ] 生成了所有目标
  ```bash
  ls -lh libpypto_core.a
  ls -lh mock_llama simple_generate pto2_demo simulator_demo
  ls -lh pypto_unit_tests pypto_integration_tests
  # 所有文件都应该存在
  ```

---

## ✅ 测试验证

- [ ] 单元测试通过
  ```bash
  ./pypto_unit_tests
  # 应该看到: [  PASSED  ] 27 tests.
  ```

- [ ] 集成测试通过
  ```bash
  ./pypto_integration_tests
  # 应该看到: [  PASSED  ] 8 tests.
  ```

- [ ] ctest 通过
  ```bash
  ctest --output-on-failure
  # 应该看到: 100% tests passed, 0 tests failed out of 2
  ```

---

## 🎮 示例验证

- [ ] mock_llama 运行成功
  ```bash
  ./mock_llama
  # 应该看到 5 个 demo 的输出
  ```

- [ ] simple_generate 运行成功
  ```bash
  ./simple_generate
  # 应该看到生成的 token
  ```

- [ ] pto2_demo 运行成功
  ```bash
  ./pto2_demo
  # 应该看到 PTO2 device 状态 (Mac 上为 false)
  ```

- [ ] simulator_demo 运行成功
  ```bash
  ./simulator_demo
  # 应该看到当前执行模式和生成结果
  ```

---

## 🔄 模式切换验证

- [ ] MOCK 模式正常
  ```bash
  PTO2_MODE=mock ./simulator_demo
  # 应该看到: [INFO] PTO2Runtime: Mock mode
  ```

- [ ] SIMULATOR 模式识别正常
  ```bash
  PTO2_MODE=simulator ./simulator_demo
  # 应该看到: [INFO] PTO2Runtime: Simulator mode (platform=a2a3sim)
  # 以及: [WARN] Simulator not yet implemented, using mock
  ```

- [ ] DEVICE 模式识别正常
  ```bash
  PTO2_MODE=device ./simulator_demo
  # 应该看到: [INFO] PTO2Runtime: Device mode
  ```

- [ ] 自动检测正常
  ```bash
  ./simulator_demo
  # Mac 上应该自动选择 MOCK
  # Linux 上应该自动选择 SIMULATOR
  ```

---

## 📚 文档阅读

- [ ] 阅读 README.md
- [ ] 阅读 QUICKSTART.md
- [ ] 阅读 AGENT_HANDOFF.md ⭐ 重要
- [ ] 阅读 SIMULATOR_SETUP.md
- [ ] 阅读 EXECUTION_MODES.md
- [ ] 阅读 PHASE1_SIMULATOR_READY.md

---

## 🎯 理解当前状态

- [ ] 理解项目目标
  - C++ LLM 推理引擎
  - 基于 PTO2 runtime
  - 参考 nano-vllm 和 SGLang

- [ ] 理解当前进度
  - Phase 0: Mock 框架完成 ✅
  - Phase 1: PTO2 集成完成 ✅
  - Phase 2: Simulator 执行待实现 ⚠️

- [ ] 理解三种执行模式
  - MOCK: 纯 Mock (已实现)
  - SIMULATOR: CPU 模拟器 (框架就绪)
  - DEVICE: 真实 NPU (框架就绪)

- [ ] 理解下一步任务
  - 实现 simulator 执行逻辑
  - 实现 LLaMA 算子
  - 端到端验证

---

## 🔍 代码熟悉

- [ ] 浏览项目结构
  ```bash
  tree -L 2 src/
  ```

- [ ] 查看关键文件
  - [ ] `src/pto2/pto2_runtime.cpp` - 需要实现 simulator
  - [ ] `src/engine/model_runner_pto2.cpp` - 需要实现推理
  - [ ] `src/pto2/pto2_config.h` - 执行模式配置

- [ ] 运行调试
  ```bash
  gdb ./simulator_demo
  # 设置断点,单步调试
  ```

---

## 🚀 开始开发

- [ ] 验证 simpler simulator 可用
  ```bash
  cd /path/to/simpler
  python examples/scripts/run_example.py \
      --kernels examples/host_build_graph/vector_example/kernels \
      --golden examples/host_build_graph/vector_example/golden.py \
      --platform a2a3sim
  ```

- [ ] 创建第一个 kernel 目录
  ```bash
  mkdir -p src/kernels/vector_add/{orchestration,aic,aiv}
  ```

- [ ] 开始实现 `PTO2Runtime::setup_simulator()`

---

## ✨ 快速验证脚本

- [ ] 运行自动验证
  ```bash
  ./verify.sh
  # 应该看到所有检查项都通过
  ```

---

## 📝 记录

### 迁移完成时间
- 开始时间: ___________
- 完成时间: ___________
- 总耗时: ___________

### 遇到的问题
1. 
2. 
3. 

### 解决方案
1. 
2. 
3. 

---

## ✅ 迁移完成标志

当以上所有项都勾选完成时,迁移成功!

可以开始 Phase 2 开发: Simulator 执行实现。

参考 `SIMULATOR_SETUP.md` 的 "Phase 2: Simulator 集成" 部分。
