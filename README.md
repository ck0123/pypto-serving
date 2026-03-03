# pypto-serving

A high-performance LLM inference engine written in C++, powered by PTO2 runtime.

## Design Goals

1. **C/C++ Core**: Performance-critical paths implemented in C/C++ (no Python in hot path)
2. **OpenAI Compatible**: OpenAI-style API with test path for validation
3. **Radix Tree KV Cache**: SGLang-style radix attention with persistent storage
4. **PTO2 Runtime**: Leverages simpler's PTO2 for NPU execution
5. **Stateless**: No long-term session state, only KV cache persistence
6. **Test-First**: Test path for validation before network integration

## Architecture

```
Frontend (Test Path) → Engine → Scheduler → Model Runner
                                    ↓
                              Radix Cache + Block Manager
                                    ↓
                              PTO2 Runtime + Kernels
```

## Current Status: Phase 1 Complete + Simulator Ready

Phase 0 (Mock Framework) and Phase 1 (PTO2 Integration) are complete. The framework supports three execution modes:
- **MOCK**: Pure mock (no PTO2, for development)
- **SIMULATOR**: CPU simulator mode (no hardware, ready for integration)
- **DEVICE**: Real Ascend NPU (ready for integration)

### Components

**Phase 0 (Mock Framework)**
- [x] Project structure
- [x] Basic types (`src/common/types.h`)
- [x] Sequence management (`src/engine/sequence.h/cpp`)
- [x] Block manager (`src/radix/block_manager.h/cpp`)
- [x] Scheduler (`src/engine/scheduler.h/cpp`)
- [x] Mock model runner (`src/engine/model_runner_mock.h/cpp`)
- [x] Sampler (`src/sampling/sampler.h/cpp`)
- [x] Engine (`src/engine/engine.h/cpp`)
- [x] Test path (`src/frontend/test_path.h/cpp`)
- [x] Unit tests (`tests/unit/`)
- [x] Integration tests (`tests/integration/`)
- [x] Mock LLaMA example (`examples/mock_llama/`)

**Phase 1 (PTO2 Integration)**
- [x] PTO2 types (`src/pto2/pto2_types.h`)
- [x] PTO2 config system (`src/pto2/pto2_config.h/cpp`)
- [x] PTO2 runtime wrapper (`src/pto2/pto2_runtime.h/cpp`)
- [x] PTO2 model runner (`src/engine/model_runner_pto2.h/cpp`)
- [x] Execution mode support (MOCK/SIMULATOR/DEVICE)
- [x] Environment variable control (`PTO2_MODE`)
- [x] Multi-runner support in Engine
- [x] PTO2 demo example (`examples/pto2_demo/`)
- [x] Simulator demo example (`examples/simulator_demo/`)

## Build

```bash
# Step 1: Install dependencies (first time only)
./setup_dependencies.sh
source pypto_workspace/../env_setup.sh

# Step 2: Create build directory
mkdir build && cd build

# Step 3: Configure
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON

# Step 4: Build
make -j$(nproc)

# Step 5: Run tests
ctest --output-on-failure

# Step 6: Run examples
./mock_llama        # Mock LLaMA demo (5 scenarios)
./simple_generate   # Simple generation example
./pto2_demo         # PTO2 runner demo (with fallback)
./simulator_demo    # Simulator mode demo

# Test simulator mode
PTO2_MODE=simulator ./simulator_demo
```

## References

- **nano-vllm**: Scheduler, BlockManager, Sequence design
- **SGLang**: RadixCache, prefix matching
- **simpler**: PTO2 runtime, orchestration, kernels
- **PTO-ISA**: 90+ tile operations, GEMM, Flash Attention

## Documentation

📖 **[Complete Documentation Index](DOCS_INDEX.md)** - Find any document quickly

### Getting Started
- [Start Here](START_HERE.md) ⭐ NEW - New agent entry point
- [Quick Start Guide](QUICKSTART.md) - Build and run in 5 minutes
- [Scripts Guide](SCRIPTS.md) ⭐ NEW - Automation scripts usage
- [Migration Guide](MIGRATION_GUIDE.md) ⭐ NEW - Move to new machine/agent
- [Agent Handoff](AGENT_HANDOFF.md) ⭐ NEW - For new agent onboarding

### Technical Guides
- [Simulator Setup Guide](SIMULATOR_SETUP.md) - Configure simulator mode
- [Execution Modes Reference](EXECUTION_MODES.md) - MOCK/SIMULATOR/DEVICE modes

### Progress Reports
- [Phase 0 Complete Report](PHASE0_COMPLETE.md) - Mock framework
- [Phase 1 Complete Report](PHASE1_SIMULATOR_READY.md) - PTO2 integration + simulator ready

### Design Documents
- [Implementation Plan v2](implementation_plan_v2.md) - Full roadmap
- [Design Goals](design%20goal.md) - Architecture principles
- [Reference: SGLang & vLLM](reference_sglang_vllm.md) - Inspiration sources

## License

TBD
