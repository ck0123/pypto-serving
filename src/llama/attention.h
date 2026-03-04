#pragma once

#include "tensor.h"
#include "layers.h"
#include "common/types.h"
#include <memory>
#include <vector>
#include <cmath>

namespace pypto {
namespace llama {

// KV Cache for a single layer
struct KVCache {
    Tensor key_cache;    // [batch, num_kv_heads, max_seq_len, head_dim]
    Tensor value_cache;  // [batch, num_kv_heads, max_seq_len, head_dim]
    std::vector<int> seq_lens;  // Current sequence length for each batch
    
    KVCache(int batch_size, int num_kv_heads, int max_seq_len, int head_dim)
        : key_cache({batch_size, num_kv_heads, max_seq_len, head_dim}),
          value_cache({batch_size, num_kv_heads, max_seq_len, head_dim}),
          seq_lens(batch_size, 0) {
        key_cache.fill(0.0f);
        value_cache.fill(0.0f);
    }
    
    // Append new key/value to cache
    void append(const Tensor& key, const Tensor& value, int batch_idx) {
        if (key.shape.size() != 4 || value.shape.size() != 4) {
            throw std::runtime_error("KVCache append: key/value must be 4D");
        }
        
        int num_heads = key.shape[1];
        int new_tokens = key.shape[2];
        int head_dim = key.shape[3];
        
        int start_pos = seq_lens[batch_idx];
        
        // Copy key and value to cache
        for (int h = 0; h < num_heads; ++h) {
            for (int t = 0; t < new_tokens; ++t) {
                for (int d = 0; d < head_dim; ++d) {
                    key_cache.at({batch_idx, h, start_pos + t, d}) = key.at({0, h, t, d});
                    value_cache.at({batch_idx, h, start_pos + t, d}) = value.at({0, h, t, d});
                }
            }
        }
        
        seq_lens[batch_idx] += new_tokens;
    }
    
    // Get cached keys/values up to current position
    std::pair<Tensor, Tensor> get(int batch_idx) const {
        int seq_len = seq_lens[batch_idx];
        int num_heads = key_cache.shape[1];
        int head_dim = key_cache.shape[3];
        
        Tensor key({1, num_heads, seq_len, head_dim});
        Tensor value({1, num_heads, seq_len, head_dim});
        
        for (int h = 0; h < num_heads; ++h) {
            for (int s = 0; s < seq_len; ++s) {
                for (int d = 0; d < head_dim; ++d) {
                    key.at({0, h, s, d}) = key_cache.at({batch_idx, h, s, d});
                    value.at({0, h, s, d}) = value_cache.at({batch_idx, h, s, d});
                }
            }
        }
        
        return {key, value};
    }
};

// Multi-Head Attention with KV Cache
class Attention : public Layer {
public:
    Attention(const ModelConfig& config)
        : hidden_size_(config.hidden_size),
          num_heads_(config.num_heads),
          num_kv_heads_(config.num_kv_heads),
          head_dim_(config.head_dim),
          rope_(config.head_dim, config.max_position_embeddings, config.rope_theta) {
        
        // Q, K, V projections
        q_proj_ = std::make_unique<Linear>(hidden_size_, num_heads_ * head_dim_, false);
        k_proj_ = std::make_unique<Linear>(hidden_size_, num_kv_heads_ * head_dim_, false);
        v_proj_ = std::make_unique<Linear>(hidden_size_, num_kv_heads_ * head_dim_, false);
        
        // Output projection
        o_proj_ = std::make_unique<Linear>(num_heads_ * head_dim_, hidden_size_, false);
        
        // Scaling factor for attention scores
        scale_ = 1.0f / std::sqrt(static_cast<float>(head_dim_));
    }
    
