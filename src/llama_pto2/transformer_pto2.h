#pragma once

#include "tensor_pto2.h"
#include "layers_pto2.h"
#include "attention_pto2.h"
#include "mlp_pto2.h"
#include "common/types.h"
#include <memory>
#include <vector>

struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

// Single Transformer Layer
class TransformerLayerPTO2 {
public:
    TransformerLayerPTO2(const ModelConfig& config)
        : hidden_size_(config.hidden_size),
          input_layernorm_(config.hidden_size),
          self_attn_(config),
          post_attention_layernorm_(config.hidden_size),
          mlp_(config) {
    }
    
    // Forward pass
    // hidden_states: [batch, seq_len, hidden_size]
    // kv_cache: KV cache for this layer
    // positions: position indices for RoPE
    // is_prefill: whether this is prefill or decode
    TensorPTO2 forward(PTO2Runtime* runtime,
                       const TensorPTO2& hidden_states,
                       KVCachePTO2& kv_cache,
                       const std::vector<int>& positions,
                       bool is_prefill);
    
    // Access sub-layers (for weight loading)
    RMSNormPTO2& input_layernorm() { return input_layernorm_; }
    AttentionPTO2& self_attn() { return self_attn_; }
    RMSNormPTO2& post_attention_layernorm() { return post_attention_layernorm_; }
    MLPPTO2& mlp() { return mlp_; }

private:
    int hidden_size_;
    
    RMSNormPTO2 input_layernorm_;           // Pre-attention norm
    AttentionPTO2 self_attn_;                // Self-attention
    RMSNormPTO2 post_attention_layernorm_;  // Pre-MLP norm
    MLPPTO2 mlp_;                            // Feed-forward network
};

// Complete LLaMA Model
class LLaMAModelPTO2 {
public:
    explicit LLaMAModelPTO2(const ModelConfig& config);
    ~LLaMAModelPTO2();
    
    // Forward pass (prefill or decode)
    // token_ids: input token IDs
    // batch_idx: batch index (for KV cache)
    // seq_len: sequence length
    // positions: position indices for each token
    // is_prefill: whether this is prefill or decode
    // Returns: logits [batch_size, vocab_size]
    TensorPTO2 forward(PTO2Runtime* runtime,
                       const std::vector<int>& token_ids,
                       int batch_idx,
                       int seq_len,
                       const std::vector<int>& positions,
                       bool is_prefill);
    
    // Initialize KV caches
    void init_kv_caches(PTO2Runtime* runtime, int max_batch, int max_seq_len);
    
    // Reset KV caches
    void reset_kv_caches();
    
    // Get model config
    const ModelConfig& config() const { return config_; }
    
    // Access layers (for weight loading)
    EmbeddingPTO2& embed_tokens() { return *embed_tokens_; }
    std::vector<std::unique_ptr<TransformerLayerPTO2>>& layers() { return layers_; }
    RMSNormPTO2& norm() { return *norm_; }
    LinearPTO2& lm_head() { return *lm_head_; }

private:
    ModelConfig config_;
    
    // Model components
    std::unique_ptr<EmbeddingPTO2> embed_tokens_;                      // Token embeddings
    std::vector<std::unique_ptr<TransformerLayerPTO2>> layers_;        // Transformer layers
    std::unique_ptr<RMSNormPTO2> norm_;                                // Final norm
    std::unique_ptr<LinearPTO2> lm_head_;                              // LM head
    
    // KV caches (one per layer)
    std::vector<KVCachePTO2> kv_caches_;
};

} // namespace llama_pto2
} // namespace pypto
