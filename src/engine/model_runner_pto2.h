#pragma once

#include "common/types.h"
#include "engine/sequence.h"
#include "pto2/pto2_runtime.h"
#include <vector>
#include <memory>
#include <random>

namespace pypto {

// PTO2-based Model Runner
// Falls back to mock implementation if device is not available
class PTO2ModelRunner {
public:
    explicit PTO2ModelRunner(const ModelConfig& model_config);
    ~PTO2ModelRunner();

    // Run inference on batch of sequences
    // Returns logits for each sequence [batch_size, vocab_size]
    std::vector<std::vector<float>> run(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);
    
    // Check if using real device or mock
    bool is_using_device() const;

private:
    ModelConfig model_config_;
    std::unique_ptr<pto2::PTO2Runtime> runtime_;
    bool device_available_;
    
    // Mock fallback
    std::mt19937 rng_;
    
    // Helper: run with PTO2 device
    std::vector<std::vector<float>> run_with_device(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);
    
    // Helper: run with mock (fallback)
    std::vector<std::vector<float>> run_with_mock(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);
    
    // Helper: generate mock logits
    std::vector<float> generate_mock_logits();
    
    // Helper: simulate compute delay
    void simulate_delay(int num_tokens, bool is_prefill);
};

} // namespace pypto
