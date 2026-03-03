#pragma once

#include <string>

namespace pypto {
namespace pto2 {

// PTO2 execution mode
enum class ExecutionMode {
    MOCK,       // Pure mock (no PTO2, for Mac/testing)
    SIMULATOR,  // CPU simulator (no hardware required)
    DEVICE      // Real Ascend NPU device
};

// PTO2 configuration
struct PTO2Config {
    ExecutionMode mode;
    std::string platform;  // "a2a3", "a2a3sim", etc.
    
    PTO2Config() 
        : mode(ExecutionMode::MOCK),
          platform("a2a3") {}
    
    // Auto-detect best available mode
    static PTO2Config auto_detect();
    
    // Create simulator config
    static PTO2Config simulator() {
        PTO2Config config;
        config.mode = ExecutionMode::SIMULATOR;
        config.platform = "a2a3sim";
        return config;
    }
    
    // Create device config
    static PTO2Config device() {
        PTO2Config config;
        config.mode = ExecutionMode::DEVICE;
        config.platform = "a2a3";
        return config;
    }
    
    // Create mock config
    static PTO2Config mock() {
        PTO2Config config;
        config.mode = ExecutionMode::MOCK;
        config.platform = "mock";
        return config;
    }
};

} // namespace pto2
} // namespace pypto
