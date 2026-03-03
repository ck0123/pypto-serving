#include "engine/scheduler.h"
#include "common/logger.h"
#include <algorithm>

namespace pypto {

Scheduler::Scheduler(const ModelConfig& model_config,
                     const CacheConfig& cache_config,
                     const SchedulerConfig& scheduler_config)
    : model_config_(model_config),
      cache_config_(cache_config),
      scheduler_config_(scheduler_config) {
    
    block_manager_ = std::make_unique<BlockManager>(
        cache_config.num_gpu_blocks,
        cache_config.block_size);
    
    LOG_INFO("Scheduler initialized: max_num_seqs=" << scheduler_config.max_num_seqs
             << ", max_num_batched_tokens=" << scheduler_config.max_num_batched_tokens);
}

void Scheduler::add_request(std::shared_ptr<Sequence> seq) {
    seq->set_status(SequenceStatus::WAITING);
    waiting_.push_back(seq);
    LOG_DEBUG("Added request " << seq->request_id() << " to waiting queue");
}

bool Scheduler::is_finished() const {
    return waiting_.empty() && running_.empty();
}

Scheduler::ScheduleOutput Scheduler::schedule() {
    // Try prefill first (prioritize new requests)
    if (!waiting_.empty()) {
        ScheduleOutput output = schedule_prefill();
        if (!output.seqs.empty()) {
            return output;
        }
    }
    
    // Fall back to decode
    return schedule_decode();
}

Scheduler::ScheduleOutput Scheduler::schedule_prefill() {
    ScheduleOutput output;
    output.is_prefill = true;
    
    // Try to schedule one prefill request
    while (!waiting_.empty()) {
        auto seq = waiting_.front();
        
        // Check if can allocate memory
        if (!block_manager_->can_allocate(*seq)) {
            LOG_DEBUG("Cannot allocate memory for prefill, need to preempt");
            
            // Try to preempt running sequences
            if (!running_.empty()) {
                auto victim = running_.back();
                preempt(victim);
                continue;
            } else {
                // No memory and nothing to preempt
                break;
            }
        }
        
        // Check token budget
        if (seq->num_prompt_tokens() > scheduler_config_.max_num_batched_tokens) {
            LOG_WARNING("Prompt too long: " << seq->num_prompt_tokens() 
                       << " > " << scheduler_config_.max_num_batched_tokens);
            waiting_.pop_front();
            seq->set_status(SequenceStatus::FINISHED);
            seq->set_finish_reason(FinishReason::LENGTH);
            continue;
        }
        
        // Allocate blocks
        block_manager_->allocate(*seq);
        seq->set_status(SequenceStatus::RUNNING);
        
        waiting_.pop_front();
        running_.push_back(seq);
        output.seqs.push_back(seq);
        
        LOG_DEBUG("Scheduled prefill for " << seq->request_id());
        
        // Only schedule one prefill at a time
        break;
    }
    
    return output;
}

Scheduler::ScheduleOutput Scheduler::schedule_decode() {
    ScheduleOutput output;
    output.is_prefill = false;
    
    if (running_.empty()) {
        return output;
    }
    
    // Try to schedule all running sequences for decode
    int total_tokens = 0;
    
    for (auto it = running_.begin(); it != running_.end(); ) {
        auto seq = *it;
        
        // Check if can append token
        if (!block_manager_->can_append(*seq)) {
            LOG_DEBUG("Cannot append token, preempting " << seq->request_id());
            preempt(seq);
            it = running_.erase(it);
            continue;
        }
        
        // Check token budget
        if (total_tokens + 1 > scheduler_config_.max_num_batched_tokens) {
            break;
        }
        
        // Check max sequences
        if (static_cast<int>(output.seqs.size()) >= scheduler_config_.max_num_seqs) {
            break;
        }
        
        output.seqs.push_back(seq);
        total_tokens += 1;
        ++it;
    }
    
    if (!output.seqs.empty()) {
        LOG_DEBUG("Scheduled decode for " << output.seqs.size() << " sequences");
    }
    
    return output;
}

void Scheduler::postprocess(const std::vector<std::shared_ptr<Sequence>>& seqs,
                           const std::vector<TokenId>& token_ids) {
    
    if (seqs.size() != token_ids.size()) {
        LOG_ERROR("Size mismatch: seqs=" << seqs.size() << ", tokens=" << token_ids.size());
        return;
    }
    
    for (size_t i = 0; i < seqs.size(); ++i) {
        auto seq = seqs[i];
        TokenId token_id = token_ids[i];
        
        // Append token
        seq->append_token(token_id);
        block_manager_->may_append(*seq);
        
        // Check if should stop
        if (seq->should_stop(model_config_.eos_token_id)) {
            seq->set_status(SequenceStatus::FINISHED);
            
            // Determine finish reason
            if (seq->num_output_tokens() >= seq->sampling_params().max_tokens) {
                seq->set_finish_reason(FinishReason::LENGTH);
            } else {
                seq->set_finish_reason(FinishReason::EOS);
            }
            
            // Deallocate blocks
            block_manager_->deallocate(*seq);
            
            // Remove from running queue
            running_.erase(
                std::remove(running_.begin(), running_.end(), seq),
                running_.end());
            
            LOG_DEBUG("Sequence " << seq->request_id() << " finished");
        }
    }
}

void Scheduler::preempt(std::shared_ptr<Sequence> seq) {
    LOG_DEBUG("Preempting sequence " << seq->request_id());
    
    seq->set_status(SequenceStatus::PREEMPTED);
    block_manager_->deallocate(*seq);
    
    // Move back to waiting queue
    waiting_.push_front(seq);
}

} // namespace pypto
