#include "frontend/test_path.h"
#include "common/logger.h"
#include <chrono>

namespace pypto {

TestPath::TestPath(std::shared_ptr<Engine> engine)
    : engine_(engine), running_(false), next_request_id_(0) {
    LOG_INFO("TestPath initialized");
}

TestPath::~TestPath() {
    stop();
}

RequestId TestPath::inject_request(const TokenIds& prompt_token_ids,
                                  const SamplingParams& sampling_params) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RequestId req_id = generate_request_id();
    
    // Create response entry
    TestResponse response;
    response.request_id = req_id;
    response.ready = false;
    responses_[req_id] = response;
    
    // Add to engine
    engine_->add_request(req_id, prompt_token_ids, sampling_params);
    
    LOG_DEBUG("Injected request " << req_id);
    
    return req_id;
}

TestResponse TestPath::get_response(const RequestId& request_id) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until response is ready
    cv_.wait(lock, [this, &request_id]() {
        auto it = responses_.find(request_id);
        return it != responses_.end() && it->second.ready;
    });
    
    TestResponse response = responses_[request_id];
    responses_.erase(request_id);
    
    LOG_DEBUG("Retrieved response for " << request_id);
    
    return response;
}

void TestPath::start() {
    if (running_.load()) {
        LOG_WARNING("TestPath already running");
        return;
    }
    
    running_.store(true);
    worker_thread_ = std::thread(&TestPath::worker_loop, this);
    
    LOG_INFO("TestPath started");
}

void TestPath::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    LOG_INFO("TestPath stopped");
}

void TestPath::worker_loop() {
    LOG_DEBUG("TestPath worker loop started");
    
    while (running_.load()) {
        // Check if engine has work
        if (engine_->is_finished()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Execute one step
        auto step_output = engine_->step();
        
        // Update responses
        if (!step_output.finished_request_ids.empty()) {
            std::lock_guard<std::mutex> lock(mutex_);
            
            for (size_t i = 0; i < step_output.finished_request_ids.size(); ++i) {
                const auto& req_id = step_output.finished_request_ids[i];
                
                auto it = responses_.find(req_id);
                if (it != responses_.end()) {
                    it->second.output_token_ids = step_output.output_token_ids[i];
                    it->second.finish_reason = FinishReason::LENGTH;  // Simplified
                    it->second.ready = true;
                    
                    LOG_DEBUG("Request " << req_id << " finished");
                }
            }
            
            cv_.notify_all();
        }
        
        // Small sleep to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    
    LOG_DEBUG("TestPath worker loop stopped");
}

RequestId TestPath::generate_request_id() {
    return "test_req_" + std::to_string(next_request_id_++);
}

} // namespace pypto
