#pragma once

#include "tensor.h"
#include "common/types.h"
#include <memory>
#include <vector>
#include <cmath>

namespace pypto {
namespace llama {

// Base Layer class
class Layer {
public:
    virtual ~Layer() = default;
    
    // Forward pass
    virtual Tensor forward(const Tensor& input) = 0;
};

// Embedding Layer
class Embedding : public Layer {
public:
    Embedding(int vocab_size, int hidden_size)
        : vocab_size_(vocab_size), hidden_size_(hidden_size) {
        // Initialize embedding table with random values
        weight_ = Tensor({vocab_size, hidden_size});
        weight_.randn(0.0f, 0.02f);
    }
    
    // Forward: token_ids [batch, seq_len] -> embeddings [batch, seq_len, hidden_size]
    Tensor forward(const std::vector<int>& token_ids, int batch_size, int seq_len) {
        Tensor output({batch_size, seq_len, hidden_size_});
        
        for (int b = 0; b < batch_size; ++b) {
            for (int s = 0; s < seq_len; ++s) {
                int token_id = token_ids[b * seq_len + s];
                if (token_id < 0 || token_id >= vocab_size_) {
                    throw std::runtime_error("Token ID out of range");
                }
                
                // Copy embedding vector
                for (int h = 0; h < hidden_size_; ++h) {
                    output.at({b, s, h}) = weight_.at({token_id, h});
                }
            }
        }
        
        return output;
    }
    
    Tensor forward(const Tensor& input) override {
        throw std::runtime_error("Use forward(token_ids, batch_size, seq_len) instead");
    }

private:
    int vocab_size_;
    int hidden_size_;
    Tensor weight_;  // [vocab_size, hidden_size]
};

// RMSNorm Layer
class RMSNorm : public Layer {
public:
    RMSNorm(int hidden_size, float eps = 1e-6f)
        : hidden_size_(hidden_size), eps_(eps) {
        // Initialize weight (gamma) to ones
        weight_ = Tensor({hidden_size});
        weight_.fill(1.0f);
    }
    
    // Forward: input [batch, seq_len, hidden_size] -> output [batch, seq_len, hidden_size]
    Tensor forward(const Tensor& input) override {
        if (input.shape.size() != 3 || input.shape[2] != hidden_size_) {
            throw std::runtime_error("RMSNorm input shape mismatch");
        }
        
        int batch = input.shape[0];
        int seq_len = input.shape[1];
        
        Tensor output = input.clone();
        
        for (int b = 0; b < batch; ++b) {
            for (int s = 0; s < seq_len; ++s) {
                // Compute RMS
                float sum_sq = 0.0f;
                for (int h = 0; h < hidden_size_; ++h) {
                    float val = input.at({b, s, h});
                    sum_sq += val * val;
                }
                float rms = std::sqrt(sum_sq / hidden_size_ + eps_);
                
                // Normalize and scale
                for (int h = 0; h < hidden_size_; ++h) {
                    output.at({b, s, h}) = (input.at({b, s, h}) / rms) * weight_[h];
                }
            }
        }
        
        return output;
    }

private:
    int hidden_size_;
    float eps_;
    Tensor weight_;  // [hidden_size]
};

// Rotary Position Embedding (RoPE)
class RoPE {
public:
    RoPE(int head_dim, int max_position_embeddings = 2048, float theta = 10000.0f)
        : head_dim_(head_dim), max_position_embeddings_(max_position_embeddings), theta_(theta) {
        // Precompute frequency bases
        freqs_.resize(head_dim_ / 2);
        for (int i = 0; i < head_dim_ / 2; ++i) {
            freqs_[i] = 1.0f / std::pow(theta_, 2.0f * i / head_dim_);
        }
    }
    
    // Apply RoPE to query or key
    // input: [batch, num_heads, seq_len, head_dim]
    // positions: [batch, seq_len] - position indices
    Tensor apply(const Tensor& input, const std::vector<int>& positions) {
        if (input.shape.size() != 4) {
            throw std::runtime_error("RoPE input must be 4D");
        }
        
        int batch = input.shape[0];
        int num_heads = input.shape[1];
        int seq_len = input.shape[2];
        int head_dim = input.shape[3];
        
        if (head_dim != head_dim_) {
            throw std::runtime_error("RoPE head_dim mismatch");
        }
        
        Tensor output = input.clone();
        
        for (int b = 0; b < batch; ++b) {
            for (int h = 0; h < num_heads; ++h) {
                for (int s = 0; s < seq_len; ++s) {
                    int pos = positions[b * seq_len + s];
                    
                    // Apply rotation to each pair of dimensions
                    for (int d = 0; d < head_dim / 2; ++d) {
                        float freq = freqs_[d];
                        float angle = pos * freq;
                        float cos_val = std::cos(angle);
                        float sin_val = std::sin(angle);
                        
                        float x0 = input.at({b, h, s, 2 * d});
                        float x1 = input.at({b, h, s, 2 * d + 1});
                        
                        output.at({b, h, s, 2 * d}) = x0 * cos_val - x1 * sin_val;
                        output.at({b, h, s, 2 * d + 1}) = x0 * sin_val + x1 * cos_val;
                    }
                }
            }
        }
        
        return output;
    }

private:
    int head_dim_;
    int max_position_embeddings_;
    float theta_;
    std::vector<float> freqs_;
};

// Linear Layer (fully connected)
class Linear : public Layer {
public:
    Linear(int in_features, int out_features, bool bias = true)
        : in_features_(in_features), out_features_(out_features), has_bias_(bias) {
        // Initialize weight with Xavier initialization
        weight_ = Tensor({out_features, in_features});
        float std = std::sqrt(2.0f / (in_features + out_features));
        weight_.randn(0.0f, std);
        
        if (has_bias_) {
            bias_ = Tensor({out_features});
            bias_.fill(0.0f);
        }
    }
    
    // Forward: input [batch, seq_len, in_features] -> output [batch, seq_len, out_features]
    Tensor forward(const Tensor& input) override {
        if (input.shape.size() != 3 || input.shape[2] != in_features_) {
            throw std::runtime_error("Linear input shape mismatch");
        }
        
        int batch = input.shape[0];
        int seq_len = input.shape[1];
        
        Tensor output({batch, seq_len, out_features_});
        
        for (int b = 0; b < batch; ++b) {
            for (int s = 0; s < seq_len; ++s) {
                for (int o = 0; o < out_features_; ++o) {
                    float sum = 0.0f;
                    for (int i = 0; i < in_features_; ++i) {
                        sum += input.at({b, s, i}) * weight_.at({o, i});
                    }
                    if (has_bias_) {
                        sum += bias_[o];
                    }
                    output.at({b, s, o}) = sum;
                }
            }
        }
        
        return output;
    }

private:
    int in_features_;
    int out_features_;
    bool has_bias_;
    Tensor weight_;  // [out_features, in_features]
    Tensor bias_;    // [out_features]
};

} // namespace llama
} // namespace pypto
