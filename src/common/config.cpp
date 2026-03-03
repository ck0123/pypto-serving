#include "common/config.h"
#include <iostream>

namespace pypto {

bool Config::load_from_file(const std::string& path) {
    // TODO: Implement JSON/YAML parsing
    std::cerr << "Config::load_from_file not implemented yet: " << path << std::endl;
    return false;
}

bool Config::validate() const {
    // Basic validation
    if (model_config.vocab_size <= 0) {
        std::cerr << "Invalid vocab_size: " << model_config.vocab_size << std::endl;
        return false;
    }
    
    if (cache_config.block_size <= 0) {
        std::cerr << "Invalid block_size: " << cache_config.block_size << std::endl;
        return false;
    }
    
    if (cache_config.num_gpu_blocks <= 0) {
        std::cerr << "Invalid num_gpu_blocks: " << cache_config.num_gpu_blocks << std::endl;
        return false;
    }
    
    if (scheduler_config.max_num_seqs <= 0) {
        std::cerr << "Invalid max_num_seqs: " << scheduler_config.max_num_seqs << std::endl;
        return false;
    }
    
    return true;
}

} // namespace pypto
