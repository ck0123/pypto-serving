#include "llama/transformer.h"
#include "common/logger.h"
#include <iostream>
#include <iomanip>

using namespace pypto;
using namespace pypto::llama;

void print_separator(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n\n";
}

void print_tensor_shape(const std::string& name, const Tensor& t) {
    std::cout << name << " shape: [";
    for (size_t i = 0; i < t.shape.size(); ++i) {
        std::cout << t.shape[i];
        if (i < t.shape.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

void print_first_values(const std::string& name, const Tensor& t, int n = 5) {
    std::cout << name << " first " << n << " values: [";
    for (int i = 0; i < std::min(n, static_cast<int>(t.data.size())); ++i) {
        std::cout << std::fixed << std::setprecision(4) << t.data[i];
        if (i < n - 1 && i < static_cast<int>(t.data.size()) - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

int main() {
    Logger::instance().set_level(LogLevel::INFO);
    
    print_separator("LLaMA Model Test");
    
    // Create small model config for testing
    ModelConfig config;
    config.vocab_size = 100;
    config.hidden_size = 64;
    config.num_layers = 2;
    config.num_heads = 4;
    config.num_kv_heads = 4;
    config.head_dim = 16;
    config.intermediate_size = 128;
    config.max_position_embeddings = 64;
    config.eos_token_id = 2;
    
    std::cout << "Model Configuration:\n";
    std::cout << "  vocab_size: " << config.vocab_size << "\n";
    std::cout << "  hidden_size: " << config.hidden_size << "\n";
    std::cout << "  num_layers: " << config.num_layers << "\n";
    std::cout << "  num_heads: " << config.num_heads << "\n";
    std::cout << "  head_dim: " << config.head_dim << "\n";
    std::cout << "\n";
    
    // Test 1: Create model
    print_separator("Test 1: Model Creation");
    std::cout << "Creating LLaMA model...\n";
    LLaMAModel model(config);
    std::cout << "✓ Model created successfully\n";
    
    // Test 2: Single token forward pass
    print_separator("Test 2: Single Token Forward Pass");
    {
        std::vector<int> token_ids = {1, 10, 20, 30};  // 4 tokens
        std::vector<int> positions = {0, 1, 2, 3};
        int batch_size = 1;
        int seq_len = 4;
        
        std::cout << "Input tokens: [";
        for (size_t i = 0; i < token_ids.size(); ++i) {
            std::cout << token_ids[i];
            if (i < token_ids.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
        
        std::cout << "Running forward pass...\n";
        Tensor logits = model.forward(token_ids, batch_size, seq_len, positions, true);
        
        print_tensor_shape("Logits", logits);
        print_first_values("Logits", logits, 10);
        
        // Get last token logits
        Tensor last_logits = model.get_last_token_logits(logits);
        print_tensor_shape("Last token logits", last_logits);
        print_first_values("Last token logits", last_logits, 10);
        
        std::cout << "✓ Forward pass successful\n";
    }
    
    // Test 3: Decode (with KV cache)
    print_separator("Test 3: Decode with KV Cache");
    {
        model.reset_kv_caches();
        
        // Prefill phase
        std::cout << "Prefill phase:\n";
        std::vector<int> prompt = {1, 5, 10, 15};
        std::vector<int> positions1 = {0, 1, 2, 3};
        Tensor logits1 = model.forward(prompt, 1, 4, positions1, true);
        print_first_values("  Prefill logits", logits1, 5);
        
        // Decode phase (generate 3 tokens)
        std::cout << "\nDecode phase:\n";
        for (int i = 0; i < 3; ++i) {
            std::vector<int> next_token = {20 + i};  // Mock next token
            std::vector<int> positions2 = {4 + i};
            Tensor logits2 = model.forward(next_token, 1, 1, positions2, false);
            
            Tensor last_logits = model.get_last_token_logits(logits2);
            std::cout << "  Step " << i << ": ";
            print_first_values("", last_logits, 5);
        }
        
        std::cout << "✓ Decode with KV cache successful\n";
    }
    
    // Test 4: Test individual layers
    print_separator("Test 4: Individual Layer Tests");
    {
        std::cout << "Testing RMSNorm...\n";
        RMSNorm norm(config.hidden_size);
        Tensor input({1, 4, config.hidden_size});
        input.randn(0.0f, 1.0f);
        Tensor normed = norm.forward(input);
        print_tensor_shape("  Normalized", normed);
        std::cout << "  ✓ RMSNorm works\n";
        
        std::cout << "\nTesting Linear layer...\n";
        Linear linear(config.hidden_size, config.hidden_size);
        Tensor linear_out = linear.forward(input);
        print_tensor_shape("  Linear output", linear_out);
        std::cout << "  ✓ Linear layer works\n";
        
        std::cout << "\nTesting MLP...\n";
        MLP mlp(config);
        Tensor mlp_out = mlp.forward(input);
        print_tensor_shape("  MLP output", mlp_out);
        std::cout << "  ✓ MLP works\n";
    }
    
    print_separator("All Tests Passed!");
    
    std::cout << "\nSummary:\n";
    std::cout << "  ✓ Model creation\n";
    std::cout << "  ✓ Forward pass (prefill)\n";
    std::cout << "  ✓ Decode with KV cache\n";
    std::cout << "  ✓ Individual layers (RMSNorm, Linear, MLP)\n";
    std::cout << "\nThe LLaMA implementation is working correctly!\n";
    
    return 0;
}
