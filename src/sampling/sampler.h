#pragma once

#include "common/types.h"
#include <vector>
#include <random>

namespace pypto {

// Sampler performs token sampling from logits
// Supports greedy, temperature, top-k, top-p sampling
class Sampler {
public:
    explicit Sampler(int vocab_size, unsigned int seed = 42);

    // Sample single token from logits
    TokenId sample(const std::vector<float>& logits, const SamplingParams& params);
    
    // Sample batch of tokens
    std::vector<TokenId> sample_batch(
        const std::vector<std::vector<float>>& logits_batch,
        const std::vector<SamplingParams>& params_batch);

private:
    int vocab_size_;
    std::mt19937 rng_;
    
    // Sampling strategies
    TokenId greedy_sample(const std::vector<float>& logits);
    TokenId temperature_sample(const std::vector<float>& logits, float temperature);
    TokenId top_k_sample(const std::vector<float>& logits, int top_k, float temperature);
    TokenId top_p_sample(const std::vector<float>& logits, float top_p, float temperature);
    
    // Helper: apply softmax with temperature
    std::vector<float> apply_temperature(const std::vector<float>& logits, float temperature);
    
    // Helper: sample from probability distribution
    TokenId sample_from_probs(const std::vector<float>& probs);
};

} // namespace pypto
