#include "pto2_runtime.h"
#include "common/logger.h"
#include <cstring>
#include <algorithm>

namespace pypto {
namespace pto2 {

// Static callbacks for mock host API
static void* mock_device_malloc(size_t size) {
    // Allocate host memory as mock device memory
    return malloc(size);
}

static void mock_device_free(void* ptr) {
    free(ptr);
}

static void mock_copy_to_device(void* dst, const void* src, size_t size) {
    memcpy(dst, src, size);
}

static void mock_copy_from_device(void* dst, const void* src, size_t size) {
    memcpy(dst, src, size);
}

static void mock_device_memset(void* ptr, int value, size_t size) {
    memset(ptr, value, size);
}

PTO2Runtime::PTO2Runtime()
    : next_task_id_(0) {
}

PTO2Runtime::~PTO2Runtime() {
    // Cleanup mock device memory
    mock_device_memory_.clear();
}

bool PTO2Runtime::init(const RuntimeConfig& config, const PTO2Config& pto2_config) {
    config_ = config;
    pto2_config_ = pto2_config;
    
    // Setup based on execution mode
    switch (pto2_config_.mode) {
        case ExecutionMode::DEVICE:
            LOG_INFO("PTO2Runtime: Device mode");
            // TODO: Initialize real device
            break;
            
        case ExecutionMode::SIMULATOR:
            LOG_INFO("PTO2Runtime: Simulator mode (platform=" << pto2_config_.platform << ")");
            // TODO: Initialize simulator
            // For now, fall back to mock
            LOG_WARNING("Simulator not yet implemented, using mock");
            setup_mock_host_api();
            break;
            
        case ExecutionMode::MOCK:
            LOG_INFO("PTO2Runtime: Mock mode");
            setup_mock_host_api();
            break;
    }
    
    // Reserve task storage
    tasks_.reserve(config_.max_tasks);
    
    return true;
}

// Removed detect_device() - now using PTO2Config

void PTO2Runtime::setup_mock_host_api() {
    host_api_.device_malloc = mock_device_malloc;
    host_api_.device_free = mock_device_free;
    host_api_.copy_to_device = mock_copy_to_device;
    host_api_.copy_from_device = mock_copy_from_device;
    host_api_.device_memset = mock_device_memset;
}

int PTO2Runtime::add_task(uint64_t* args, int num_args, int func_id, CoreType core_type) {
    if (next_task_id_ >= config_.max_tasks) {
        last_error_ = "Task table full";
        LOG_ERROR("PTO2Runtime: " << last_error_);
        return -1;
    }
    
    if (num_args > 16) {
        last_error_ = "Too many arguments";
        LOG_ERROR("PTO2Runtime: " << last_error_);
        return -1;
    }
    
    Task task;
    task.task_id = next_task_id_;
    task.func_id = func_id;
    task.num_args = num_args;
    task.core_type = core_type;
    task.fanin = 0;
    task.fanout_count = 0;
    task.function_bin_addr = 0;
    
    if (args && num_args > 0) {
        memcpy(task.args, args, num_args * sizeof(uint64_t));
    }
    
    tasks_.push_back(task);
    return next_task_id_++;
}

void PTO2Runtime::add_successor(int from_task, int to_task) {
    if (from_task < 0 || from_task >= next_task_id_) {
        LOG_ERROR("PTO2Runtime: Invalid from_task " << from_task);
        return;
    }
    
    if (to_task < 0 || to_task >= next_task_id_) {
        LOG_ERROR("PTO2Runtime: Invalid to_task " << to_task);
        return;
    }
    
    Task& from = tasks_[from_task];
    Task& to = tasks_[to_task];
    
    if (from.fanout_count >= 512) {
        LOG_ERROR("PTO2Runtime: Fanout overflow for task " << from_task);
        return;
    }
    
    from.fanout[from.fanout_count++] = to_task;
    to.fanin++;
}

void* PTO2Runtime::device_malloc(size_t size) {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        return host_api_.device_malloc(size);
    }
    
    // TODO: Call actual PTO2 device/simulator malloc
    last_error_ = "Real device/simulator malloc not implemented";
    return nullptr;
}

void PTO2Runtime::device_free(void* ptr) {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        host_api_.device_free(ptr);
        return;
    }
    
    // TODO: Call actual PTO2 device/simulator free
}

void PTO2Runtime::copy_to_device(void* dst, const void* src, size_t size) {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        host_api_.copy_to_device(dst, src, size);
        return;
    }
    
    // TODO: Call actual PTO2 copy
}

void PTO2Runtime::copy_from_device(void* dst, const void* src, size_t size) {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        host_api_.copy_from_device(dst, src, size);
        return;
    }
    
    // TODO: Call actual PTO2 copy
}

bool PTO2Runtime::launch() {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        LOG_WARNING("PTO2Runtime: Cannot launch in mock mode");
        return false;
    }
    
    // TODO: Call actual PTO2 launch (device or simulator)
    last_error_ = "Real device/simulator launch not implemented";
    return false;
}

void PTO2Runtime::wait() {
    if (pto2_config_.mode == ExecutionMode::MOCK) {
        // Mock: nothing to wait for
        return;
    }
    
    // TODO: Call actual PTO2 wait
}

} // namespace pto2
} // namespace pypto
