# 依赖安装脚本总结

本次为 pypto-serving 项目添加了完整的依赖安装和自动化脚本。

---

## 📦 新增文件

### 1. setup_dependencies.sh (6K)
**来源**: 基于 `/Users/qiaolina/Code/oh-my-knowledge/code/run_pto/run_pypto.sh`

**功能**:
- 自动克隆 simpler 仓库 (特定 commit)
- 自动下载 PTOAS 二进制 (根据架构)
- 自动克隆 pypto 仓库
- 生成环境配置脚本 (env_setup.sh)

**使用**:
```bash
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh
```

**输出**:
```
pypto_workspace/
├── simpler/    # PTO2 runtime
├── ptoas/      # PTO 汇编器
└── pypto/      # PyPTO 库

env_setup.sh    # 环境变量配置
```

---

### 2. quick_start.sh (4K) ⭐ 新增
**功能**: 一键完成所有设置

**流程**:
1. 安装依赖 (调用 setup_dependencies.sh)
2. 设置环境变量
3. 配置 CMake
4. 编译项目
5. 运行测试

**使用**:
```bash
./quick_start.sh
```

**选项**:
- `--skip-deps`: 跳过依赖安装
- `--skip-tests`: 跳过测试

---

### 3. verify.sh (更新)
**新增功能**:
- 检查 simpler 是否已安装
- 检查 PTOAS 是否已安装
- 检查环境变量是否已设置

**使用**:
```bash
./verify.sh
```

---

### 4. SCRIPTS.md (新增)
**功能**: 脚本使用指南

**内容**:
- 所有脚本的详细说明
- 使用场景和示例
- 故障排查
- 最佳实践

---

## 📝 更新的文档

### 1. MIGRATION_GUIDE.md
**更新内容**:
- 添加 setup_dependencies.sh 使用说明
- 更新依赖安装章节
- 添加自动安装和手动安装两种方式

### 2. MIGRATION_CHECKLIST.md
**更新内容**:
- 添加依赖安装检查项
- 添加环境变量检查项

### 3. AGENT_HANDOFF.md
**更新内容**:
- 添加依赖安装步骤
- 更新快速开始流程

### 4. README.md
**更新内容**:
- 添加依赖安装步骤
- 添加 SCRIPTS.md 链接

### 5. START_HERE.md
**更新内容**:
- 添加一键启动方式
- 更新快速开始流程

---

## 🎯 使用场景

### 场景 1: 首次设置 (最简单)
```bash
./quick_start.sh
```

### 场景 2: 分步设置
```bash
# 1. 安装依赖
./setup_dependencies.sh

# 2. 设置环境
source pypto_workspace/../env_setup.sh

# 3. 验证
./verify.sh

# 4. 编译
cd build && make -j$(nproc)
```

### 场景 3: 迁移到新机器
```bash
# 1. 传输代码
git clone <repo-url>
cd pypto-serving

# 2. 一键设置
./quick_start.sh
```

---

## 🔧 依赖说明

### simpler
- **用途**: PTO2 runtime 和 kernels
- **版本**: commit eede5613f28f9fa2c1ac0b29b27fa6eacb2ef2db
- **仓库**: https://github.com/ChaoWao/simpler

### PTOAS
- **用途**: PTO 汇编器
- **版本**: 最新 release
- **仓库**: https://github.com/zhangstevenunity/PTOAS
- **架构**: 自动检测 (aarch64/x86_64)

### pypto
- **用途**: PyPTO 库 (可选,用于 Python 接口)
- **版本**: 最新 main 分支
- **仓库**: https://github.com/hw-native-sys/pypto.git

---

## 🌍 环境变量

安装完成后,会生成 `env_setup.sh`:

```bash
export SIMPLER_ROOT=/path/to/pypto_workspace/simpler
export PTOAS_ROOT=/path/to/pypto_workspace/ptoas
export PYPTO_ROOT=/path/to/pypto_workspace/pypto
```

**使用方式**:
```bash
source pypto_workspace/../env_setup.sh
```

**持久化** (可选):
```bash
echo 'source /path/to/pypto-serving/pypto_workspace/../env_setup.sh' >> ~/.bashrc
```

---

## ✅ 验证安装

运行验证脚本:
```bash
./verify.sh
```

应该看到:
```
[3.5/6] 检查 PTO2 依赖...
✅ simpler 已安装
✅ PTOAS 已安装
✅ SIMPLER_ROOT 已设置: /path/to/simpler
```

---

## 📊 脚本对比

| 脚本 | 功能 | 耗时 | 适用场景 |
|------|------|------|----------|
| quick_start.sh | 一键完成所有 | 5-10 分钟 | 首次设置 ⭐ |
| setup_dependencies.sh | 安装依赖 | 3-5 分钟 | 更新依赖 |
| verify.sh | 验证环境 | 2-3 分钟 | 快速检查 |

---

## 🔗 相关文档

- [SCRIPTS.md](SCRIPTS.md) - 脚本详细使用指南
- [MIGRATION_GUIDE.md](MIGRATION_GUIDE.md) - 完整迁移指南
- [AGENT_HANDOFF.md](AGENT_HANDOFF.md) - Agent 交接文档

---

## 🎉 总结

现在 pypto-serving 项目具备:

- ✅ 自动依赖安装 (setup_dependencies.sh)
- ✅ 一键完整设置 (quick_start.sh)
- ✅ 环境验证 (verify.sh)
- ✅ 完整文档 (SCRIPTS.md)

新用户可以通过一条命令完成所有设置:
```bash
./quick_start.sh
```

迁移到新机器也非常简单:
```bash
git clone <repo-url>
cd pypto-serving
./quick_start.sh
```