    // Forward pass with KV cache
    // input: [batch, seq_len, hidden_size]
    // kv_cache: cache for this layer
    // positions: [batch * seq_len] - position indices for RoPE
    // is_prefill: whether this is prefill or decode
    Tensor forward_with_cache(const Tensor& input, KVCache& kv_cache,
                             const std::vector<int>& positions, bool is_prefill) {
        if (input.shape.size() != 3) {
            throw std::runtime_error("Attention input must be 3D");
        }
        
        int batch = input.shape[0];
        int seq_len = input.shape[1];
        
        // Project to Q, K, V
        Tensor q = q_proj_->forward(input);  // [batch, seq_len, num_heads * head_dim]
        Tensor k = k_proj_->forward(input);  // [batch, seq_len, num_kv_heads * head_dim]
        Tensor v = v_proj_->forward(input);  // [batch, seq_len, num_kv_heads * head_dim]
        
        // Reshape to [batch, num_heads, seq_len, head_dim]
        q = reshape_for_attention(q, batch, seq_len, num_heads_, head_dim_);
        k = reshape_for_attention(k, batch, seq_len, num_kv_heads_, head_dim_);
        v = reshape_for_attention(v, batch, seq_len, num_kv_heads_, head_dim_);
        
        // Apply RoPE
        q = rope_.apply(q, positions);
        k = rope_.apply(k, positions);
        
        // Update KV cache (for simplicity, assume batch=1)
        if (batch != 1) {
            throw std::runtime_error("Attention currently only supports batch=1");
        }
        kv_cache.append(k, v, 0);
        
        // Get full KV from cache
        auto [cached_k, cached_v] = kv_cache.get(0);
        
        // Repeat KV heads if using GQA (Grouped Query Attention)
        if (num_kv_heads_ < num_heads_) {
            cached_k = repeat_kv(cached_k, num_heads_ / num_kv_heads_);
            cached_v = repeat_kv(cached_v, num_heads_ / num_kv_heads_);
        }
        
        // Compute attention scores: Q @ K^T
        // q: [batch, num_heads, seq_len, head_dim]
        // cached_k: [batch, num_heads, cache_len, head_dim]
        Tensor scores = compute_attention_scores(q, cached_k);  // [batch, num_heads, seq_len, cache_len]
        
        // Apply softmax
        scores = apply_attention_softmax(scores);
        
        // Compute attention output: scores @ V
        // scores: [batch, num_heads, seq_len, cache_len]
        // cached_v: [batch, num_heads, cache_len, head_dim]
        Tensor attn_output = compute_attention_output(scores, cached_v);  // [batch, num_heads, seq_len, head_dim]
        
        // Reshape back to [batch, seq_len, num_heads * head_dim]
        attn_output = reshape_from_attention(attn_output, batch, seq_len, num_heads_, head_dim_);
        
        // Output projection
        Tensor output = o_proj_->forward(attn_output);
        
        return output;
    }
    
    Tensor forward(const Tensor& input) override {
        throw std::runtime_error("Use forward_with_cache instead");
    }

private:
    int hidden_size_;
    int num_heads_;
    int num_kv_heads_;
    int head_dim_;
    float scale_;
    
    std::unique_ptr<Linear> q_proj_;
    std::unique_ptr<Linear> k_proj_;
    std::unique_ptr<Linear> v_proj_;
    std::unique_ptr<Linear> o_proj_;
    
    RoPE rope_;
    
    // Reshape [batch, seq_len, num_heads * head_dim] -> [batch, num_heads, seq_len, head_dim]
    Tensor reshape_for_attention(const Tensor& x, int batch, int seq_len, int num_heads, int head_dim) {
        Tensor output({batch, num_heads, seq_len, head_dim});
        
        for (int b = 0; b < batch; ++b) {
            for (int s = 0; s < seq_len; ++s) {
                for (int h = 0; h < num_heads; ++h) {
                    for (int d = 0; d < head_dim; ++d) {
                        output.at({b, h, s, d}) = x.at({b, s, h * head_dim + d});
                    }
                }
            }
        }
        
        return output;
    }
    
    // Reshape [batch, num_heads, seq_len, head_dim] -> [batch, seq_len, num_heads * head_dim]
    Tensor reshape_from_attention(const Tensor& x, int batch, int seq_len, int num_heads, int head_dim) {
        Tensor output({batch, seq_len, num_heads * head_dim});
        
        for (int b = 0; b < batch; ++b) {
            for (int s = 0; s < seq_len; ++s) {
                for (int h = 0; h < num_heads; ++h) {
                    for (int d = 0; d < head_dim; ++d) {
                        output.at({b, s, h * head_dim + d}) = x.at({b, h, s, d});
                    }
                }
            }
        }
        
        return output;
    }
    
