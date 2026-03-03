#pragma once

#include "common/types.h"
#include "engine/scheduler.h"
#include "engine/model_runner_mock.h"
#include "engine/model_runner_pto2.h"
#include "engine/sequence.h"
#include "sampling/sampler.h"
#include <memory>
#include <vector>
#include <string>

namespace pypto {

// Model runner type selection
enum class ModelRunnerType {
    MOCK,   // Mock implementation (for testing)
    PTO2    // PTO2 device implementation
};

// Engine is the main orchestrator for LLM inference
// Design reference: nano-vllm's LLMEngine
class Engine {
public:
    Engine(const ModelConfig& model_config,
           const CacheConfig& cache_config,
           const SchedulerConfig& scheduler_config,
           ModelRunnerType runner_type = ModelRunnerType::MOCK);

    // Add new generation request
    void add_request(const RequestId& request_id,
                    const TokenIds& prompt_token_ids,
                    const SamplingParams& sampling_params);

    // Single step of generation
    struct StepOutput {
        std::vector<RequestId> finished_request_ids;
        std::vector<TokenIds> output_token_ids;
        
        StepOutput() = default;
    };
    
    StepOutput step();
    
    // Check if all requests are finished
    bool is_finished() const { return scheduler_->is_finished(); }
    
    // Synchronous generation (for simple use cases)
    struct GenerateOutput {
        TokenIds output_token_ids;
        FinishReason finish_reason;
        
        GenerateOutput() : finish_reason(FinishReason::NONE) {}
    };
    
    std::vector<GenerateOutput> generate(
        const std::vector<TokenIds>& prompts,
        const SamplingParams& sampling_params);

    // Check if using PTO2 device
    bool is_using_pto2_device() const;

private:
    ModelConfig model_config_;
    CacheConfig cache_config_;
    SchedulerConfig scheduler_config_;
    ModelRunnerType runner_type_;
    
    std::unique_ptr<Scheduler> scheduler_;
    std::unique_ptr<MockModelRunner> mock_runner_;
    std::unique_ptr<PTO2ModelRunner> pto2_runner_;
    std::unique_ptr<Sampler> sampler_;
    
    int next_request_id_;
    
    // Helper: generate unique request ID
    RequestId generate_request_id();
    
    // Helper: run model (dispatch to correct runner)
    std::vector<std::vector<float>> run_model(
        const std::vector<std::shared_ptr<Sequence>>& seqs,
        bool is_prefill);
};

} // namespace pypto
