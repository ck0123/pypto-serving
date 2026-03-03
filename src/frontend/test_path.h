#pragma once

#include "common/types.h"
#include "engine/engine.h"
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <thread>
#include <atomic>

namespace pypto {

// Test request
struct TestRequest {
    RequestId request_id;
    TokenIds prompt_token_ids;
    SamplingParams sampling_params;
    
    TestRequest() = default;
    TestRequest(const RequestId& id, const TokenIds& prompt, const SamplingParams& params)
        : request_id(id), prompt_token_ids(prompt), sampling_params(params) {}
};

// Test response
struct TestResponse {
    RequestId request_id;
    TokenIds output_token_ids;
    FinishReason finish_reason;
    bool ready;
    
    TestResponse() : finish_reason(FinishReason::NONE), ready(false) {}
};

// TestPath provides a synchronous interface for testing
// Runs engine in background thread
class TestPath {
public:
    explicit TestPath(std::shared_ptr<Engine> engine);
    ~TestPath();

    // Inject a test request
    RequestId inject_request(const TokenIds& prompt_token_ids,
                            const SamplingParams& sampling_params);
    
    // Get response (blocks until ready)
    TestResponse get_response(const RequestId& request_id);
    
    // Start background worker
    void start();
    
    // Stop background worker
    void stop();

private:
    std::shared_ptr<Engine> engine_;
    
    std::mutex mutex_;
    std::condition_variable cv_;
    std::unordered_map<RequestId, TestResponse> responses_;
    
    std::atomic<bool> running_;
    std::thread worker_thread_;
    
    int next_request_id_;
    
    // Background worker loop
    void worker_loop();
    
    // Generate unique request ID
    RequestId generate_request_id();
};

} // namespace pypto
