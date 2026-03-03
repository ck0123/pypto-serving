#include "engine/scheduler.h"
#include <gtest/gtest.h>

using namespace pypto;

class SchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_config_.vocab_size = 100;
        model_config_.eos_token_id = 2;
        
        cache_config_.block_size = 4;
        cache_config_.num_gpu_blocks = 10;
        
        scheduler_config_.max_num_seqs = 4;
        scheduler_config_.max_num_batched_tokens = 32;
        
        scheduler_ = std::make_unique<Scheduler>(
            model_config_, cache_config_, scheduler_config_);
    }

    ModelConfig model_config_;
    CacheConfig cache_config_;
    SchedulerConfig scheduler_config_;
    std::unique_ptr<Scheduler> scheduler_;
};

TEST_F(SchedulerTest, Initialization) {
    EXPECT_TRUE(scheduler_->is_finished());
}

TEST_F(SchedulerTest, AddRequest) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(1.0f, -1, 1.0f, 5, false);
    auto seq = std::make_shared<Sequence>("req1", prompt, params);
    
    scheduler_->add_request(seq);
    EXPECT_FALSE(scheduler_->is_finished());
    EXPECT_EQ(seq->status(), SequenceStatus::WAITING);
}

TEST_F(SchedulerTest, SchedulePrefill) {
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params(1.0f, -1, 1.0f, 5, false);
    auto seq = std::make_shared<Sequence>("req1", prompt, params);
    
    scheduler_->add_request(seq);
    
    auto output = scheduler_->schedule();
    
    EXPECT_TRUE(output.is_prefill);
    EXPECT_EQ(output.seqs.size(), 1);
    EXPECT_EQ(output.seqs[0]->request_id(), "req1");
    EXPECT_EQ(output.seqs[0]->status(), SequenceStatus::RUNNING);
}

TEST_F(SchedulerTest, ScheduleDecode) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(1.0f, -1, 1.0f, 5, false);
    auto seq = std::make_shared<Sequence>("req1", prompt, params);
    
    scheduler_->add_request(seq);
    
    // First schedule: prefill
    auto output1 = scheduler_->schedule();
    EXPECT_TRUE(output1.is_prefill);
    
    // Second schedule: decode
    auto output2 = scheduler_->schedule();
    EXPECT_FALSE(output2.is_prefill);
    EXPECT_EQ(output2.seqs.size(), 1);
    EXPECT_EQ(output2.seqs[0]->request_id(), "req1");
}

TEST_F(SchedulerTest, Postprocess) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(1.0f, -1, 1.0f, 5, false);
    auto seq = std::make_shared<Sequence>("req1", prompt, params);
    
    scheduler_->add_request(seq);
    
    // Prefill
    auto output1 = scheduler_->schedule();
    EXPECT_EQ(seq->num_output_tokens(), 0);
    
    // Postprocess with new token
    scheduler_->postprocess(output1.seqs, {10});
    EXPECT_EQ(seq->num_output_tokens(), 1);
    EXPECT_EQ(seq->output_token_ids()[0], 10);
}

TEST_F(SchedulerTest, FinishOnMaxTokens) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(1.0f, -1, 1.0f, 2, false);  // max_tokens = 2
    auto seq = std::make_shared<Sequence>("req1", prompt, params);
    
    scheduler_->add_request(seq);
    
    // Prefill
    scheduler_->schedule();
    
    // Generate 2 tokens
    auto output1 = scheduler_->schedule();
    scheduler_->postprocess(output1.seqs, {10});
    EXPECT_FALSE(seq->is_finished());
    
    auto output2 = scheduler_->schedule();
    scheduler_->postprocess(output2.seqs, {11});
    EXPECT_TRUE(seq->is_finished());
    EXPECT_EQ(seq->finish_reason(), FinishReason::LENGTH);
    EXPECT_TRUE(scheduler_->is_finished());
}

TEST_F(SchedulerTest, FinishOnEOS) {
    TokenIds prompt = {1, 2, 3};
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    auto seq = std::make_shared<Sequence>("req1", prompt, params);
    
    scheduler_->add_request(seq);
    
    // Prefill
    scheduler_->schedule();
    
    // Generate EOS token
    auto output = scheduler_->schedule();
    scheduler_->postprocess(output.seqs, {model_config_.eos_token_id});
    
    EXPECT_TRUE(seq->is_finished());
    EXPECT_EQ(seq->finish_reason(), FinishReason::EOS);
    EXPECT_TRUE(scheduler_->is_finished());
}

TEST_F(SchedulerTest, MultipleSequences) {
    SamplingParams params(1.0f, -1, 1.0f, 5, false);
    
    auto seq1 = std::make_shared<Sequence>("req1", TokenIds{1, 2}, params);
    auto seq2 = std::make_shared<Sequence>("req2", TokenIds{3, 4}, params);
    auto seq3 = std::make_shared<Sequence>("req3", TokenIds{5, 6}, params);
    
    scheduler_->add_request(seq1);
    scheduler_->add_request(seq2);
    scheduler_->add_request(seq3);
    
    // Schedule prefill for seq1
    auto output1 = scheduler_->schedule();
    EXPECT_TRUE(output1.is_prefill);
    EXPECT_EQ(output1.seqs.size(), 1);
    
    // Schedule prefill for seq2 (still prioritizes prefill)
    auto output2 = scheduler_->schedule();
    EXPECT_TRUE(output2.is_prefill);
    EXPECT_EQ(output2.seqs.size(), 1);
    
    // Schedule prefill for seq3 (still prioritizes prefill)
    auto output3 = scheduler_->schedule();
    EXPECT_TRUE(output3.is_prefill);
    EXPECT_EQ(output3.seqs.size(), 1);
    
    // Now all are running, schedule decode
    auto output4 = scheduler_->schedule();
    EXPECT_FALSE(output4.is_prefill);
    EXPECT_EQ(output4.seqs.size(), 3);
}

TEST_F(SchedulerTest, OutOfMemory) {
    SamplingParams params(1.0f, -1, 1.0f, 5, false);
    
    // Fill up memory with large prompts
    for (int i = 0; i < 10; ++i) {
        TokenIds prompt(8, i);  // 8 tokens = 2 blocks each
        auto seq = std::make_shared<Sequence>("req" + std::to_string(i), prompt, params);
        scheduler_->add_request(seq);
    }
    
    // Schedule prefills - scheduler will preempt to make room
    int prefill_count = 0;
    for (int i = 0; i < 10; ++i) {
        auto output = scheduler_->schedule();
        if (!output.is_prefill) {
            break;
        }
        prefill_count++;
    }
    
    // All 10 requests will eventually be scheduled (with preemption)
    EXPECT_EQ(prefill_count, 10);
}
