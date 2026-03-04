# 代码组织说明

## 核心代码（应该提交到 Git）

### 源代码

```
src/
├── common/          # 通用类型和工具
├── engine/          # 推理引擎核心
│   ├── sequence.*
│   ├── scheduler.*
│   ├── model_runner_mock.*
│   ├── model_runner_llama.*       # LLaMA Mock 实现
│   ├── model_runner_llama_pto2.*  # LLaMA PTO2 实现
│   ├── model_runner_pto2.*
│   └── engine.*
├── radix/           # Radix tree KV cache
├── sampling/        # 采样策略
├── frontend/        # 前端接口
├── pto2/            # PTO2 runtime 封装
├── llama/           # LLaMA Mock 实现
└── llama_pto2/      # LLaMA PTO2 实现
```

### 测试

```
tests/
├── unit/            # 单元测试
│   ├── test_*.cpp
│   └── test_llama.cpp
└── integration/     # 集成测试
```

### 示例

```
examples/
├── simple_generate/         # 简单生成示例
├── mock_llama/             # Mock LLaMA 示例
├── llama_demo/             # LLaMA 演示
├── llama_test/             # LLaMA 测试
├── llama_pto2_demo/        # LLaMA PTO2 演示
├── config_verification/    # 配置验证
├── tinyllama_test/         # TinyLlama 测试
├── pto2_demo/              # PTO2 演示
└── simulator_demo/         # 模拟器演示
```

### 文档

```
*.md                 # 所有 Markdown 文档
├── README.md
├── DESIGN_COMPARISON.md
├── LLAMA_IMPLEMENTATION.md
├── MOCK_VS_REAL.md
├── MODEL_CONFIGS.md
├── PTO2_INTEGRATION_V1.md
└── ...
```

### 构建配置

```
CMakeLists.txt       # CMake 构建配置
setup_dependencies.sh # 依赖安装脚本
quick_start.sh       # 快速启动脚本
```

---

## 不提交到 Git 的内容（已在 .gitignore 中）

### 构建产物

- `build/` - CMake 构建目录
- `*.o`, `*.a`, `*.so` - 编译产物
- `compile_commands.json` - CMake 生成的编译数据库

### 依赖项

- `pypto_workspace/` - 外部依赖（simpler, PTOAS, pypto）
- `nano-vllm/` - 参考代码（不是 serving 的一部分）
- `third_party/` - 第三方库

### 临时文件

- `env_setup.sh` - 自动生成的环境设置脚本
- `verify_*.sh` - 临时验证脚本
- `*.log`, `*.tmp` - 日志和临时文件

### IDE 配置

- `.vscode/`, `.idea/` - IDE 配置
- `*.swp`, `*.swo` - 编辑器临时文件

### Python 相关

- `__pycache__/`, `*.pyc` - Python 缓存
- `.venv/`, `venv/` - 虚拟环境

### 模型文件

- `models/` - 模型权重文件（太大）
- `*.safetensors`, `*.bin` - 权重文件

---

## 开发工作流

### 1. 克隆仓库

```bash
git clone <repo-url>
cd pypto-serving
```

### 2. 安装依赖

```bash
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh  # 如果生成了
```

### 3. 构建

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON
make -j$(nproc)
```

### 4. 运行测试

```bash
ctest --output-on-failure
```

### 5. 提交代码

```bash
# 添加核心代码
git add src/ tests/ examples/ CMakeLists.txt *.md

# 提交
git commit -m "描述你的更改"

# 推送
git push
```

---

## 代码审查清单

提交前请确认：

- [ ] 所有新代码都在 `src/`、`tests/` 或 `examples/` 目录下
- [ ] 没有提交 `build/` 目录
- [ ] 没有提交 `pypto_workspace/` 或 `nano-vllm/`
- [ ] 没有提交临时脚本（`env_setup.sh`, `verify_*.sh`）
- [ ] 没有提交 IDE 配置文件
- [ ] 更新了相关文档（如果需要）
- [ ] 代码可以编译通过
- [ ] 测试通过

---

## 目录结构概览

```
pypto-serving/
├── src/                    ✅ 核心源代码
├── tests/                  ✅ 测试代码
├── examples/               ✅ 示例程序
├── CMakeLists.txt          ✅ 构建配置
├── *.md                    ✅ 文档
├── setup_dependencies.sh   ✅ 依赖安装
├── quick_start.sh          ✅ 快速启动
├── .gitignore              ✅ Git 忽略配置
│
├── build/                  ❌ 构建产物（忽略）
├── pypto_workspace/        ❌ 依赖项（忽略）
├── nano-vllm/              ❌ 参考代码（忽略）
├── env_setup.sh            ❌ 自动生成（忽略）
└── verify_*.sh             ❌ 临时脚本（忽略）
```

---

## 常见问题

### Q: 为什么 `pypto_workspace/` 不提交？

A: 这是外部依赖目录，通过 `setup_dependencies.sh` 自动下载和管理。每个开发者应该自己运行安装脚本。

### Q: 为什么 `nano-vllm/` 不提交？

A: 这只是参考代码，用于设计对比，不是 `pypto-serving` 的一部分。

### Q: 如何更新依赖？

A: 修改 `setup_dependencies.sh` 脚本，然后重新运行。

### Q: 如何添加新的源文件？

A: 
1. 在 `src/` 下创建新文件
2. 在 `CMakeLists.txt` 中添加到 `PYPTO_SOURCES`
3. 重新运行 `cmake` 和 `make`

---

## 联系方式

如有问题，请查看 `README.md` 或提交 Issue。
