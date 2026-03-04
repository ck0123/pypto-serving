#!/bin/bash
# 清理构建产物和临时文件

set -e

echo "========================================="
echo "  清理 pypto-serving 项目"
echo "========================================="
echo ""

# 清理构建目录
if [ -d "build" ]; then
    echo "🗑️  删除 build/ 目录..."
    rm -rf build/
fi

# 清理 CMake 缓存
echo "🗑️  删除 CMake 缓存..."
find . -name "CMakeCache.txt" -delete 2>/dev/null || true
find . -name "CMakeFiles" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "cmake_install.cmake" -delete 2>/dev/null || true

# 清理编译产物
echo "🗑️  删除编译产物..."
find . -name "*.o" -delete 2>/dev/null || true
find . -name "*.a" -delete 2>/dev/null || true
find . -name "*.so" -delete 2>/dev/null || true

# 清理 Python 缓存
echo "🗑️  删除 Python 缓存..."
find . -name "__pycache__" -type d -exec rm -rf {} + 2>/dev/null || true
find . -name "*.pyc" -delete 2>/dev/null || true
find . -name "*.pyo" -delete 2>/dev/null || true
find . -name ".pytest_cache" -type d -exec rm -rf {} + 2>/dev/null || true

# 清理日志文件
echo "🗑️  删除日志文件..."
find . -name "*.log" -delete 2>/dev/null || true

# 清理临时文件
echo "🗑️  删除临时文件..."
find . -name "*.tmp" -delete 2>/dev/null || true
find . -name "*.bak" -delete 2>/dev/null || true
find . -name "*~" -delete 2>/dev/null || true

# 清理编辑器临时文件
echo "🗑️  删除编辑器临时文件..."
find . -name "*.swp" -delete 2>/dev/null || true
find . -name "*.swo" -delete 2>/dev/null || true

echo ""
echo "✅ 清理完成！"
echo ""
echo "保留的内容："
echo "  ✓ 源代码 (src/)"
echo "  ✓ 测试 (tests/)"
echo "  ✓ 示例 (examples/)"
echo "  ✓ 文档 (*.md)"
echo "  ✓ 构建配置 (CMakeLists.txt)"
echo ""
echo "已删除的内容："
echo "  ✗ build/"
echo "  ✗ 编译产物 (*.o, *.a, *.so)"
echo "  ✗ Python 缓存 (__pycache__/)"
echo "  ✗ 日志和临时文件"
echo ""
echo "依赖项 (pypto_workspace/, nano-vllm/) 未删除"
echo "如需重新安装依赖，请运行: ./setup_dependencies.sh"
