#include "radix/block_manager.h"
#include "common/logger.h"
#include <algorithm>
#include <functional>

namespace pypto {

BlockManager::BlockManager(int num_blocks, int block_size)
    : block_size_(block_size) {
    
    // Initialize all blocks
    blocks_.reserve(num_blocks);
    for (int i = 0; i < num_blocks; ++i) {
        blocks_.emplace_back(i);
        free_block_ids_.push_back(i);
    }
    
    LOG_INFO("BlockManager initialized: " << num_blocks << " blocks, " 
             << block_size << " tokens per block");
}

bool BlockManager::can_allocate(const Sequence& seq) const {
    int num_tokens = seq.num_total_tokens();
    int num_blocks_needed = (num_tokens + block_size_ - 1) / block_size_;
    
    // Account for prefix caching
    int num_cached_blocks = seq.num_cached_tokens() / block_size_;
    int num_new_blocks_needed = num_blocks_needed - num_cached_blocks;
    
    return num_free_blocks() >= num_new_blocks_needed;
}

bool BlockManager::can_append(const Sequence& seq) const {
    int num_tokens = seq.num_total_tokens() + 1;  // +1 for the new token
    int num_blocks_needed = (num_tokens + block_size_ - 1) / block_size_;
    int current_blocks = seq.num_blocks();
    
    return (num_blocks_needed == current_blocks) || (num_free_blocks() > 0);
}

void BlockManager::allocate(Sequence& seq) {
    const TokenIds all_tokens = seq.all_token_ids();
    int num_tokens = static_cast<int>(all_tokens.size());
    int num_blocks_needed = (num_tokens + block_size_ - 1) / block_size_;
    
    seq.block_table().clear();
    seq.set_num_cached_tokens(0);
    
    uint64_t prefix_hash = 0;
    int cached_tokens = 0;
    
    // Try to reuse blocks via prefix caching
    for (int block_idx = 0; block_idx < num_blocks_needed; ++block_idx) {
        int start_idx = block_idx * block_size_;
        int end_idx = std::min(start_idx + block_size_, num_tokens);
        
        TokenIds block_tokens(all_tokens.begin() + start_idx, all_tokens.begin() + end_idx);
        uint64_t block_hash = compute_hash(block_tokens, prefix_hash);
        
        // Check if this block is already cached
        auto it = hash_to_block_id_.find(block_hash);
        if (it != hash_to_block_id_.end()) {
            BlockId block_id = it->second;
            Block& block = blocks_[block_id];
            
            // Verify token match (hash collision check)
            if (block.token_ids == block_tokens) {
                block.ref_count++;
                seq.block_table().push_back(block_id);
                cached_tokens += static_cast<int>(block_tokens.size());
                prefix_hash = block_hash;
                continue;
            }
        }
        
        // Need to allocate a new block
        BlockId new_block_id = _allocate_block();
        Block& new_block = blocks_[new_block_id];
        new_block.token_ids = block_tokens;
        new_block.hash = block_hash;
        new_block.ref_count = 1;
        
        hash_to_block_id_[block_hash] = new_block_id;
        seq.block_table().push_back(new_block_id);
        prefix_hash = block_hash;
    }
    
    seq.set_num_cached_tokens(cached_tokens);
    
    LOG_DEBUG("Allocated " << num_blocks_needed << " blocks for seq " 
              << seq.request_id() << ", cached " << cached_tokens << " tokens");
}

void BlockManager::deallocate(Sequence& seq) {
    for (BlockId block_id : seq.block_table()) {
        _deallocate_block(block_id);
    }
    seq.block_table().clear();
    seq.set_num_cached_tokens(0);
    
    LOG_DEBUG("Deallocated blocks for seq " << seq.request_id());
}

void BlockManager::may_append(Sequence& seq) {
    int num_tokens = seq.num_total_tokens();
    int num_blocks_needed = (num_tokens + block_size_ - 1) / block_size_;
    int current_blocks = seq.num_blocks();
    
    if (num_blocks_needed > current_blocks) {
        // Need to allocate a new block
        BlockId new_block_id = _allocate_block();
        seq.block_table().push_back(new_block_id);
        
        Block& new_block = blocks_[new_block_id];
        new_block.ref_count = 1;
        
        LOG_DEBUG("Appended new block " << new_block_id << " to seq " << seq.request_id());
    }
}

BlockId BlockManager::_allocate_block() {
    if (free_block_ids_.empty()) {
        LOG_ERROR("Out of memory: no free blocks available");
        throw std::runtime_error("Out of memory: no free blocks");
    }
    
    BlockId block_id = free_block_ids_.front();
    free_block_ids_.pop_front();
    used_block_ids_.insert(block_id);
    
    return block_id;
}

void BlockManager::_deallocate_block(BlockId block_id) {
    Block& block = blocks_[block_id];
    block.ref_count--;
    
    if (block.ref_count == 0) {
        // Remove from hash table
        if (block.hash != 0) {
            hash_to_block_id_.erase(block.hash);
        }
        
        // Clear block data
        block.token_ids.clear();
        block.hash = 0;
        
        // Return to free pool
        used_block_ids_.erase(block_id);
        free_block_ids_.push_back(block_id);
    }
}

uint64_t BlockManager::compute_hash(const TokenIds& token_ids, uint64_t prefix_hash) const {
    // Simple hash function (FNV-1a variant)
    uint64_t hash = prefix_hash;
    for (TokenId token : token_ids) {
        hash ^= static_cast<uint64_t>(token);
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace pypto
