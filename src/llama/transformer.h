#pragma once

#include "tensor.h"
#include "layers.h"
#include "attention.h"
#include "mlp.h"
#include "common/types.h"
#include <memory>
#include <vector>

namespace pypto {
namespace llama {

// Single Transformer Layer
class TransformerLayer {
public:
    TransformerLayer(const ModelConfig& config)
        : hidden_size_(config.hidden_size) {
        
        // Pre-attention norm
        input_layernorm_ = std::make_unique<RMSNorm>(config.hidden_size);
        
        // Self-attention
        self_attn_ = std::make_unique<Attention>(config);
        
        // Pre-MLP norm
        post_attention_layernorm_ = std::make_unique<RMSNorm>(config.hidden_size);
        
        // MLP
        mlp_ = std::make_unique<MLP>(config);
    }
    
    // Forward pass
    // hidden_states: [batch, seq_len, hidden_size]
    // kv_cache: KV cache for this layer
    // positions: position indices for RoPE
    // is_prefill: whether this is prefill or decode
    Tensor forward(const Tensor& hidden_states, KVCache& kv_cache,
                  const std::vector<int>& positions, bool is_prefill) {
        // Self-attention with residual connection
        Tensor normed = input_layernorm_->forward(hidden_states);
        Tensor attn_output = self_attn_->forward_with_cache(normed, kv_cache, positions, is_prefill);
        Tensor hidden = ops::add(hidden_states, attn_output);
        
        // MLP with residual connection
        normed = post_attention_layernorm_->forward(hidden);
        Tensor mlp_output = mlp_->forward(normed);
        hidden = ops::add(hidden, mlp_output);
        
        return hidden;
    }

private:
    int hidden_size_;
    
    std::unique_ptr<RMSNorm> input_layernorm_;
    std::unique_ptr<Attention> self_attn_;
    std::unique_ptr<RMSNorm> post_attention_layernorm_;
    std::unique_ptr<MLP> mlp_;
};

// Full LLaMA Model
class LLaMAModel {
public:
    LLaMAModel(const ModelConfig& config)
        : config_(config),
          vocab_size_(config.vocab_size),
          hidden_size_(config.hidden_size),
          num_layers_(config.num_layers) {
        
        // Token embeddings
        embed_tokens_ = std::make_unique<Embedding>(vocab_size_, hidden_size_);
        
        // Transformer layers
        layers_.reserve(num_layers_);
        for (int i = 0; i < num_layers_; ++i) {
            layers_.push_back(std::make_unique<TransformerLayer>(config));
        }
        
        // Final norm
        norm_ = std::make_unique<RMSNorm>(hidden_size_);
        
        // LM head (output projection to vocabulary)
        lm_head_ = std::make_unique<Linear>(hidden_size_, vocab_size_, false);
        
        // Initialize KV caches for all layers
        kv_caches_.reserve(num_layers_);
        for (int i = 0; i < num_layers_; ++i) {
            kv_caches_.emplace_back(
                1,  // batch_size (fixed to 1 for now)
                config.num_kv_heads,
                config.max_position_embeddings,
                config.head_dim
            );
        }
    }
    
    // Forward pass
    // token_ids: [batch * seq_len] - flattened token IDs
    // batch_size: batch size
    // seq_len: sequence length
    // positions: [batch * seq_len] - position indices
    // is_prefill: whether this is prefill or decode
    // Returns: logits [batch, seq_len, vocab_size]
    Tensor forward(const std::vector<int>& token_ids, int batch_size, int seq_len,
                  const std::vector<int>& positions, bool is_prefill) {
        // Embedding
        Tensor hidden_states = embed_tokens_->forward(token_ids, batch_size, seq_len);
        
        // Pass through all transformer layers
        for (int i = 0; i < num_layers_; ++i) {
            hidden_states = layers_[i]->forward(hidden_states, kv_caches_[i], positions, is_prefill);
        }
        
        // Final norm
        hidden_states = norm_->forward(hidden_states);
        
        // LM head to get logits
        Tensor logits = lm_head_->forward(hidden_states);
        
        return logits;
    }
    
    // Get logits for the last token (for sampling)
    // logits: [batch, seq_len, vocab_size]
    // Returns: [batch, vocab_size]
    Tensor get_last_token_logits(const Tensor& logits) {
        if (logits.shape.size() != 3) {
            throw std::runtime_error("logits must be 3D");
        }
        
        int batch = logits.shape[0];
        int seq_len = logits.shape[1];
        int vocab_size = logits.shape[2];
        
        Tensor last_logits({batch, vocab_size});
        
        for (int b = 0; b < batch; ++b) {
            for (int v = 0; v < vocab_size; ++v) {
                last_logits.at({b, v}) = logits.at({b, seq_len - 1, v});
            }
        }
        
        return last_logits;
    }
    
    // Reset KV caches (for new sequence)
    void reset_kv_caches() {
        for (auto& cache : kv_caches_) {
            cache.key_cache.fill(0.0f);
            cache.value_cache.fill(0.0f);
            std::fill(cache.seq_lens.begin(), cache.seq_lens.end(), 0);
        }
    }
    
    const ModelConfig& get_config() const {
        return config_;
    }

private:
    ModelConfig config_;
    int vocab_size_;
    int hidden_size_;
    int num_layers_;
    
    std::unique_ptr<Embedding> embed_tokens_;
    std::vector<std::unique_ptr<TransformerLayer>> layers_;
    std::unique_ptr<RMSNorm> norm_;
    std::unique_ptr<Linear> lm_head_;
    
    std::vector<KVCache> kv_caches_;
};

} // namespace llama
} // namespace pypto
