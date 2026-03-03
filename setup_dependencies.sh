#!/usr/bin/env bash
#
# PyPTO-Serving Dependency Setup
#
# This script sets up the dependencies for pypto-serving project.
# It clones necessary dependencies (simpler, pypto, PTOAS) and sets up the environment.
#
# Usage: ./setup_dependencies.sh [--workspace DIR]
#
# Examples:
#   ./setup_dependencies.sh                    # Setup in ./pypto_workspace
#   ./setup_dependencies.sh --workspace /tmp   # Setup in custom directory
#

set -e

# Configuration
SIMPLER_REPO="https://github.com/ChaoWao/simpler"
SIMPLER_COMMIT="eede5613f28f9fa2c1ac0b29b27fa6eacb2ef2db"
PYPTO_REPO="https://github.com/hw-native-sys/pypto.git"
PTOAS_REPO="https://github.com/zhangstevenunity/PTOAS"
DEFAULT_WORKSPACE_DIR="./pypto_workspace"

# Parse arguments
WORKSPACE_DIR="$DEFAULT_WORKSPACE_DIR"

while [ $# -gt 0 ]; do
    case "$1" in
        --workspace)
            WORKSPACE_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--workspace DIR]"
            echo ""
            echo "Options:"
            echo "  --workspace DIR    Set workspace directory (default: ./pypto_workspace)"
            echo "  -h, --help         Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

# Setup workspace
echo "=========================================="
echo "  PyPTO-Serving Dependency Setup"
echo "=========================================="
echo ""
echo "Workspace: $WORKSPACE_DIR"
echo ""

mkdir -p "$WORKSPACE_DIR"
cd "$WORKSPACE_DIR"
WORKSPACE_ABS=$(pwd)

# Clone and setup simpler repository
echo "[1/3] Setting up simpler..."
SIMPLER_DIR="$WORKSPACE_ABS/simpler"
if [ -d "$SIMPLER_DIR" ]; then
    echo "✓ simpler directory exists, checking commit..."
    cd "$SIMPLER_DIR"
    CURRENT_COMMIT=$(git rev-parse HEAD)
    if [ "$CURRENT_COMMIT" != "$SIMPLER_COMMIT" ]; then
        echo "  Checking out simpler commit: $SIMPLER_COMMIT"
        git fetch origin
        git checkout "$SIMPLER_COMMIT"
    else
        echo "  Already on correct commit"
    fi
else
    echo "  Cloning simpler repository..."
    git clone "$SIMPLER_REPO" simpler
    cd simpler
    git checkout "$SIMPLER_COMMIT"
    echo "✓ simpler cloned"
fi

export SIMPLER_ROOT="$SIMPLER_DIR"
echo "  SIMPLER_ROOT=$SIMPLER_ROOT"
echo ""

# Download and setup PTOAS
echo "[2/3] Setting up PTOAS..."
PTOAS_DIR="$WORKSPACE_ABS/ptoas"
if [ -d "$PTOAS_DIR" ]; then
    echo "✓ PTOAS directory exists at $PTOAS_DIR"
