#pragma once

#include "common/types.h"
#include <vector>
#include <cstddef>

namespace pypto {

// Mock KV cache pool (placeholder for future PTO2 integration)
// In real implementation, this will manage device memory
class KVPoolMock {
public:
    KVPoolMock(int num_blocks, int block_size, int num_layers, int num_kv_heads, int head_dim);

    // Allocate a block (returns block_id)
    BlockId allocate();
    
    // Deallocate a block
    void deallocate(BlockId block_id);
    
    // Get statistics
    int num_free_blocks() const { return num_free_blocks_; }
    int num_used_blocks() const { return num_blocks_ - num_free_blocks_; }
    
    // Get memory info
    size_t total_memory_bytes() const;
    size_t used_memory_bytes() const;

private:
    int num_blocks_;
    int block_size_;
    int num_layers_;
    int num_kv_heads_;
    int head_dim_;
    int num_free_blocks_;
    
    // Mock: just track allocation status
    std::vector<bool> allocated_;
};

} // namespace pypto
