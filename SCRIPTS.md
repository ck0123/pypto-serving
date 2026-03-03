# 脚本使用指南

pypto-serving 项目提供了多个自动化脚本,简化开发和部署流程。

---

## 📜 脚本列表

### 1. quick_start.sh ⭐ 推荐
**用途**: 一键完成所有设置和编译

**功能**:
- 安装依赖 (simpler, PTOAS, pypto)
- 设置环境变量
- 配置 CMake
- 编译项目
- 运行测试

**使用方式**:
```bash
# 完整流程
./quick_start.sh

# 跳过依赖安装 (如果已安装)
./quick_start.sh --skip-deps

# 跳过测试
./quick_start.sh --skip-tests

# 查看帮助
./quick_start.sh --help
```

**适用场景**:
- 首次设置项目
- 快速验证环境
- 自动化 CI/CD

**预期输出**:
```
==========================================
  ✅ Quick Start Complete!
==========================================

Build artifacts:
  - libpypto_core.a
  - mock_llama
  - simple_generate
  - pto2_demo
  - simulator_demo
```

---

### 2. setup_dependencies.sh
**用途**: 安装 PTO2 相关依赖

**功能**:
- 克隆 simpler 仓库 (特定 commit)
- 下载 PTOAS 二进制 (根据架构自动选择)
- 克隆 pypto 仓库
- 生成环境配置脚本 (env_setup.sh)

**使用方式**:
```bash
# 默认安装到 ./pypto_workspace
./setup_dependencies.sh

# 自定义安装目录
./setup_dependencies.sh --workspace /path/to/custom/dir

# 查看帮助
./setup_dependencies.sh --help
```

**生成的目录结构**:
```
pypto_workspace/
├── simpler/    # PTO2 runtime 和 kernels
├── ptoas/      # PTO 汇编器
└── pypto/      # PyPTO 库

env_setup.sh    # 环境变量配置脚本
```

**环境变量**:
```bash
export SIMPLER_ROOT=/path/to/pypto_workspace/simpler
export PTOAS_ROOT=/path/to/pypto_workspace/ptoas
export PYPTO_ROOT=/path/to/pypto_workspace/pypto
```

**适用场景**:
- 首次设置开发环境
- 更新依赖版本
- 在新机器上部署

**预期输出**:
```
==========================================
  ✅ Setup Complete!
==========================================

Dependencies installed in: /path/to/pypto_workspace

To use these dependencies, source the environment script:
  source env_setup.sh
```

---

### 3. verify.sh
**用途**: 验证环境和编译结果

**功能**:
- 检查编译器 (g++, g++-15)
- 检查 CMake 版本
- 检查 Python 版本
- 检查 PTO2 依赖
- 编译项目
- 运行所有测试 (单元测试 + 集成测试)
- 运行示例程序
- 测试执行模式切换

**使用方式**:
```bash
./verify.sh
```

**检查项**:
1. ✅ g++ 编译器
2. ✅ g++-15 (用于 simulator)
3. ✅ CMake 3.20+
4. ✅ Python 3.8+
5. ✅ simpler 已安装
6. ✅ PTOAS 已安装
7. ✅ 环境变量已设置
8. ✅ 编译成功
9. ✅ 单元测试通过 (27/27)
10. ✅ 集成测试通过 (8/8)
11. ✅ 示例运行成功
12. ✅ 模式切换正常

**适用场景**:
- 验证新环境
- 检查依赖完整性
- 快速诊断问题
- CI/CD 验证

**预期输出**:
```
==========================================
  ✅ 环境验证完成!
==========================================

项目状态:
  - 编译: ✅
  - 测试: ✅ (35/35 通过)
  - 示例: ✅
  - 模式切换: ✅
```

---

## 🎯 使用场景

### 场景 1: 首次设置项目
```bash
# 方式 1: 一键完成
./quick_start.sh

# 方式 2: 分步执行
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh
./verify.sh
```

### 场景 2: 迁移到新机器
```bash
# 1. 传输代码
git clone <repo-url>
cd pypto-serving

# 2. 一键设置
./quick_start.sh

# 或分步执行
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh
mkdir build && cd build
cmake .. && make -j$(nproc)
```

### 场景 3: 验证环境
```bash
# 快速验证
./verify.sh

# 如果失败,查看日志
cat build/cmake.log
cat build/make.log
cat build/unit_test.log
```

### 场景 4: 更新依赖
```bash
# 删除旧依赖
rm -rf pypto_workspace

# 重新安装
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh

# 重新编译
cd build && make clean && make -j$(nproc)
```

