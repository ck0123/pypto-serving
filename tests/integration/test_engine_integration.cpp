#include "engine/engine.h"
#include "frontend/test_path.h"
#include <gtest/gtest.h>
#include <memory>

using namespace pypto;

class EngineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_config_.vocab_size = 100;
        model_config_.eos_token_id = 2;
        
        cache_config_.block_size = 4;
        cache_config_.num_gpu_blocks = 20;
        
        scheduler_config_.max_num_seqs = 4;
        scheduler_config_.max_num_batched_tokens = 64;
        
        engine_ = std::make_shared<Engine>(
            model_config_, cache_config_, scheduler_config_);
    }

    ModelConfig model_config_;
    CacheConfig cache_config_;
    SchedulerConfig scheduler_config_;
    std::shared_ptr<Engine> engine_;
};

TEST_F(EngineIntegrationTest, SingleRequest) {
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params(0.0f, -1, 1.0f, 3, false);  // Greedy, 3 tokens
    
    auto outputs = engine_->generate({prompt}, params);
    
    ASSERT_EQ(outputs.size(), 1);
    EXPECT_EQ(outputs[0].output_token_ids.size(), 3);
}

TEST_F(EngineIntegrationTest, MultipleRequests) {
    TokenIds prompt1 = {1, 2, 3};
    TokenIds prompt2 = {4, 5, 6};
    TokenIds prompt3 = {7, 8, 9};
    
    SamplingParams params(0.0f, -1, 1.0f, 2, false);
    
    auto outputs = engine_->generate({prompt1, prompt2, prompt3}, params);
    
    ASSERT_EQ(outputs.size(), 3);
    for (const auto& output : outputs) {
        EXPECT_EQ(output.output_token_ids.size(), 2);
    }
}

TEST_F(EngineIntegrationTest, DifferentPromptLengths) {
    TokenIds short_prompt = {1, 2};
    TokenIds medium_prompt = {1, 2, 3, 4, 5};
    TokenIds long_prompt = {1, 2, 3, 4, 5, 6, 7, 8};
    
    SamplingParams params(0.0f, -1, 1.0f, 3, false);
    
    auto outputs = engine_->generate(
        {short_prompt, medium_prompt, long_prompt}, params);
    
    ASSERT_EQ(outputs.size(), 3);
    for (const auto& output : outputs) {
        EXPECT_EQ(output.output_token_ids.size(), 3);
    }
}

TEST_F(EngineIntegrationTest, TestPathSingleRequest) {
    auto test_path = std::make_shared<TestPath>(engine_);
    test_path->start();
    
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params(0.0f, -1, 1.0f, 3, false);
    
    RequestId req_id = test_path->inject_request(prompt, params);
    TestResponse response = test_path->get_response(req_id);
    
    EXPECT_EQ(response.request_id, req_id);
    EXPECT_EQ(response.output_token_ids.size(), 3);
    EXPECT_TRUE(response.ready);
    
    test_path->stop();
}

TEST_F(EngineIntegrationTest, TestPathMultipleRequests) {
    auto test_path = std::make_shared<TestPath>(engine_);
    test_path->start();
    
    SamplingParams params(0.0f, -1, 1.0f, 2, false);
    
    // Inject multiple requests
    std::vector<RequestId> request_ids;
    for (int i = 0; i < 3; ++i) {
        TokenIds prompt = {1, 2, 3};
        RequestId req_id = test_path->inject_request(prompt, params);
        request_ids.push_back(req_id);
    }
    
    // Collect responses
    std::vector<TestResponse> responses;
    for (const auto& req_id : request_ids) {
        TestResponse response = test_path->get_response(req_id);
        responses.push_back(response);
    }
    
    ASSERT_EQ(responses.size(), 3);
    for (const auto& response : responses) {
        EXPECT_EQ(response.output_token_ids.size(), 2);
        EXPECT_TRUE(response.ready);
    }
    
    test_path->stop();
}

TEST_F(EngineIntegrationTest, PrefixCaching) {
    // Two requests with shared prefix
    TokenIds prompt1 = {1, 2, 3, 4, 5, 6, 7, 8};
    TokenIds prompt2 = {1, 2, 3, 4, 9, 10};  // Shares first 4 tokens
    
    SamplingParams params(0.0f, -1, 1.0f, 2, false);
    
    auto outputs = engine_->generate({prompt1, prompt2}, params);
    
    ASSERT_EQ(outputs.size(), 2);
    EXPECT_EQ(outputs[0].output_token_ids.size(), 2);
    EXPECT_EQ(outputs[1].output_token_ids.size(), 2);
}

TEST_F(EngineIntegrationTest, LongGeneration) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(0.0f, -1, 1.0f, 10, false);  // Generate 10 tokens
    
    auto outputs = engine_->generate({prompt}, params);
    
    ASSERT_EQ(outputs.size(), 1);
    EXPECT_EQ(outputs[0].output_token_ids.size(), 10);
}

TEST_F(EngineIntegrationTest, TemperatureSampling) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(1.0f, -1, 1.0f, 5, false);  // Temperature = 1.0
    
    auto outputs = engine_->generate({prompt}, params);
    
    ASSERT_EQ(outputs.size(), 1);
    EXPECT_EQ(outputs[0].output_token_ids.size(), 5);
    
    // Tokens should be different due to randomness
    // (This is a weak test, but validates sampling works)
}
