#include "engine/model_runner_pto2.h"
#include "common/logger.h"
#include <thread>
#include <chrono>

namespace pypto {

PTO2ModelRunner::PTO2ModelRunner(const ModelConfig& model_config)
    : model_config_(model_config), rng_(42) {
    
    // Initialize PTO2 runtime
    runtime_ = std::make_unique<pto2::PTO2Runtime>();
    
    pto2::RuntimeConfig config;
    config.max_tasks = 131072;
    config.max_worker = 8;
    config.enable_profiling = false;
    
    if (!runtime_->init(config)) {
        LOG_ERROR("Failed to initialize PTO2 runtime");
        device_available_ = false;
    } else {
        device_available_ = runtime_->is_device_available();
    }
    
    if (device_available_) {
        LOG_INFO("PTO2ModelRunner initialized with device support");
    } else {
        LOG_INFO("PTO2ModelRunner initialized in mock mode (no device)");
    }
}

PTO2ModelRunner::~PTO2ModelRunner() {
}

bool PTO2ModelRunner::is_using_device() const {
    return device_available_;
}

std::vector<std::vector<float>> PTO2ModelRunner::run(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill) {
    
    if (device_available_) {
        return run_with_device(seqs, is_prefill);
    } else {
        return run_with_mock(seqs, is_prefill);
    }
}

std::vector<std::vector<float>> PTO2ModelRunner::run_with_device(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill) {
    
    // TODO: Implement actual PTO2 device execution
    // For now, fall back to mock
    LOG_DEBUG("PTO2ModelRunner: Device execution not yet implemented, using mock");
    return run_with_mock(seqs, is_prefill);
}

std::vector<std::vector<float>> PTO2ModelRunner::run_with_mock(
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
    
    LOG_DEBUG("PTO2ModelRunner (mock): " << (is_prefill ? "prefill" : "decode") 
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

std::vector<float> PTO2ModelRunner::generate_mock_logits() {
    std::vector<float> logits(model_config_.vocab_size);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 0; i < model_config_.vocab_size; ++i) {
        logits[i] = dist(rng_);
    }
    
    return logits;
}

void PTO2ModelRunner::simulate_delay(int num_tokens, bool is_prefill) {
    // Simulate realistic inference latency
    // Prefill: ~0.1ms per token
    // Decode: ~1ms per token (slower due to memory bandwidth)
    int delay_us = is_prefill ? (num_tokens * 100) : (num_tokens * 1000);
    
    // Cap delay for testing
    delay_us = std::min(delay_us, 10000);  // Max 10ms
    
    std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
}

} // namespace pypto
