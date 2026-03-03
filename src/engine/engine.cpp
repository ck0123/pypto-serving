#include "engine/engine.h"
#include "common/logger.h"
#include <unordered_map>

namespace pypto {

Engine::Engine(const ModelConfig& model_config,
               const CacheConfig& cache_config,
               const SchedulerConfig& scheduler_config,
               ModelRunnerType runner_type)
    : model_config_(model_config),
      cache_config_(cache_config),
      scheduler_config_(scheduler_config),
      runner_type_(runner_type),
      next_request_id_(0) {
    
    scheduler_ = std::make_unique<Scheduler>(
        model_config, cache_config, scheduler_config);
    
    // Initialize model runner based on type
    if (runner_type_ == ModelRunnerType::PTO2) {
        pto2_runner_ = std::make_unique<PTO2ModelRunner>(model_config);
        LOG_INFO("Engine initialized with PTO2 model runner");
    } else {
        mock_runner_ = std::make_unique<MockModelRunner>(model_config);
        LOG_INFO("Engine initialized with Mock model runner");
    }
    
    sampler_ = std::make_unique<Sampler>(model_config.vocab_size);
    
    LOG_INFO("Engine initialized");
}

void Engine::add_request(const RequestId& request_id,
                        const TokenIds& prompt_token_ids,
                        const SamplingParams& sampling_params) {
    
    auto seq = std::make_shared<Sequence>(request_id, prompt_token_ids, sampling_params);
    scheduler_->add_request(seq);
    
    LOG_INFO("Added request " << request_id << " with " 
             << prompt_token_ids.size() << " prompt tokens");
}

Engine::StepOutput Engine::step() {
    StepOutput output;
    
    // Schedule next batch
    auto schedule_output = scheduler_->schedule();
    
    if (schedule_output.seqs.empty()) {
        return output;
    }
    
    // Run model
    auto logits_batch = run_model(
        schedule_output.seqs,
        schedule_output.is_prefill);
    
    // Sample tokens
    std::vector<SamplingParams> params_batch;
    params_batch.reserve(schedule_output.seqs.size());
    for (const auto& seq : schedule_output.seqs) {
        params_batch.push_back(seq->sampling_params());
    }
    
    auto sampled_tokens = sampler_->sample_batch(logits_batch, params_batch);
    
    // Postprocess
    scheduler_->postprocess(schedule_output.seqs, sampled_tokens);
    
    // Collect finished sequences
    for (const auto& seq : schedule_output.seqs) {
        if (seq->is_finished()) {
            output.finished_request_ids.push_back(seq->request_id());
            output.output_token_ids.push_back(seq->output_token_ids());
        }
    }
    
    return output;
}

std::vector<Engine::GenerateOutput> Engine::generate(
    const std::vector<TokenIds>& prompts,
    const SamplingParams& sampling_params) {
    
    // Add all requests
    std::vector<RequestId> request_ids;
    for (size_t i = 0; i < prompts.size(); ++i) {
        RequestId req_id = generate_request_id();
        add_request(req_id, prompts[i], sampling_params);
        request_ids.push_back(req_id);
    }
    
    // Map to track results
    std::unordered_map<RequestId, GenerateOutput> results;
    
    // Run until all finished
    while (!is_finished()) {
        auto step_output = step();
        
        // Collect finished sequences
        for (size_t i = 0; i < step_output.finished_request_ids.size(); ++i) {
            const auto& req_id = step_output.finished_request_ids[i];
            GenerateOutput gen_output;
            gen_output.output_token_ids = step_output.output_token_ids[i];
            gen_output.finish_reason = FinishReason::LENGTH;  // Simplified
            results[req_id] = gen_output;
        }
    }
    
    // Return results in original order
    std::vector<GenerateOutput> outputs;
    outputs.reserve(request_ids.size());
    for (const auto& req_id : request_ids) {
        outputs.push_back(results[req_id]);
    }
    
    return outputs;
}

RequestId Engine::generate_request_id() {
    return "req_" + std::to_string(next_request_id_++);
}

bool Engine::is_using_pto2_device() const {
    if (runner_type_ == ModelRunnerType::PTO2 && pto2_runner_) {
        return pto2_runner_->is_using_device();
    }
    return false;
}

std::vector<std::vector<float>> Engine::run_model(
    const std::vector<std::shared_ptr<Sequence>>& seqs,
    bool is_prefill) {
    
    if (runner_type_ == ModelRunnerType::PTO2 && pto2_runner_) {
        return pto2_runner_->run(seqs, is_prefill);
    } else if (mock_runner_) {
        return mock_runner_->run(seqs, is_prefill);
    } else {
        LOG_ERROR("No model runner available");
        return {};
    }
}

} // namespace pypto
