#pragma once

#include "common/types.h"
#include "engine/sequence.h"
#include <vector>
#include <random>
#include <memory>

namespace pypto {

// MockModelRunner simulates model inference
// Returns random logits for testing
class MockModelRunner {
public:
    explicit MockModelRunner(const ModelConfig& model_config);

    // Run inference on batch of sequences
    // Returns logits for each sequence [batch_size, vocab_size]
    std::vector<std::vector<float>> run(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);

private:
    ModelConfig model_config_;
    std::mt19937 rng_;
    
    // Generate mock logits
    std::vector<float> generate_mock_logits();
    
    // Simulate compute delay
    void simulate_delay(int num_tokens, bool is_prefill);
};

} // namespace pypto
