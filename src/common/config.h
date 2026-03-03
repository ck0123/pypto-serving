#pragma once

#include "types.h"
#include <string>

namespace pypto {

// Global configuration holder
class Config {
public:
    static Config& instance() {
        static Config config;
        return config;
    }

    // Model configuration
    ModelConfig model_config;
    
    // Cache configuration
    CacheConfig cache_config;
    
    // Scheduler configuration
    SchedulerConfig scheduler_config;

    // Load from file (future work)
    bool load_from_file(const std::string& path);
    
    // Validate configuration
    bool validate() const;

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

} // namespace pypto
