#include "engine/model_runner_mock.h"
#include "common/logger.h"
#include <thread>
#include <chrono>

namespace pypto {

MockModelRunner::MockModelRunner(const ModelConfig& model_config)
    : model_config_(model_config), rng_(42) {
    LOG_INFO("MockModelRunner initialized");
}

std::vector<std::vector<float>> MockModelRunner::run(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill) {
    
    int batch_size = static_cast<int>(seqs.size());
    int total_tokens = 0;
    
    for (const auto& seq : seqs) {
        if (is_prefill) {
            total_tokens += seq->num_prompt_tokens();
        } else {
            total_tokens += 1;  // Decode: one token per sequence
        }
    }
    
    LOG_DEBUG("MockModelRunner: " << (is_prefill ? "prefill" : "decode") 
              << " batch_size=" << batch_size << " total_tokens=" << total_tokens);
    
    // Simulate compute delay
    simulate_delay(total_tokens, is_prefill);
    
    // Generate mock logits for each sequence
    std::vector<std::vector<float>> logits_batch;
    logits_batch.reserve(batch_size);
    
    for (int i = 0; i < batch_size; ++i) {
        logits_batch.push_back(generate_mock_logits());
    }
    
    return logits_batch;
}

std::vector<float> MockModelRunner::generate_mock_logits() {
    std::vector<float> logits(model_config_.vocab_size);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 0; i < model_config_.vocab_size; ++i) {
        logits[i] = dist(rng_);
    }
    
    return logits;
}

void MockModelRunner::simulate_delay(int num_tokens, bool is_prefill) {
    // Simulate realistic inference latency
    // Prefill: ~0.1ms per token
    // Decode: ~1ms per token (slower due to memory bandwidth)
    int delay_us = is_prefill ? (num_tokens * 100) : (num_tokens * 1000);
    
    // Cap delay for testing
    delay_us = std::min(delay_us, 10000);  // Max 10ms
    
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
}

} // namespace pypto
