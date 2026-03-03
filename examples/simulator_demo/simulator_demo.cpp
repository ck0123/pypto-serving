#include "engine/engine.h"
#include "pto2/pto2_config.h"
#include "common/logger.h"
#include <iostream>
#include <cstdlib>

using namespace pypto;

int main() {
    // Set log level
    Logger::instance().set_level(LogLevel::INFO);
    
    std::cout << "========================================" << std::endl;
    std::cout << "   PTO2 Simulator Mode Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Configure
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    // Check environment variable
    const char* mode_env = std::getenv("PTO2_MODE");
    std::cout << "\nEnvironment:" << std::endl;
    std::cout << "  PTO2_MODE = " << (mode_env ? mode_env : "(not set)") << std::endl;
    std::cout << "\nYou can set PTO2_MODE to:" << std::endl;
    std::cout << "  - mock       : Pure mock (no PTO2)" << std::endl;
    std::cout << "  - simulator  : CPU simulator mode" << std::endl;
    std::cout << "  - device     : Real Ascend NPU" << std::endl;
    std::cout << std::endl;
    
    // Create engine with PTO2 runner (will auto-detect mode)
    std::cout << "Creating engine with PTO2 model runner..." << std::endl;
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config,
        ModelRunnerType::PTO2);
    
    // Check execution mode
    std::cout << "\nExecution mode: ";
    if (engine->is_using_pto2_device()) {
        std::cout << "✓ DEVICE (Real Ascend NPU)" << std::endl;
    } else {
        std::cout << "⚠ MOCK/SIMULATOR (fallback)" << std::endl;
    }
    
    // Prepare prompt
    TokenIds prompt = {1, 2, 3, 4, 5};
    
    std::cout << "\nPrompt tokens: [";
    for (size_t i = 0; i < prompt.size(); ++i) {
        std::cout << prompt[i];
        if (i < prompt.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Sampling parameters
    SamplingParams params;
    params.temperature = 0.0f;  // Greedy
    params.max_tokens = 10;
    
    std::cout << "Generating " << params.max_tokens << " tokens..." << std::endl;
    
    // Generate
    auto outputs = engine->generate({prompt}, params);
    
    // Print result
    std::cout << "\nGenerated tokens: [";
    for (size_t i = 0; i < outputs[0].output_token_ids.size(); ++i) {
        std::cout << outputs[0].output_token_ids[i];
        if (i < outputs[0].output_token_ids.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Demo complete!" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nTo test simulator mode:" << std::endl;
    std::cout << "  PTO2_MODE=simulator ./simulator_demo" << std::endl;
    std::cout << std::endl;
    
    return 0;
}
