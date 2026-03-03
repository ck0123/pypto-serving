#include "radix/block_manager.h"
#include "engine/sequence.h"
#include <gtest/gtest.h>

using namespace pypto;

class BlockManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        num_blocks_ = 10;
        block_size_ = 4;
        block_manager_ = std::make_unique<BlockManager>(num_blocks_, block_size_);
    }

    std::unique_ptr<BlockManager> block_manager_;
    int num_blocks_;
    int block_size_;
};

TEST_F(BlockManagerTest, Initialization) {
    EXPECT_EQ(block_manager_->num_free_blocks(), num_blocks_);
    EXPECT_EQ(block_manager_->num_used_blocks(), 0);
    EXPECT_EQ(block_manager_->block_size(), block_size_);
}

TEST_F(BlockManagerTest, CanAllocate) {
    TokenIds prompt = {1, 2, 3, 4, 5};  // 5 tokens = 2 blocks (4 + 1)
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    Sequence seq("req1", prompt, params);
    
    EXPECT_TRUE(block_manager_->can_allocate(seq));
}

TEST_F(BlockManagerTest, CannotAllocateOutOfMemory) {
    // Create sequence needing more blocks than available
    TokenIds prompt(50, 1);  // 50 tokens = 13 blocks > 10 available
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    Sequence seq("req1", prompt, params);
    
    EXPECT_FALSE(block_manager_->can_allocate(seq));
}

TEST_F(BlockManagerTest, AllocateAndDeallocate) {
    TokenIds prompt = {1, 2, 3, 4, 5};
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    Sequence seq("req1", prompt, params);
    
    EXPECT_EQ(block_manager_->num_free_blocks(), 10);
    
    block_manager_->allocate(seq);
    
    // 5 tokens = 2 blocks
    EXPECT_EQ(seq.num_blocks(), 2);
    EXPECT_EQ(block_manager_->num_free_blocks(), 8);
    EXPECT_EQ(block_manager_->num_used_blocks(), 2);
    
    block_manager_->deallocate(seq);
    
    EXPECT_EQ(seq.num_blocks(), 0);
    EXPECT_EQ(block_manager_->num_free_blocks(), 10);
    EXPECT_EQ(block_manager_->num_used_blocks(), 0);
}

TEST_F(BlockManagerTest, PrefixCaching) {
    TokenIds prompt1 = {1, 2, 3, 4, 5, 6, 7, 8};  // 8 tokens = 2 blocks
    TokenIds prompt2 = {1, 2, 3, 4, 9, 10};       // 6 tokens, shares first block
    
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    Sequence seq1("req1", prompt1, params);
    Sequence seq2("req2", prompt2, params);
    
    // Allocate first sequence
    block_manager_->allocate(seq1);
    EXPECT_EQ(seq1.num_blocks(), 2);
    EXPECT_EQ(block_manager_->num_free_blocks(), 8);
    
    // Allocate second sequence (should reuse first block)
    block_manager_->allocate(seq2);
    EXPECT_EQ(seq2.num_blocks(), 2);
    EXPECT_EQ(seq2.num_cached_tokens(), 4);  // First block cached
    EXPECT_EQ(block_manager_->num_free_blocks(), 7);  // Only 1 new block allocated
    
    // Deallocate seq1 (first block should still be in use by seq2)
    block_manager_->deallocate(seq1);
    EXPECT_EQ(block_manager_->num_free_blocks(), 8);  // Only second block freed
    
    // Deallocate seq2
    block_manager_->deallocate(seq2);
    EXPECT_EQ(block_manager_->num_free_blocks(), 10);  // All blocks freed
}

TEST_F(BlockManagerTest, CanAppend) {
    TokenIds prompt = {1, 2, 3};  // 3 tokens = 1 block
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    Sequence seq("req1", prompt, params);
    
    block_manager_->allocate(seq);
    EXPECT_EQ(seq.num_blocks(), 1);
    
    // Can append within same block
    EXPECT_TRUE(block_manager_->can_append(seq));
    
    seq.append_token(4);
    EXPECT_TRUE(block_manager_->can_append(seq));  // Still within block
    
    seq.append_token(5);
    EXPECT_TRUE(block_manager_->can_append(seq));  // Need new block, but available
}

TEST_F(BlockManagerTest, MayAppend) {
    TokenIds prompt = {1, 2, 3};  // 3 tokens = 1 block
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    Sequence seq("req1", prompt, params);
    
    block_manager_->allocate(seq);
    EXPECT_EQ(seq.num_blocks(), 1);
    EXPECT_EQ(block_manager_->num_free_blocks(), 9);
    
    // Append within same block
    seq.append_token(4);
    block_manager_->may_append(seq);
    EXPECT_EQ(seq.num_blocks(), 1);  // No new block needed
    EXPECT_EQ(block_manager_->num_free_blocks(), 9);
    
    // Append requiring new block
    seq.append_token(5);
    block_manager_->may_append(seq);
    EXPECT_EQ(seq.num_blocks(), 2);  // New block allocated
    EXPECT_EQ(block_manager_->num_free_blocks(), 8);
}

TEST_F(BlockManagerTest, MultipleSequences) {
    SamplingParams params(1.0f, -1, 1.0f, 10, false);
    
    Sequence seq1("req1", {1, 2, 3, 4}, params);
    Sequence seq2("req2", {5, 6, 7, 8}, params);
    Sequence seq3("req3", {9, 10}, params);
    
    block_manager_->allocate(seq1);
    EXPECT_EQ(seq1.num_blocks(), 1);
    EXPECT_EQ(block_manager_->num_free_blocks(), 9);
    
    block_manager_->allocate(seq2);
    EXPECT_EQ(seq2.num_blocks(), 1);
    EXPECT_EQ(block_manager_->num_free_blocks(), 8);
    
    block_manager_->allocate(seq3);
    EXPECT_EQ(seq3.num_blocks(), 1);
    EXPECT_EQ(block_manager_->num_free_blocks(), 7);
    
    // Deallocate middle sequence
    block_manager_->deallocate(seq2);
    EXPECT_EQ(block_manager_->num_free_blocks(), 8);
    
    // Deallocate all
    block_manager_->deallocate(seq1);
    block_manager_->deallocate(seq3);
    EXPECT_EQ(block_manager_->num_free_blocks(), 10);
}
