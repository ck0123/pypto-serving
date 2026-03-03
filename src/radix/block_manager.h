#pragma once

#include "common/types.h"
#include "engine/sequence.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>

namespace pypto {

// Block represents a physical KV cache block
// Design reference: nano-vllm's Block class
struct Block {
    BlockId block_id;
    int ref_count;
    uint64_t hash;
    TokenIds token_ids;

    Block(BlockId id) : block_id(id), ref_count(0), hash(0) {}
};

// BlockManager manages allocation and deallocation of KV cache blocks
// Implements hash-based prefix caching (nano-vllm style)
// Design reference: nano-vllm's BlockManager
class BlockManager {
public:
    BlockManager(int num_blocks, int block_size);

    // Check if can allocate blocks for sequence
    bool can_allocate(const Sequence& seq) const;
    
    // Check if can append one more token to sequence
    bool can_append(const Sequence& seq) const;
    
    // Allocate blocks for sequence (with prefix caching)
    void allocate(Sequence& seq);
    
    // Deallocate blocks for sequence
    void deallocate(Sequence& seq);
    
    // Append token to sequence (may allocate new block)
    void may_append(Sequence& seq);
    
    // Statistics
    int num_free_blocks() const { return static_cast<int>(free_block_ids_.size()); }
    int num_used_blocks() const { return static_cast<int>(used_block_ids_.size()); }
    int block_size() const { return block_size_; }

private:
    int block_size_;  // Tokens per block
    std::vector<Block> blocks_;  // All blocks
    
    // Hash table for prefix caching
    std::unordered_map<uint64_t, BlockId> hash_to_block_id_;
    
    // Free and used block tracking
    std::deque<BlockId> free_block_ids_;
    std::unordered_set<BlockId> used_block_ids_;
    
    // Helper functions
    BlockId _allocate_block();
    void _deallocate_block(BlockId block_id);
    uint64_t compute_hash(const TokenIds& token_ids, uint64_t prefix_hash = 0) const;
};

} // namespace pypto
