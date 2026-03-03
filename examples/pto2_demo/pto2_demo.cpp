#include "engine/engine.h"
#include "common/logger.h"
#include <iostream>

using namespace pypto;

int main() {
    // Set log level
    Logger::instance().set_level(LogLevel::INFO);
    
    std::cout << "========================================" << std::endl;
    std::cout << "   PTO2 Model Runner Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Configure
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    // Create engine with PTO2 runner
    std::cout << "\nCreating engine with PTO2 model runner..." << std::endl;
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config,
        ModelRunnerType::PTO2);  // Use PTO2 runner
    
    // Check if using device
    if (engine->is_using_pto2_device()) {
        std::cout << "✓ Using PTO2 device" << std::endl;
    } else {
        std::cout << "⚠ PTO2 device not available, using mock fallback" << std::endl;
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
    
    return 0;
}
