#include "sampling/sampler.h"
#include "common/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>

namespace pypto {

Sampler::Sampler(int vocab_size, unsigned int seed)
    : vocab_size_(vocab_size), rng_(seed) {
    LOG_INFO("Sampler initialized with vocab_size=" << vocab_size);
}

TokenId Sampler::sample(const std::vector<float>& logits, const SamplingParams& params) {
    if (static_cast<int>(logits.size()) != vocab_size_) {
        throw std::invalid_argument("Logits size mismatch");
    }
    
    // Greedy sampling (temperature == 0 or very close)
    if (params.temperature < 1e-6f) {
        return greedy_sample(logits);
    }
    
    // Top-k sampling
    if (params.top_k > 0) {
        return top_k_sample(logits, params.top_k, params.temperature);
    }
    
    // Top-p sampling
    if (params.top_p < 1.0f) {
        return top_p_sample(logits, params.top_p, params.temperature);
    }
    
    // Temperature sampling
    return temperature_sample(logits, params.temperature);
}

std::vector<TokenId> Sampler::sample_batch(
    const std::vector<std::vector<float>>& logits_batch,
    const std::vector<SamplingParams>& params_batch) {
    
    if (logits_batch.size() != params_batch.size()) {
        throw std::invalid_argument("Batch size mismatch");
    }
    
    std::vector<TokenId> tokens;
    tokens.reserve(logits_batch.size());
    
    for (size_t i = 0; i < logits_batch.size(); ++i) {
        tokens.push_back(sample(logits_batch[i], params_batch[i]));
    }
    
    return tokens;
}

TokenId Sampler::greedy_sample(const std::vector<float>& logits) {
    auto max_it = std::max_element(logits.begin(), logits.end());
    return static_cast<TokenId>(std::distance(logits.begin(), max_it));
}

TokenId Sampler::temperature_sample(const std::vector<float>& logits, float temperature) {
    std::vector<float> probs = apply_temperature(logits, temperature);
    return sample_from_probs(probs);
}

TokenId Sampler::top_k_sample(const std::vector<float>& logits, int top_k, float temperature) {
    // Create index-value pairs
    std::vector<std::pair<TokenId, float>> indexed_logits;
    indexed_logits.reserve(logits.size());
    for (size_t i = 0; i < logits.size(); ++i) {
        indexed_logits.emplace_back(static_cast<TokenId>(i), logits[i]);
    }
    
    // Partial sort to get top-k
    int k = std::min(top_k, static_cast<int>(logits.size()));
    std::partial_sort(indexed_logits.begin(), 
                     indexed_logits.begin() + k,
                     indexed_logits.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Extract top-k logits
    std::vector<float> top_k_logits(k);
    for (int i = 0; i < k; ++i) {
        top_k_logits[i] = indexed_logits[i].second;
    }
    
    // Apply temperature and sample
    std::vector<float> probs = apply_temperature(top_k_logits, temperature);
    int sampled_idx = sample_from_probs(probs);
    
    return indexed_logits[sampled_idx].first;
}

TokenId Sampler::top_p_sample(const std::vector<float>& logits, float top_p, float temperature) {
    // Create index-value pairs and sort by logit value
    std::vector<std::pair<TokenId, float>> indexed_logits;
    indexed_logits.reserve(logits.size());
    for (size_t i = 0; i < logits.size(); ++i) {
        indexed_logits.emplace_back(static_cast<TokenId>(i), logits[i]);
    }
    
    std::sort(indexed_logits.begin(), indexed_logits.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Apply temperature to get probabilities
    std::vector<float> sorted_logits;
    sorted_logits.reserve(indexed_logits.size());
    for (const auto& pair : indexed_logits) {
        sorted_logits.push_back(pair.second);
    }
    
    std::vector<float> probs = apply_temperature(sorted_logits, temperature);
    
    // Find top-p cutoff
    float cumsum = 0.0f;
    size_t cutoff = 0;
    for (size_t i = 0; i < probs.size(); ++i) {
        cumsum += probs[i];
        cutoff = i + 1;
        if (cumsum >= top_p) {
            break;
        }
    }
    
    // Renormalize probabilities
    probs.resize(cutoff);
    float sum = std::accumulate(probs.begin(), probs.end(), 0.0f);
    for (float& p : probs) {
        p /= sum;
    }
    
    // Sample from top-p distribution
    int sampled_idx = sample_from_probs(probs);
    return indexed_logits[sampled_idx].first;
}

std::vector<float> Sampler::apply_temperature(const std::vector<float>& logits, float temperature) {
    // Find max for numerical stability
    float max_logit = *std::max_element(logits.begin(), logits.end());
    
    // Apply temperature and softmax
    std::vector<float> probs(logits.size());
    float sum = 0.0f;
    
    for (size_t i = 0; i < logits.size(); ++i) {
        probs[i] = std::exp((logits[i] - max_logit) / temperature);
        sum += probs[i];
    }
    
    // Normalize
    for (float& p : probs) {
        p /= sum;
    }
    
    return probs;
}

TokenId Sampler::sample_from_probs(const std::vector<float>& probs) {
    std::discrete_distribution<TokenId> dist(probs.begin(), probs.end());
    return dist(rng_);
}

} // namespace pypto
