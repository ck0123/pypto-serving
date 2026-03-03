#!/bin/bash
#
# PyPTO-Serving Quick Start
#
# This script automates the complete setup process:
# 1. Install dependencies (simpler, PTOAS, pypto)
# 2. Set up environment
# 3. Build the project
# 4. Run tests
# 5. Run examples
#
# Usage: ./quick_start.sh [--skip-deps] [--skip-tests]
#

set -e

SKIP_DEPS=false
SKIP_TESTS=false

# Parse arguments
while [ $# -gt 0 ]; do
    case "$1" in
        --skip-deps)
            SKIP_DEPS=true
            shift
            ;;
        --skip-tests)
            SKIP_TESTS=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --skip-deps    Skip dependency installation"
            echo "  --skip-tests   Skip running tests"
            echo "  -h, --help     Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo "=========================================="
echo "  PyPTO-Serving Quick Start"
echo "=========================================="
echo ""

# Step 1: Install dependencies
if [ "$SKIP_DEPS" = false ]; then
    echo "[Step 1/5] Installing dependencies..."
    if [ -d "pypto_workspace/simpler" ] && [ -d "pypto_workspace/ptoas" ]; then
        echo "✓ Dependencies already installed, skipping..."
    else
        ./setup_dependencies.sh
    fi
    
    # Source environment
    if [ -f "pypto_workspace/../env_setup.sh" ]; then
        source pypto_workspace/../env_setup.sh
    fi
else
    echo "[Step 1/5] Skipping dependency installation..."
fi
echo ""

# Step 2: Create build directory
echo "[Step 2/5] Creating build directory..."
mkdir -p build
cd build
echo "✓ Build directory ready"
echo ""

# Step 3: Configure with CMake
echo "[Step 3/5] Configuring with CMake..."
if cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON > cmake.log 2>&1; then
    echo "✓ CMake configuration successful"
else
    echo "✗ CMake configuration failed"
    echo "Check build/cmake.log for details"
    exit 1
fi
echo ""

# Step 4: Build
echo "[Step 4/5] Building project..."
if make -j$(nproc) > make.log 2>&1; then
    echo "✓ Build successful"
else
    echo "✗ Build failed"
    echo "Check build/make.log for details"
    exit 1
fi
echo ""

# Step 5: Run tests
if [ "$SKIP_TESTS" = false ]; then
    echo "[Step 5/5] Running tests..."
    
    echo "  Running unit tests..."
    if ./pypto_unit_tests > unit_test.log 2>&1; then
        UNIT_RESULT=$(grep "PASSED" unit_test.log | tail -1)
        echo "  ✓ Unit tests: $UNIT_RESULT"
    else
        echo "  ✗ Unit tests failed (check build/unit_test.log)"
    fi
    
    echo "  Running integration tests..."
    if ./pypto_integration_tests > integration_test.log 2>&1; then
        INTEGRATION_RESULT=$(grep "PASSED" integration_test.log | tail -1)
        echo "  ✓ Integration tests: $INTEGRATION_RESULT"
    else
        echo "  ✗ Integration tests failed (check build/integration_test.log)"
    fi
else
    echo "[Step 5/5] Skipping tests..."
fi
echo ""

echo "=========================================="
echo "  ✅ Quick Start Complete!"
echo "=========================================="
echo ""
echo "Build artifacts:"
echo "  - libpypto_core.a"
echo "  - mock_llama"
echo "  - simple_generate"
echo "  - pto2_demo"
echo "  - simulator_demo"
echo ""
echo "Try running examples:"
echo "  cd build"
echo "  ./mock_llama"
echo "  ./simulator_demo"
echo "  PTO2_MODE=simulator ./simulator_demo"
echo ""
echo "For more information, see:"
echo "  - README.md"
echo "  - QUICKSTART.md"
echo "  - AGENT_HANDOFF.md"
echo ""
