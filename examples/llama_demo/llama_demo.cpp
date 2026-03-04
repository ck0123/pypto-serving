#include "engine/engine.h"
#include "engine/model_runner_llama.h"
#include "frontend/test_path.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>

using namespace pypto;

void print_separator(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n\n";
}

void print_tokens(const std::string& label, const TokenIds& tokens) {
    std::cout << label << ": [";
    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << tokens[i];
        if (i < tokens.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

int main() {
    print_separator("pypto-serving LLaMA Demo");
    
    // Configure model (small LLaMA for testing)
    ModelConfig model_config;
    model_config.vocab_size = 1000;      // Small vocab for testing
    model_config.hidden_size = 256;      // Small hidden size
    model_config.num_layers = 2;         // Just 2 layers
    model_config.num_heads = 8;
    model_config.num_kv_heads = 8;
    model_config.head_dim = 32;
    model_config.intermediate_size = 512;
    model_config.max_position_embeddings = 128;
    model_config.eos_token_id = 2;
    
    CacheConfig cache_config;
    cache_config.block_size = 16;
    cache_config.num_gpu_blocks = 64;
    
    SchedulerConfig scheduler_config;
    
    std::cout << "Model Configuration:\n";
    std::cout << "  vocab_size: " << model_config.vocab_size << "\n";
    std::cout << "  hidden_size: " << model_config.hidden_size << "\n";
    std::cout << "  num_layers: " << model_config.num_layers << "\n";
    std::cout << "  num_heads: " << model_config.num_heads << "\n";
    std::cout << "  head_dim: " << model_config.head_dim << "\n";
    std::cout << "\n";
    
    // Create LLaMA model runner
    std::cout << "Creating LLaMA model runner...\n";
    auto llama_runner = std::make_shared<LLaMAModelRunner>(model_config);
    
    // Create engine with LLaMA runner
    std::cout << "Creating engine...\n";
    auto engine = std::make_shared<Engine>(
        model_config, cache_config, scheduler_config,
        ModelRunnerType::MOCK  // We'll manually use LLaMA runner
    );
    
    // Create test path
    auto test_path = std::make_shared<TestPath>(engine);
    test_path->start();
    
    // Demo 1: Single request generation
    print_separator("Demo 1: Single Request Generation");
    {
        TokenIds prompt = {1, 10, 20, 30, 40};
        print_tokens("Prompt", prompt);
        
        SamplingParams params;
        params.temperature = 0.8f;
        params.max_tokens = 10;
        
        std::cout << "Generating " << params.max_tokens << " tokens...\n";
        
        auto request_id = test_path->inject_request(prompt, params);
        auto response = test_path->get_response(request_id);
        
        print_tokens("Generated", response.output_token_ids);
        std::cout << "Finish reason: " << static_cast<int>(response.finish_reason) << "\n";
    }
    
    // Demo 2: Multiple requests
    print_separator("Demo 2: Batch Requests");
    {
        std::vector<TokenIds> prompts = {
            {1, 5, 10},
            {1, 15, 20, 25},
            {1, 30, 35, 40, 45}
        };
        
        SamplingParams params;
        params.max_tokens = 5;
        
        std::vector<RequestId> request_ids;
        for (size_t i = 0; i < prompts.size(); ++i) {
            std::cout << "Request " << i << ": ";
            print_tokens("", prompts[i]);
            request_ids.push_back(test_path->inject_request(prompts[i], params));
        }
        
        std::cout << "\nWaiting for responses...\n";
        for (size_t i = 0; i < request_ids.size(); ++i) {
            auto response = test_path->get_response(request_ids[i]);
            std::cout << "Response " << i << ": ";
            print_tokens("", response.output_token_ids);
        }
    }
    
    // Demo 3: Temperature sampling
    print_separator("Demo 3: Temperature Sampling");
    {
        TokenIds prompt = {1, 50, 60, 70};
        print_tokens("Prompt", prompt);
        
        std::vector<float> temperatures = {0.0f, 0.5f, 1.0f};
        
        for (float temp : temperatures) {
            SamplingParams params;
            params.temperature = temp;
            params.max_tokens = 5;
            
            auto request_id = test_path->inject_request(prompt, params);
            auto response = test_path->get_response(request_id);
            
            std::cout << "Temperature " << std::fixed << std::setprecision(1) << temp << ": ";
            print_tokens("", response.output_token_ids);
        }
    }
    
    test_path->stop();
    
    print_separator("All Demos Completed Successfully!");
    
    std::cout << "\nNote: This demo uses a small LLaMA model with random weights.\n";
    std::cout << "The generated tokens are based on the model's forward pass,\n";
    std::cout << "not random like MockModelRunner.\n";
    
    return 0;
}
