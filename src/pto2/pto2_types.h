#pragma once

#include <cstdint>
#include <cstddef>

namespace pypto {
namespace pto2 {

// Forward declarations for PTO2 types
// These mirror the types from simpler/runtime.h but are self-contained

enum class CoreType : uint8_t {
    AIC = 0,  // AICore
    AIV = 1   // AICPU (Vector)
};

// Task structure (simplified)
struct Task {
    int task_id;
    int func_id;
    int num_args;
    uint64_t args[16];  // Max 16 args
    uint64_t function_bin_addr;
    CoreType core_type;
    int fanin;
    int fanout_count;
    int fanout[512];  // Max 512 fanout
};

// Host API for device memory management
struct HostAPI {
    void* (*device_malloc)(size_t size);
    void (*device_free)(void* ptr);
    void (*copy_to_device)(void* dst, const void* src, size_t size);
    void (*copy_from_device)(void* dst, const void* src, size_t size);
    void (*device_memset)(void* ptr, int value, size_t size);
};

// Runtime configuration
struct RuntimeConfig {
    int max_tasks;
    int max_worker;
    bool enable_profiling;
    
    RuntimeConfig()
        : max_tasks(131072),
          max_worker(8),
          enable_profiling(false) {}
};

} // namespace pto2
} // namespace pypto
