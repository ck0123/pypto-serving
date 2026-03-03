#include "radix/kv_pool_mock.h"
#include "common/logger.h"
#include <stdexcept>

namespace pypto {

KVPoolMock::KVPoolMock(int num_blocks, int block_size, int num_layers, 
                       int num_kv_heads, int head_dim)
    : num_blocks_(num_blocks),
      block_size_(block_size),
      num_layers_(num_layers),
      num_kv_heads_(num_kv_heads),
      head_dim_(head_dim),
      num_free_blocks_(num_blocks) {
    
    allocated_.resize(num_blocks, false);
    
    LOG_INFO("KVPoolMock initialized: " << num_blocks << " blocks, "
             << block_size << " tokens/block, "
             << num_layers << " layers, "
             << num_kv_heads << " KV heads, "
             << head_dim << " head dim");
}

BlockId KVPoolMock::allocate() {
    for (int i = 0; i < num_blocks_; ++i) {
        if (!allocated_[i]) {
            allocated_[i] = true;
            num_free_blocks_--;
            return i;
        }
    }
    
    throw std::runtime_error("KVPoolMock: out of memory");
}

void KVPoolMock::deallocate(BlockId block_id) {
    if (block_id < 0 || block_id >= num_blocks_) {
        throw std::invalid_argument("Invalid block_id");
    }
    
    if (!allocated_[block_id]) {
        LOG_WARNING("Attempting to deallocate already free block: " << block_id);
        return;
    }
    
    allocated_[block_id] = false;
    num_free_blocks_++;
}

size_t KVPoolMock::total_memory_bytes() const {
    // Each block stores: [num_layers, 2 (K/V), num_kv_heads, block_size, head_dim]
    // Assuming fp16 (2 bytes per element)
    size_t bytes_per_block = num_layers_ * 2 * num_kv_heads_ * block_size_ * head_dim_ * 2;
    return num_blocks_ * bytes_per_block;
}

size_t KVPoolMock::used_memory_bytes() const {
    return (num_blocks_ - num_free_blocks_) * (total_memory_bytes() / num_blocks_);
}

} // namespace pypto
