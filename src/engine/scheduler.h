#pragma once

#include "common/types.h"
#include "engine/sequence.h"
#include "radix/block_manager.h"
#include <deque>
#include <memory>
#include <vector>

namespace pypto {

// Scheduler manages sequence scheduling and memory allocation
// Design reference: nano-vllm's Scheduler
class Scheduler {
public:
    Scheduler(const ModelConfig& model_config,
              const CacheConfig& cache_config,
              const SchedulerConfig& scheduler_config);

    // Add new request to waiting queue
    void add_request(std::shared_ptr<Sequence> seq);
    
    // Check if all sequences are finished
    bool is_finished() const;
    
    // Schedule output
    struct ScheduleOutput {
        std::vector<std::shared_ptr<Sequence>> seqs;
        bool is_prefill;
        
        ScheduleOutput() : is_prefill(false) {}
    };
    
    // Schedule next batch
    ScheduleOutput schedule();
    
    // Post-process after model execution
    void postprocess(const std::vector<std::shared_ptr<Sequence>>& seqs,
                    const std::vector<TokenId>& token_ids);

private:
    ModelConfig model_config_;
    CacheConfig cache_config_;
    SchedulerConfig scheduler_config_;
    
    std::unique_ptr<BlockManager> block_manager_;
    
    // Queues
    std::deque<std::shared_ptr<Sequence>> waiting_;
    std::deque<std::shared_ptr<Sequence>> running_;
    
    // Helper: preempt a running sequence
    void preempt(std::shared_ptr<Sequence> seq);
    
    // Helper: try to schedule prefill
    ScheduleOutput schedule_prefill();
    
    // Helper: try to schedule decode
    ScheduleOutput schedule_decode();
};

} // namespace pypto
