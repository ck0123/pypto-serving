#include "engine/sequence.h"
#include <gtest/gtest.h>

using namespace pypto;

class SequenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        request_id_ = "test_req_1";
        prompt_tokens_ = {1, 2, 3, 4, 5};
        sampling_params_ = SamplingParams(1.0f, -1, 1.0f, 10, false);
    }

    RequestId request_id_;
    TokenIds prompt_tokens_;
    SamplingParams sampling_params_;
};

TEST_F(SequenceTest, Initialization) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    EXPECT_EQ(seq.request_id(), request_id_);
    EXPECT_EQ(seq.prompt_token_ids(), prompt_tokens_);
    EXPECT_EQ(seq.output_token_ids().size(), 0);
    EXPECT_EQ(seq.status(), SequenceStatus::WAITING);
    EXPECT_EQ(seq.finish_reason(), FinishReason::NONE);
    EXPECT_EQ(seq.num_prompt_tokens(), 5);
    EXPECT_EQ(seq.num_output_tokens(), 0);
    EXPECT_EQ(seq.num_total_tokens(), 5);
    EXPECT_EQ(seq.num_cached_tokens(), 0);
    EXPECT_FALSE(seq.is_finished());
}

TEST_F(SequenceTest, AppendToken) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    seq.append_token(10);
    EXPECT_EQ(seq.num_output_tokens(), 1);
    EXPECT_EQ(seq.num_total_tokens(), 6);
    EXPECT_EQ(seq.output_token_ids()[0], 10);
    
    seq.append_token(11);
    seq.append_token(12);
    EXPECT_EQ(seq.num_output_tokens(), 3);
    EXPECT_EQ(seq.num_total_tokens(), 8);
}

TEST_F(SequenceTest, AllTokenIds) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    seq.append_token(10);
    seq.append_token(11);
    
    TokenIds all_tokens = seq.all_token_ids();
    EXPECT_EQ(all_tokens.size(), 7);
    EXPECT_EQ(all_tokens[0], 1);
    EXPECT_EQ(all_tokens[4], 5);
    EXPECT_EQ(all_tokens[5], 10);
    EXPECT_EQ(all_tokens[6], 11);
}

TEST_F(SequenceTest, StatusManagement) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    EXPECT_EQ(seq.status(), SequenceStatus::WAITING);
    EXPECT_FALSE(seq.is_finished());
    
    seq.set_status(SequenceStatus::RUNNING);
    EXPECT_EQ(seq.status(), SequenceStatus::RUNNING);
    EXPECT_FALSE(seq.is_finished());
    
    seq.set_status(SequenceStatus::FINISHED);
    EXPECT_EQ(seq.status(), SequenceStatus::FINISHED);
    EXPECT_TRUE(seq.is_finished());
}

TEST_F(SequenceTest, FinishReason) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    EXPECT_EQ(seq.finish_reason(), FinishReason::NONE);
    
    seq.set_finish_reason(FinishReason::LENGTH);
    EXPECT_EQ(seq.finish_reason(), FinishReason::LENGTH);
    
    seq.set_finish_reason(FinishReason::EOS);
    EXPECT_EQ(seq.finish_reason(), FinishReason::EOS);
}

TEST_F(SequenceTest, ShouldStopMaxTokens) {
    SamplingParams params(1.0f, -1, 1.0f, 3, false);  // max_tokens = 3
    Sequence seq(request_id_, prompt_tokens_, params);
    
    TokenId eos_token = 2;
    
    EXPECT_FALSE(seq.should_stop(eos_token));
    
    seq.append_token(10);
    EXPECT_FALSE(seq.should_stop(eos_token));
    
    seq.append_token(11);
    EXPECT_FALSE(seq.should_stop(eos_token));
    
    seq.append_token(12);
    EXPECT_TRUE(seq.should_stop(eos_token));  // Reached max_tokens
}

TEST_F(SequenceTest, ShouldStopEOS) {
    SamplingParams params(1.0f, -1, 1.0f, 10, false);  // ignore_eos = false
    Sequence seq(request_id_, prompt_tokens_, params);
    
    TokenId eos_token = 2;
    
    seq.append_token(10);
    EXPECT_FALSE(seq.should_stop(eos_token));
    
    seq.append_token(eos_token);
    EXPECT_TRUE(seq.should_stop(eos_token));  // Hit EOS
}

TEST_F(SequenceTest, ShouldStopIgnoreEOS) {
    SamplingParams params(1.0f, -1, 1.0f, 10, true);  // ignore_eos = true
    Sequence seq(request_id_, prompt_tokens_, params);
    
    TokenId eos_token = 2;
    
    seq.append_token(eos_token);
    EXPECT_FALSE(seq.should_stop(eos_token));  // EOS ignored
    
    // Should still stop at max_tokens
    for (int i = 0; i < 9; ++i) {
        seq.append_token(10);
    }
    EXPECT_TRUE(seq.should_stop(eos_token));
}

TEST_F(SequenceTest, BlockTable) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    EXPECT_EQ(seq.num_blocks(), 0);
    
    seq.block_table().push_back(0);
    seq.block_table().push_back(1);
    seq.block_table().push_back(2);
    
    EXPECT_EQ(seq.num_blocks(), 3);
    EXPECT_EQ(seq.block_table()[0], 0);
    EXPECT_EQ(seq.block_table()[1], 1);
    EXPECT_EQ(seq.block_table()[2], 2);
}

TEST_F(SequenceTest, CachedTokens) {
    Sequence seq(request_id_, prompt_tokens_, sampling_params_);
    
    EXPECT_EQ(seq.num_cached_tokens(), 0);
    
    seq.set_num_cached_tokens(3);
    EXPECT_EQ(seq.num_cached_tokens(), 3);
    
    seq.set_num_cached_tokens(5);
    EXPECT_EQ(seq.num_cached_tokens(), 5);
}