    // Repeat KV heads for GQA
    Tensor repeat_kv(const Tensor& x, int n_rep) {
        if (n_rep == 1) return x.clone();
        
        int batch = x.shape[0];
        int num_kv_heads = x.shape[1];
        int seq_len = x.shape[2];
        int head_dim = x.shape[3];
        
        Tensor output({batch, num_kv_heads * n_rep, seq_len, head_dim});
        
        for (int b = 0; b < batch; ++b) {
            for (int h = 0; h < num_kv_heads; ++h) {
                for (int r = 0; r < n_rep; ++r) {
                    for (int s = 0; s < seq_len; ++s) {
                        for (int d = 0; d < head_dim; ++d) {
                            output.at({b, h * n_rep + r, s, d}) = x.at({b, h, s, d});
                        }
                    }
                }
            }
        }
        
        return output;
    }
    
    // Compute attention scores: Q @ K^T * scale
    Tensor compute_attention_scores(const Tensor& q, const Tensor& k) {
        int batch = q.shape[0];
        int num_heads = q.shape[1];
        int q_len = q.shape[2];
        int head_dim = q.shape[3];
        int k_len = k.shape[2];
        
        Tensor scores({batch, num_heads, q_len, k_len});
        
        for (int b = 0; b < batch; ++b) {
            for (int h = 0; h < num_heads; ++h) {
                for (int i = 0; i < q_len; ++i) {
                    for (int j = 0; j < k_len; ++j) {
                        float sum = 0.0f;
                        for (int d = 0; d < head_dim; ++d) {
                            sum += q.at({b, h, i, d}) * k.at({b, h, j, d});
                        }
                        scores.at({b, h, i, j}) = sum * scale_;
                    }
                }
            }
        }
        
        return scores;
    }
    
    // Apply softmax with causal mask
    Tensor apply_attention_softmax(const Tensor& scores) {
        int batch = scores.shape[0];
        int num_heads = scores.shape[1];
        int q_len = scores.shape[2];
        int k_len = scores.shape[3];
        
        Tensor output = scores.clone();
        
        for (int b = 0; b < batch; ++b) {
            for (int h = 0; h < num_heads; ++h) {
                for (int i = 0; i < q_len; ++i) {
                    // Find max for numerical stability
                    float max_val = -1e9f;
                    for (int j = 0; j <= k_len - q_len + i; ++j) {  // Causal mask
                        max_val = std::max(max_val, scores.at({b, h, i, j}));
                    }
                    
                    // Compute exp and sum
                    float sum = 0.0f;
                    for (int j = 0; j < k_len; ++j) {
                        if (j <= k_len - q_len + i) {  // Causal mask
                            float exp_val = std::exp(scores.at({b, h, i, j}) - max_val);
                            output.at({b, h, i, j}) = exp_val;
                            sum += exp_val;
                        } else {
                            output.at({b, h, i, j}) = 0.0f;
                        }
                    }
                    
                    // Normalize
                    for (int j = 0; j <= k_len - q_len + i; ++j) {
                        output.at({b, h, i, j}) /= sum;
                    }
                }
            }
        }
        
        return output;
    }
    
    // Compute attention output: scores @ V
    Tensor compute_attention_output(const Tensor& scores, const Tensor& v) {
        int batch = scores.shape[0];
        int num_heads = scores.shape[1];
        int q_len = scores.shape[2];
        int k_len = scores.shape[3];
        int head_dim = v.shape[3];
        
        Tensor output({batch, num_heads, q_len, head_dim});
        
        for (int b = 0; b < batch; ++b) {
            for (int h = 0; h < num_heads; ++h) {
                for (int i = 0; i < q_len; ++i) {
                    for (int d = 0; d < head_dim; ++d) {
                        float sum = 0.0f;
                        for (int j = 0; j < k_len; ++j) {
                            sum += scores.at({b, h, i, j}) * v.at({b, h, j, d});
                        }
                        output.at({b, h, i, d}) = sum;
                    }
                }
            }
        }
        
        return output;
    }
};

} // namespace llama
} // namespace pypto
