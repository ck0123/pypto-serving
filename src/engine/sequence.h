#pragma once

#include "common/types.h"
#include <vector>
#include <string>

namespace pypto {

// Sequence represents a single generation request
// Design reference: nano-vllm's Sequence class
class Sequence {
public:
    Sequence(const RequestId& request_id, 
             const TokenIds& prompt_token_ids, 
             const SamplingParams& sampling_params);

    // Accessors
    const RequestId& request_id() const { return request_id_; }
    const TokenIds& prompt_token_ids() const { return prompt_token_ids_; }
    const TokenIds& output_token_ids() const { return output_token_ids_; }
    
    // Get all tokens (prompt + output)
    TokenIds all_token_ids() const;
    
    // Status management
    SequenceStatus status() const { return status_; }
    void set_status(SequenceStatus status) { status_ = status; }
    bool is_finished() const { return status_ == SequenceStatus::FINISHED; }
    
    // Token management
    void append_token(TokenId token_id);
    int num_prompt_tokens() const { return static_cast<int>(prompt_token_ids_.size()); }
    int num_output_tokens() const { return static_cast<int>(output_token_ids_.size()); }
    int num_total_tokens() const { return num_prompt_tokens() + num_output_tokens(); }
    
    // Block table management (for paged attention)
    const std::vector<BlockId>& block_table() const { return block_table_; }
    std::vector<BlockId>& block_table() { return block_table_; }
    int num_blocks() const { return static_cast<int>(block_table_.size()); }
    
    // Prefix caching support
    int num_cached_tokens() const { return num_cached_tokens_; }
    void set_num_cached_tokens(int n) { num_cached_tokens_ = n; }
    
    // Sampling parameters
    const SamplingParams& sampling_params() const { return sampling_params_; }
    
    // Finish reason
    FinishReason finish_reason() const { return finish_reason_; }
    void set_finish_reason(FinishReason reason) { finish_reason_ = reason; }
    
    // Check if should stop generation
    bool should_stop(TokenId eos_token_id) const;

private:
    RequestId request_id_;
    TokenIds prompt_token_ids_;
    TokenIds output_token_ids_;
    SamplingParams sampling_params_;
    
    SequenceStatus status_;
    FinishReason finish_reason_;
    
    // Block table for paged attention
    std::vector<BlockId> block_table_;
    
    // Number of tokens already in KV cache (for prefix caching)
    int num_cached_tokens_;
};

} // namespace pypto
