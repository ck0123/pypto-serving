#include "engine/engine.h"
#include "common/logger.h"
#include <iostream>

using namespace pypto;

int main() {
    // Set log level
    Logger::instance().set_level(LogLevel::INFO);
    
    std::cout << "Simple Generation Example" << std::endl;
    std::cout << "=========================" << std::endl;
    
    // Configure
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    // Create engine
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
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
    
    std::cout << "\nFinish reason: ";
    switch (outputs[0].finish_reason) {
        case FinishReason::LENGTH:
            std::cout << "LENGTH (reached max_tokens)" << std::endl;
            break;
        case FinishReason::EOS:
            std::cout << "EOS (generated end-of-sequence token)" << std::endl;
            break;
        default:
            std::cout << "UNKNOWN" << std::endl;
    }
    
    std::cout << "\nGeneration complete!" << std::endl;
    
    return 0;
}
