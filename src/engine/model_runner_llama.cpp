#include "engine/model_runner_llama.h"
#include "common/logger.h"

namespace pypto {

LLaMAModelRunner::LLaMAModelRunner(const ModelConfig& model_config)
    : model_config_(model_config) {
    
    // Create LLaMA model
    model_ = std::make_unique<llama::LLaMAModel>(model_config);
    
    LOG_INFO("LLaMAModelRunner initialized with " << model_config.num_layers << " layers");
}

std::vector<std::vector<float>> LLaMAModelRunner::run(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill) {
    
    // Prepare input
    std::vector<int> token_ids;
    std::vector<int> positions;
    int batch_size, seq_len;
    
    prepare_input(seqs, is_prefill, token_ids, positions, batch_size, seq_len);
    
    LOG_DEBUG("LLaMAModelRunner: " << (is_prefill ? "prefill" : "decode") 
              << " batch_size=" << batch_size << " seq_len=" << seq_len);
    
    // Forward pass through model
    llama::Tensor logits = model_->forward(token_ids, batch_size, seq_len, positions, is_prefill);
    
    // Extract logits for each sequence
    return extract_logits(logits, batch_size);
}

void LLaMAModelRunner::prepare_input(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill,
    std::vector<int>& token_ids,
    std::vector<int>& positions,
    int& batch_size,
    int& seq_len) {
    
    batch_size = static_cast<int>(seqs.size());
    
    if (is_prefill) {
        // Prefill: use all prompt tokens
        // For simplicity, assume all sequences have same length (padding if needed)
        seq_len = 0;
        for (const auto& seq : seqs) {
            seq_len = std::max(seq_len, seq->num_prompt_tokens());
        }
        
        token_ids.resize(batch_size * seq_len);
        positions.resize(batch_size * seq_len);
        
        for (int b = 0; b < batch_size; ++b) {
            const auto& seq = seqs[b];
            const auto& prompt = seq->prompt_token_ids();
            
            for (int s = 0; s < seq_len; ++s) {
                if (s < static_cast<int>(prompt.size())) {
                    token_ids[b * seq_len + s] = prompt[s];
                    positions[b * seq_len + s] = s;
                } else {
                    // Padding (use pad_token_id)
                    token_ids[b * seq_len + s] = model_config_.pad_token_id;
                    positions[b * seq_len + s] = s;
                }
            }
        }
    } else {
        // Decode: use last generated token
        seq_len = 1;
        token_ids.resize(batch_size);
        positions.resize(batch_size);
        
        for (int b = 0; b < batch_size; ++b) {
            const auto& seq = seqs[b];
            const auto& all_tokens = seq->all_token_ids();
            
            if (all_tokens.empty()) {
                throw std::runtime_error("Decode: sequence has no tokens");
            }
            
            token_ids[b] = all_tokens.back();
            positions[b] = static_cast<int>(all_tokens.size()) - 1;
        }
    }
}

std::vector<std::vector<float>> LLaMAModelRunner::extract_logits(
    const llama::Tensor& logits,
    int batch_size) {
    
    // logits shape: [batch, seq_len, vocab_size]
    // We only need the last token's logits for sampling
    
    if (logits.shape.size() != 3) {
        throw std::runtime_error("Logits must be 3D tensor");
    }
    
    int seq_len = logits.shape[1];
    int vocab_size = logits.shape[2];
    
    std::vector<std::vector<float>> result;
    result.reserve(batch_size);
    
    for (int b = 0; b < batch_size; ++b) {
        std::vector<float> seq_logits(vocab_size);
        
        // Get logits for the last token
        for (int v = 0; v < vocab_size; ++v) {
            seq_logits[v] = logits.at({b, seq_len - 1, v});
        }
        
        result.push_back(std::move(seq_logits));
    }
    
    return result;
}

} // namespace pypto
