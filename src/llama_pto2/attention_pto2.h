#pragma once

#include "tensor_pto2.h"
#include "layers_pto2.h"
#include "common/types.h"
#include <memory>
#include <vector>

struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

// KV Cache for a single layer
struct KVCachePTO2 {
    TensorPTO2 k_cache;  // [max_batch, num_kv_heads, max_seq_len, head_dim]
    TensorPTO2 v_cache;  // [max_batch, num_kv_heads, max_seq_len, head_dim]
    int current_seq_len;  // Current sequence length
    
    KVCachePTO2() : current_seq_len(0) {}
    
    KVCachePTO2(int max_batch, int num_kv_heads, int max_seq_len, int head_dim)
        : k_cache({max_batch, num_kv_heads, max_seq_len, head_dim}),
          v_cache({max_batch, num_kv_heads, max_seq_len, head_dim}),
          current_seq_len(0) {
    }
};

// Multi-Head Attention with GQA support
class AttentionPTO2 {
public:
    AttentionPTO2(const ModelConfig& config)
        : hidden_size_(config.hidden_size),
          num_heads_(config.num_heads),
          num_kv_heads_(config.num_kv_heads),
          head_dim_(config.head_dim),
          q_proj_(config.hidden_size, config.num_heads * config.head_dim, false),
          k_proj_(config.hidden_size, config.num_kv_heads * config.head_dim, false),
          v_proj_(config.hidden_size, config.num_kv_heads * config.head_dim, false),
          o_proj_(config.num_heads * config.head_dim, config.hidden_size, false) {
    }
    
    // Forward with KV cache
    // hidden_states: [batch, seq_len, hidden_size]
    // kv_cache: KV cache for this layer
    // positions: position indices for RoPE
    // is_prefill: whether this is prefill or decode
    TensorPTO2 forward_with_cache(PTO2Runtime* runtime,
                                   const TensorPTO2& hidden_states,
                                   KVCachePTO2& kv_cache,
                                   const std::vector<int>& positions,
                                   bool is_prefill);
    
    // Access projection layers (for weight loading)
    LinearPTO2& q_proj() { return q_proj_; }
    LinearPTO2& k_proj() { return k_proj_; }
    LinearPTO2& v_proj() { return v_proj_; }
    LinearPTO2& o_proj() { return o_proj_; }

private:
    int hidden_size_;
    int num_heads_;
    int num_kv_heads_;
    int head_dim_;
    
    LinearPTO2 q_proj_;  // Query projection
    LinearPTO2 k_proj_;  // Key projection
    LinearPTO2 v_proj_;  // Value projection
    LinearPTO2 o_proj_;  // Output projection
    
    RoPEPTO2 rope_ = RoPEPTO2(64, 2048, 10000.0f);  // Initialize with defaults
    
    // Helper: reshape for attention [batch, seq_len, num_heads, head_dim]
    TensorPTO2 reshape_for_attention(const TensorPTO2& x, int num_heads);
    
    // Helper: repeat KV for GQA
    TensorPTO2 repeat_kv(PTO2Runtime* runtime, const TensorPTO2& x, int n_rep);
    
    // Helper: compute attention scores
    TensorPTO2 compute_attention_scores(PTO2Runtime* runtime,
                                        const TensorPTO2& q,
                                        const TensorPTO2& k);
    
    // Helper: apply softmax with causal mask
    TensorPTO2 apply_attention_softmax(PTO2Runtime* runtime,
                                       const TensorPTO2& scores,
                                       bool is_prefill);
    
    // Helper: compute attention output
    TensorPTO2 compute_attention_output(PTO2Runtime* runtime,
                                        const TensorPTO2& attn_weights,
                                        const TensorPTO2& v);
};

} // namespace llama_pto2
} // namespace pypto
