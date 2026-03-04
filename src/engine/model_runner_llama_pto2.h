#pragma once

#include "common/types.h"
#include "engine/sequence.h"
#include "llama_pto2/transformer_pto2.h"
#include <vector>
#include <memory>

// Forward declare PTO2Runtime
struct PTO2Runtime;

namespace pypto {

// LLaMA Model Runner with PTO2 backend
class LLaMAModelRunnerPTO2 {
public:
    explicit LLaMAModelRunnerPTO2(const ModelConfig& model_config);
    ~LLaMAModelRunnerPTO2();

    // Run inference on batch of sequences
    // Returns logits for each sequence [batch_size, vocab_size]
    std::vector<std::vector<float>> run(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);
    
    // Check if using PTO2 runtime
    bool is_using_pto2() const { return runtime_ != nullptr; }
    
    // Get execution mode
    std::string get_execution_mode() const;

private:
    ModelConfig model_config_;
    
    // PTO2 runtime (raw pointer to avoid incomplete type issues)
    PTO2Runtime* runtime_;
    
    // LLaMA model
    std::unique_ptr<llama_pto2::LLaMAModelPTO2> model_;
    
    // Helper: initialize PTO2 runtime
    bool init_pto2_runtime();
    
    // Helper: extract logits from tensor
    std::vector<std::vector<float>> extract_logits(
        const llama_pto2::TensorPTO2& logits_tensor,
        int batch_size);
};

} // namespace pypto