else
    echo "  Downloading PTOAS..."
    mkdir -p "$PTOAS_DIR"
    
    # Detect platform
    ARCH=$(uname -m)
    if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
        PTOAS_ARCHIVE="ptoas-bin-aarch64.tar.gz"
    elif [ "$ARCH" = "x86_64" ]; then
        PTOAS_ARCHIVE="ptoas-bin-x86_64.tar.gz"
    else
        echo "Error: Unsupported architecture: $ARCH"
        exit 1
    fi
    
    # Get latest release download URL
    echo "  Fetching latest release info..."
    PTOAS_DOWNLOAD_URL=$(curl -s https://api.github.com/repos/zhangstevenunity/PTOAS/releases/latest | \
        grep -o "\"browser_download_url\": \"[^\"]*${PTOAS_ARCHIVE}[^\"]*\"" | \
        head -1 | \
        sed 's/"browser_download_url": "\(.*\)"/\1/')
    
    if [ -z "$PTOAS_DOWNLOAD_URL" ]; then
        echo "Error: Could not find PTOAS download URL for $PTOAS_ARCHIVE"
        exit 1
    fi
    
    echo "  Downloading from: $PTOAS_DOWNLOAD_URL"
    curl -L -o "$WORKSPACE_ABS/$PTOAS_ARCHIVE" "$PTOAS_DOWNLOAD_URL"
    
    echo "  Extracting PTOAS to $PTOAS_DIR..."
    tar -xzf "$WORKSPACE_ABS/$PTOAS_ARCHIVE" -C "$PTOAS_DIR"
    rm "$WORKSPACE_ABS/$PTOAS_ARCHIVE"
    
    # Make ptoas executable
    chmod +x "$PTOAS_DIR/ptoas" 2>/dev/null || true
    chmod +x "$PTOAS_DIR/bin/ptoas" 2>/dev/null || true
    
    echo "✓ PTOAS setup complete"
fi

export PTOAS_ROOT="$PTOAS_DIR"
echo "  PTOAS_ROOT=$PTOAS_ROOT"
echo ""

# Clone and setup pypto repository
echo "[3/3] Setting up pypto..."
cd "$WORKSPACE_ABS"
PYPTO_DIR="$WORKSPACE_ABS/pypto"
if [ -d "$PYPTO_DIR" ]; then
    echo "✓ pypto directory exists, pulling latest..."
    cd "$PYPTO_DIR"
    git pull || true
else
    echo "  Cloning pypto repository..."
    git clone --recursive "$PYPTO_REPO" pypto
    cd "$PYPTO_DIR"
    echo "✓ pypto cloned"
fi

# Install Python dependencies (optional for C++ project)
if command -v python3 &> /dev/null; then
    echo ""
    echo "Installing pypto Python package (optional)..."
    python3 -m pip install --upgrade pip > /dev/null 2>&1 || true
    pip install -v .[dev] > /dev/null 2>&1 || echo "  (Python package installation skipped)"
fi

# Create environment setup script
cd "$WORKSPACE_ABS/.."
ENV_SCRIPT="$WORKSPACE_ABS/../env_setup.sh"
cat > "$ENV_SCRIPT" << EOF
#!/usr/bin/env bash
# PyPTO-Serving Environment Setup
# Source this file to set up environment variables

export SIMPLER_ROOT="$SIMPLER_DIR"
export PTOAS_ROOT="$PTOAS_DIR"
export PYPTO_ROOT="$PYPTO_DIR"

echo "Environment variables set:"
echo "  SIMPLER_ROOT=\$SIMPLER_ROOT"
echo "  PTOAS_ROOT=\$PTOAS_ROOT"
echo "  PYPTO_ROOT=\$PYPTO_ROOT"
EOF

chmod +x "$ENV_SCRIPT"

echo ""
echo "=========================================="
echo "  ✅ Setup Complete!"
echo "=========================================="
echo ""
echo "Dependencies installed in: $WORKSPACE_ABS"
echo ""
echo "Directory structure:"
echo "  $WORKSPACE_ABS/"
echo "  ├── simpler/    (PTO2 runtime and kernels)"
echo "  ├── ptoas/      (PTO assembler)"
echo "  └── pypto/      (PyPTO library)"
echo ""
echo "Environment setup script created: $ENV_SCRIPT"
echo ""
echo "To use these dependencies, source the environment script:"
echo "  source $ENV_SCRIPT"
echo ""
echo "Or manually set environment variables:"
echo "  export SIMPLER_ROOT=$SIMPLER_DIR"
echo "  export PTOAS_ROOT=$PTOAS_DIR"
echo "  export PYPTO_ROOT=$PYPTO_DIR"
echo ""
echo "Next steps:"
echo "  1. Source the environment: source $ENV_SCRIPT"
echo "  2. Build pypto-serving: cd build && cmake .. && make"
echo "  3. Run tests: ./pypto_unit_tests"
echo ""
