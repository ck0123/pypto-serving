#pragma once

#include "tensor_pto2.h"
#include "common/types.h"
#include <memory>
#include <vector>

struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

// Base Layer class for PTO2
class LayerPTO2 {
public:
    virtual ~LayerPTO2() = default;
    
    // Forward pass (pure virtual)
    virtual TensorPTO2 forward(PTO2Runtime* runtime, const TensorPTO2& input) = 0;
};

// =============================================================================
// Embedding Layer
// =============================================================================

class EmbeddingPTO2 : public LayerPTO2 {
public:
    EmbeddingPTO2(int vocab_size, int embedding_dim)
        : vocab_size_(vocab_size),
          embedding_dim_(embedding_dim),
          weight_({vocab_size, embedding_dim}) {
        // Initialize with random weights
        weight_.randn(0.0f, 0.02f);
    }
    
    // Forward: token_ids [batch_size] -> embeddings [batch_size, embedding_dim]
    TensorPTO2 forward(PTO2Runtime* runtime, const TensorPTO2& token_ids) override;
    
    // Forward with explicit batch size and sequence length
    TensorPTO2 forward(PTO2Runtime* runtime, const std::vector<int>& token_ids);
    
    TensorPTO2& weight() { return weight_; }

private:
    int vocab_size_;
    int embedding_dim_;
    TensorPTO2 weight_;  // [vocab_size, embedding_dim]
};

// =============================================================================
// RMSNorm Layer
// =============================================================================

class RMSNormPTO2 : public LayerPTO2 {
public:
    explicit RMSNormPTO2(int hidden_size, float eps = 1e-6f)
        : hidden_size_(hidden_size),
          eps_(eps),
          weight_({hidden_size}) {
        // Initialize with ones
        weight_.fill(1.0f);
    }
    
    // Forward: x [*, hidden_size] -> normalized [*, hidden_size]
    TensorPTO2 forward(PTO2Runtime* runtime, const TensorPTO2& x) override;
    
    TensorPTO2& weight() { return weight_; }

private:
    int hidden_size_;
    float eps_;
    TensorPTO2 weight_;  // [hidden_size]
};

// =============================================================================
// Linear Layer
// =============================================================================

class LinearPTO2 : public LayerPTO2 {
public:
    LinearPTO2(int in_features, int out_features, bool bias = false)
        : in_features_(in_features),
          out_features_(out_features),
          has_bias_(bias),
          weight_({out_features, in_features}) {
        // Initialize with random weights
        weight_.randn(0.0f, 0.02f);
        
        if (has_bias_) {
            bias_ = TensorPTO2({out_features});
            bias_.fill(0.0f);
        }
    }
    
    // Forward: x [*, in_features] -> y [*, out_features]
    TensorPTO2 forward(PTO2Runtime* runtime, const TensorPTO2& x) override;
    
    TensorPTO2& weight() { return weight_; }
    TensorPTO2& bias() { return bias_; }

private:
    int in_features_;
    int out_features_;
    bool has_bias_;
    TensorPTO2 weight_;  // [out_features, in_features]
    TensorPTO2 bias_;    // [out_features]
};

// =============================================================================
// RoPE (Rotary Position Embedding)
// =============================================================================

class RoPEPTO2 {
public:
    RoPEPTO2(int head_dim, int max_position_embeddings = 2048, float theta = 10000.0f)
        : head_dim_(head_dim),
          max_position_embeddings_(max_position_embeddings),
          theta_(theta) {
    }
    
    // Apply RoPE to query or key
    // x: [batch, num_heads, seq_len, head_dim]
    // positions: position indices for each token
    TensorPTO2 forward(PTO2Runtime* runtime, const TensorPTO2& x, const std::vector<int>& positions);

private:
    int head_dim_;
    int max_position_embeddings_;
    float theta_;
};

} // namespace llama_pto2
} // namespace pypto
