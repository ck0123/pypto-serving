#include "engine/engine.h"
#include "frontend/test_path.h"
#include "common/logger.h"
#include <iostream>
#include <vector>
#include <memory>

using namespace pypto;

void print_tokens(const std::string& label, const TokenIds& tokens) {
    std::cout << label << ": [";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i];
        if (i < tokens.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
}

void demo_simple_generation() {
    std::cout << "\n=== Demo 1: Simple Generation ===" << std::endl;
    
    // Configure
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    scheduler_config.max_num_seqs = 256;
    scheduler_config.max_num_batched_tokens = 2048;
    
    // Create engine
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
    // Generate
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params(0.0f, -1, 1.0f, 10, false);  // Greedy, 10 tokens
    
    std::cout << "Generating with prompt: [1, 2, 3, 4, 5]" << std::endl;
    
    auto outputs = engine->generate({prompt}, params);
    
    print_tokens("Generated tokens", outputs[0].output_token_ids);
    std::cout << "Finish reason: " 
              << (outputs[0].finish_reason == FinishReason::LENGTH ? "LENGTH" : "OTHER")
              << std::endl;
}

void demo_batch_generation() {
    std::cout << "\n=== Demo 2: Batch Generation ===" << std::endl;
    
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
    // Multiple prompts
    std::vector<TokenIds> prompts = {
        {1, 2, 3},
        {4, 5, 6, 7},
        {8, 9, 10, 11, 12}
    };
    
    SamplingParams params(0.0f, -1, 1.0f, 5, false);
    
    std::cout << "Generating for " << prompts.size() << " prompts" << std::endl;
    
    auto outputs = engine->generate(prompts, params);
    
    for (size_t i = 0; i < outputs.size(); ++i) {
        std::cout << "Request " << i << ": ";
        print_tokens("output", outputs[i].output_token_ids);
    }
}

void demo_test_path() {
    std::cout << "\n=== Demo 3: Test Path (Async) ===" << std::endl;
    
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
    auto test_path = std::make_shared<TestPath>(engine);
    test_path->start();
    
    // Inject requests
    std::vector<RequestId> request_ids;
    SamplingParams params(0.0f, -1, 1.0f, 5, false);
    
    for (int i = 0; i < 3; ++i) {
        TokenIds prompt = {1, 2, 3};
        RequestId req_id = test_path->inject_request(prompt, params);
        request_ids.push_back(req_id);
        std::cout << "Injected request: " << req_id << std::endl;
    }
    
    // Collect responses
    std::cout << "\nWaiting for responses..." << std::endl;
    for (const auto& req_id : request_ids) {
        TestResponse response = test_path->get_response(req_id);
        std::cout << "Response for " << req_id << ": ";
        print_tokens("output", response.output_token_ids);
    }
    
    test_path->stop();
}

void demo_prefix_caching() {
    std::cout << "\n=== Demo 4: Prefix Caching ===" << std::endl;
    
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    cache_config.enable_prefix_caching = true;
    
    SchedulerConfig scheduler_config;
    
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
    // Two prompts with shared prefix
    TokenIds prompt1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    TokenIds prompt2 = {1, 2, 3, 4, 5, 6, 7, 8, 17, 18};  // Shares first 8 tokens
    
    SamplingParams params(0.0f, -1, 1.0f, 3, false);
    
    std::cout << "Prompt 1: 16 tokens" << std::endl;
    std::cout << "Prompt 2: 10 tokens (shares first 8 with prompt 1)" << std::endl;
    
    auto outputs = engine->generate({prompt1, prompt2}, params);
    
    std::cout << "\nGeneration complete!" << std::endl;
    std::cout << "Prompt 1 output: ";
    print_tokens("", outputs[0].output_token_ids);
    std::cout << "Prompt 2 output: ";
    print_tokens("", outputs[1].output_token_ids);
}

void demo_temperature_sampling() {
    std::cout << "\n=== Demo 5: Temperature Sampling ===" << std::endl;
    
    ModelConfig model_config;
    model_config.vocab_size = 32000;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 1024;
    
    SchedulerConfig scheduler_config;
    
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config);
    
    TokenIds prompt = {1, 2, 3, 4, 5};
    
    // Different temperatures
    std::vector<float> temperatures = {0.0f, 0.5f, 1.0f};
    
    for (float temp : temperatures) {
        SamplingParams params(temp, -1, 1.0f, 5, false);
        auto outputs = engine->generate({prompt}, params);
        
        std::cout << "Temperature " << temp << ": ";
        print_tokens("", outputs[0].output_token_ids);
    }
}

int main() {
    // Set log level
    Logger::instance().set_level(LogLevel::INFO);
    
    std::cout << "========================================" << std::endl;
    std::cout << "   pypto-serving Mock LLaMA Demo" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        demo_simple_generation();
        demo_batch_generation();
        demo_test_path();
        demo_prefix_caching();
        demo_temperature_sampling();
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "   All demos completed successfully!" << std::endl;
        std::cout << "========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
