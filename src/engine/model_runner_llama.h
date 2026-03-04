#pragma once

#include "common/types.h"
#include "engine/sequence.h"
#include "llama/transformer.h"
#include <vector>
#include <memory>

namespace pypto {

// LLaMAModelRunner uses real LLaMA implementation (mock mode)
// This replaces random logits with actual forward pass through LLaMA layers
class LLaMAModelRunner {
public:
    explicit LLaMAModelRunner(const ModelConfig& model_config);

    // Run inference on batch of sequences
    // Returns logits for each sequence [batch_size, vocab_size]
    std::vector<std::vector<float>> run(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);

private:
    ModelConfig model_config_;
    std::unique_ptr<llama::LLaMAModel> model_;
    
    // Convert sequence tokens to model input
    void prepare_input(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill,
        std::vector<int>& token_ids,
        std::vector<int>& positions,
        int& batch_size,
        int& seq_len);
    
    // Extract logits for sampling
    std::vector<std::vector<float>> extract_logits(
        const llama::Tensor& logits,
        int batch_size);
};

} // namespace pypto
