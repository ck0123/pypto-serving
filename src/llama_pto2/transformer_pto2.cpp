#include "transformer_pto2.h"
#include <stdexcept>

// Forward declare PTO2 types
struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

// =============================================================================
// TransformerLayer
// =============================================================================

TensorPTO2 TransformerLayerPTO2::forward(PTO2Runtime* runtime,
                                          const TensorPTO2& hidden_states,
                                          KVCachePTO2& kv_cache,
                                          const std::vector<int>& positions,
                                          bool is_prefill) {
    // Self-attention with residual connection
    TensorPTO2 normed = input_layernorm_.forward(runtime, hidden_states);
    TensorPTO2 attn_output = self_attn_.forward_with_cache(runtime, normed, kv_cache, positions, is_prefill);
    TensorPTO2 hidden = ops::add(runtime, hidden_states, attn_output);
    
    // MLP with residual connection
    normed = post_attention_layernorm_.forward(runtime, hidden);
    TensorPTO2 mlp_output = mlp_.forward(runtime, normed);
    hidden = ops::add(runtime, hidden, mlp_output);
    
    return hidden;
}

// =============================================================================
// LLaMAModel
// =============================================================================

LLaMAModelPTO2::LLaMAModelPTO2(const ModelConfig& config)
    : config_(config) {
    
    // Create embedding layer
    embed_tokens_ = std::make_unique<EmbeddingPTO2>(config.vocab_size, config.hidden_size);
    
    // Create transformer layers
    for (int i = 0; i < config.num_layers; ++i) {
        layers_.push_back(std::make_unique<TransformerLayerPTO2>(config));
    }
    
    // Create final norm
    norm_ = std::make_unique<RMSNormPTO2>(config.hidden_size);
    
    // Create LM head
    lm_head_ = std::make_unique<LinearPTO2>(config.hidden_size, config.vocab_size, false);
}

LLaMAModelPTO2::~LLaMAModelPTO2() {
}

void LLaMAModelPTO2::init_kv_caches(PTO2Runtime* runtime, int max_batch, int max_seq_len) {
    kv_caches_.clear();
    
    for (int i = 0; i < config_.num_layers; ++i) {
        KVCachePTO2 cache(max_batch, config_.num_kv_heads, max_seq_len, config_.head_dim);
        
        // Allocate device memory for caches
        cache.k_cache.allocate_device_memory(runtime);
        cache.v_cache.allocate_device_memory(runtime);
        
        kv_caches_.push_back(std::move(cache));
    }
}

void LLaMAModelPTO2::reset_kv_caches() {
    for (auto& cache : kv_caches_) {
        cache.current_seq_len = 0;
    }
}

TensorPTO2 LLaMAModelPTO2::forward(PTO2Runtime* runtime,
                                    const std::vector<int>& token_ids,
                                    int batch_idx,
                                    int seq_len,
                                    const std::vector<int>& positions,
                                    bool is_prefill) {
    // Token embeddings
    TensorPTO2 hidden_states = embed_tokens_->forward(runtime, token_ids);
    
    // Through transformer layers
    for (size_t i = 0; i < layers_.size(); ++i) {
        hidden_states = layers_[i]->forward(runtime, hidden_states, kv_caches_[i], positions, is_prefill);
    }
    
    // Final norm
    hidden_states = norm_->forward(runtime, hidden_states);
    
    // LM head
    TensorPTO2 logits = lm_head_->forward(runtime, hidden_states);
    
    // For decode, only return last token's logits
    if (!is_prefill && seq_len > 1) {
        // Extract last token: logits[:, -1, :]
        // For now, return full logits (mock)
    }
    
    return logits;
}

} // namespace llama_pto2
} // namespace pypto
