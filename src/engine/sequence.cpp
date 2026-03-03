#include "engine/sequence.h"

namespace pypto {

Sequence::Sequence(const RequestId& request_id,
                   const TokenIds& prompt_token_ids,
                   const SamplingParams& sampling_params)
    : request_id_(request_id),
      prompt_token_ids_(prompt_token_ids),
      sampling_params_(sampling_params),
      status_(SequenceStatus::WAITING),
      finish_reason_(FinishReason::NONE),
      num_cached_tokens_(0) {
}

TokenIds Sequence::all_token_ids() const {
    TokenIds all_tokens;
    all_tokens.reserve(prompt_token_ids_.size() + output_token_ids_.size());
    all_tokens.insert(all_tokens.end(), prompt_token_ids_.begin(), prompt_token_ids_.end());
    all_tokens.insert(all_tokens.end(), output_token_ids_.begin(), output_token_ids_.end());
    return all_tokens;
}

void Sequence::append_token(TokenId token_id) {
    output_token_ids_.push_back(token_id);
}

bool Sequence::should_stop(TokenId eos_token_id) const {
    // Check max_tokens limit
    if (num_output_tokens() >= sampling_params_.max_tokens) {
        return true;
    }
    
    // Check EOS token (unless ignore_eos is set)
    if (!sampling_params_.ignore_eos && 
        !output_token_ids_.empty() && 
        output_token_ids_.back() == eos_token_id) {
        return true;
    }
    
    return false;
}

} // namespace pypto
