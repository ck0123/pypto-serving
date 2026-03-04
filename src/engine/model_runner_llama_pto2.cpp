#include "model_runner_llama_pto2.h"
#include <iostream>
#include <cstring>

// Forward declare PTO2 types and functions
extern "C" {
    enum PTO2RuntimeMode {
        PTO2_MODE_EXECUTE = 0,
        PTO2_MODE_SIMULATE = 1,
        PTO2_MODE_GRAPH_ONLY = 2
    };
    
    struct PTO2Runtime;
    
    PTO2Runtime* pto2_runtime_create(int mode);
    void pto2_runtime_destroy(PTO2Runtime* rt);
}

namespace pypto {

LLaMAModelRunnerPTO2::LLaMAModelRunnerPTO2(const ModelConfig& model_config)
    : model_config_(model_config), runtime_(nullptr) {
    
    // Initialize PTO2 runtime
    if (!init_pto2_runtime()) {
        std::cerr << "Warning: Failed to initialize PTO2 runtime, using mock mode" << std::endl;
    }
    
    // Create LLaMA model
    model_ = std::make_unique<llama_pto2::LLaMAModelPTO2>(model_config);
    
    // Initialize KV caches
    model_->init_kv_caches(runtime_, 32, 2048);  // max_batch=32, max_seq_len=2048
}

LLaMAModelRunnerPTO2::~LLaMAModelRunnerPTO2() {
    // Cleanup PTO2 runtime
    if (runtime_) {
        pto2_runtime_destroy(runtime_);
        runtime_ = nullptr;
    }
}

bool LLaMAModelRunnerPTO2::init_pto2_runtime() {
    // Create PTO2 runtime
    // For now, use simulation mode (no actual hardware)
    runtime_ = pto2_runtime_create(PTO2_MODE_SIMULATE);
    
    return runtime_ != nullptr;
}

std::string LLaMAModelRunnerPTO2::get_execution_mode() const {
    if (!runtime_) {
        return "MOCK";
    }
    
    // For now, always return SIMULATOR since we created it with PTO2_MODE_SIMULATE
    return "SIMULATOR";
}

std::vector<std::vector<float>> LLaMAModelRunnerPTO2::run(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill) {
    
    std::vector<std::vector<float>> all_logits;
    
    // Process each sequence
    for (size_t i = 0; i < seqs.size(); ++i) {
        auto seq = seqs[i];
        
        // Get token IDs
        std::vector<int> token_ids;
        std::vector<int> positions;
        
        if (is_prefill) {
            // Prefill: use prompt tokens
            token_ids = seq->prompt_token_ids();
            for (size_t j = 0; j < token_ids.size(); ++j) {
                positions.push_back(j);
            }
        } else {
            // Decode: use last generated token
            token_ids = {seq->all_token_ids().back()};
            positions = {static_cast<int>(seq->all_token_ids().size() - 1)};
        }
        
        // Run model forward
        llama_pto2::TensorPTO2 logits = model_->forward(
            runtime_,
            token_ids,
            i,  // batch_idx
            token_ids.size(),
            positions,
            is_prefill
        );
        
        // Extract logits for this sequence
        auto seq_logits = extract_logits(logits, 1);
        all_logits.push_back(seq_logits[0]);
    }
    
    return all_logits;
}

std::vector<std::vector<float>> LLaMAModelRunnerPTO2::extract_logits(
    const llama_pto2::TensorPTO2& logits_tensor,
    int batch_size) {
    
    std::vector<std::vector<float>> result;
    
    // logits_tensor: [batch, seq_len, vocab_size] or [batch, vocab_size]
    int vocab_size = model_config_.vocab_size;
    
    // For now, return mock logits
    for (int i = 0; i < batch_size; ++i) {
        std::vector<float> logits(vocab_size, 0.0f);
        
        // Generate some mock values
        for (int j = 0; j < vocab_size; ++j) {
            logits[j] = static_cast<float>(rand()) / RAND_MAX - 0.5f;
        }
        
        result.push_back(logits);
    }
    
    return result;
}

} // namespace pypto