### 场景 5: CI/CD 集成
```yaml
# .github/workflows/build.yml 示例
steps:
  - name: Checkout code
    uses: actions/checkout@v2
  
  - name: Setup and build
    run: ./quick_start.sh
  
  - name: Verify
    run: ./verify.sh
```

---

## 🔧 脚本依赖关系

```
quick_start.sh (一键完成)
    ↓
    ├─→ setup_dependencies.sh (安装依赖)
    │       ↓
    │       └─→ env_setup.sh (设置环境变量)
    │
    ├─→ cmake (配置)
    │
    ├─→ make (编译)
    │
    └─→ 测试和示例

verify.sh (验证)
    ↓
    ├─→ 检查工具链
    ├─→ 检查依赖
    ├─→ 编译
    └─→ 测试
```

---

## 📝 脚本输出

### 成功输出
所有脚本成功时都会显示 `✅` 标记:
```
✅ Setup Complete!
✅ Build successful
✅ Tests passed
✅ 环境验证完成!
```

### 失败输出
失败时会显示 `✗` 或 `❌` 标记,并提示查看日志:
```
✗ Build failed
Check build/make.log for details
```

### 警告输出
非致命问题会显示 `⚠️` 标记:
```
⚠️ g++-15 未安装 (simulator 模式需要)
⚠️ Python3 未安装 (simpler 工具链需要)
```

---

## 🐛 故障排查

### 问题: setup_dependencies.sh 下载失败
**原因**: 网络问题或 GitHub API 限制

**解决**:
```bash
# 手动克隆 simpler
git clone https://github.com/ChaoWao/simpler pypto_workspace/simpler
cd pypto_workspace/simpler
git checkout eede5613f28f9fa2c1ac0b29b27fa6eacb2ef2db

# 手动下载 PTOAS
# 从 https://github.com/zhangstevenunity/PTOAS/releases 下载
```

### 问题: verify.sh 编译失败
**原因**: 依赖未安装或环境变量未设置

**解决**:
```bash
# 检查依赖
ls pypto_workspace/simpler
ls pypto_workspace/ptoas

# 检查环境变量
echo $SIMPLER_ROOT
echo $PTOAS_ROOT

# 重新设置
source pypto_workspace/../env_setup.sh
```

### 问题: quick_start.sh 测试失败
**原因**: 代码问题或环境问题

**解决**:
```bash
# 查看详细日志
cat build/unit_test.log
cat build/integration_test.log

# 手动运行测试
cd build
./pypto_unit_tests -v
./pypto_integration_tests -v
```

---

## 💡 最佳实践

### 1. 首次使用
```bash
# 推荐: 使用 quick_start.sh
./quick_start.sh

# 阅读输出,了解每个步骤
```

### 2. 日常开发
```bash
# 只需编译
cd build && make -j$(nproc)

# 运行测试
./pypto_unit_tests
```

### 3. 定期验证
```bash
# 每周或每次重大更改后
./verify.sh
```

### 4. 环境变量持久化
```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
echo 'source /path/to/pypto-serving/pypto_workspace/../env_setup.sh' >> ~/.bashrc
```

---

## 📊 脚本对比

| 脚本 | 用途 | 耗时 | 适用场景 |
|------|------|------|----------|
| quick_start.sh | 一键完成所有 | 5-10 分钟 | 首次设置 |
| setup_dependencies.sh | 安装依赖 | 3-5 分钟 | 更新依赖 |
| verify.sh | 验证环境 | 2-3 分钟 | 快速检查 |

---

## 🎓 进阶用法

### 自定义编译选项
```bash
# 修改 quick_start.sh 中的 cmake 命令
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_CXX_FLAGS="-g -O0"
```

### 并行测试
```bash
# 修改 verify.sh 中的测试命令
ctest -j$(nproc) --output-on-failure
```

### 自定义依赖版本
```bash
# 修改 setup_dependencies.sh 中的配置
SIMPLER_COMMIT="your-commit-hash"
```

---

## 📞 需要帮助?

查看相关文档:
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - 完整迁移指南
- [QUICKSTART.md](QUICKSTART.md) - 快速开始
- [AGENT_HANDOFF.md](AGENT_HANDOFF.md) - Agent 交接

或运行脚本的 `--help` 选项:
```bash
./quick_start.sh --help
./setup_dependencies.sh --help
```

---

## 总结

- **quick_start.sh**: 一键完成所有设置 ⭐
- **setup_dependencies.sh**: 安装 PTO2 依赖
- **verify.sh**: 验证环境和编译

选择合适的脚本,快速开始开发!
