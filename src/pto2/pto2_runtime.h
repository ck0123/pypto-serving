#pragma once

#include "pto2_types.h"
#include "pto2_config.h"
#include <vector>
#include <memory>
#include <string>

namespace pypto {
namespace pto2 {

// PTO2 Runtime wrapper
// This is a lightweight interface that can work with or without actual PTO2 hardware
class PTO2Runtime {
public:
    PTO2Runtime();
    ~PTO2Runtime();

    // Initialize runtime
    bool init(const RuntimeConfig& config = RuntimeConfig(),
              const PTO2Config& pto2_config = PTO2Config::auto_detect());
    
    // Check execution mode
    ExecutionMode get_execution_mode() const { return pto2_config_.mode; }
    bool is_device_available() const { return pto2_config_.mode == ExecutionMode::DEVICE; }
    bool is_simulator() const { return pto2_config_.mode == ExecutionMode::SIMULATOR; }
    bool is_mock() const { return pto2_config_.mode == ExecutionMode::MOCK; }
    
    // Task management
    int add_task(uint64_t* args, int num_args, int func_id, CoreType core_type = CoreType::AIV);
    void add_successor(int from_task, int to_task);
    
    // Memory management
    void* device_malloc(size_t size);
    void device_free(void* ptr);
    void copy_to_device(void* dst, const void* src, size_t size);
    void copy_from_device(void* dst, const void* src, size_t size);
    
    // Execution
    bool launch();
    void wait();
    
    // Query
    int get_task_count() const { return next_task_id_; }
    
    // Get last error
    std::string get_last_error() const { return last_error_; }

private:
    RuntimeConfig config_;
    PTO2Config pto2_config_;
    std::string last_error_;
    
    // Task management
    std::vector<Task> tasks_;
    int next_task_id_;
    
    // Host API (mock implementation for Mac)
    HostAPI host_api_;
    
    // Mock device memory pool
    std::vector<std::unique_ptr<uint8_t[]>> mock_device_memory_;
    
    // Helper: detect device availability
    bool detect_device();
    
    // Helper: setup mock host API
    void setup_mock_host_api();
};

} // namespace pto2
} // namespace pypto
