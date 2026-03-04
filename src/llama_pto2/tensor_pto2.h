#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <string>

// Forward declare PTO2 types
struct PTO2Runtime;
struct PTOTensorRegion;

namespace pypto {
namespace llama_pto2 {

// Data type for tensors
enum class DataType {
    FLOAT32,
    FLOAT16,
    BFLOAT16,
    INT32,
    INT8
};

// PTO2-based Tensor
// Wraps PTO2 tensor regions and provides high-level operations
class TensorPTO2 {
public:
    // Constructors
    TensorPTO2();
    TensorPTO2(const std::vector<int>& shape, DataType dtype = DataType::FLOAT32);
    ~TensorPTO2();
    
    // Shape and metadata
    const std::vector<int>& shape() const { return shape_; }
    int ndim() const { return shape_.size(); }
    int size() const;  // Total number of elements
    int size_bytes() const;  // Total size in bytes
    DataType dtype() const { return dtype_; }
    
    // Memory management
    bool allocate_device_memory(PTO2Runtime* runtime);
    bool allocate_host_memory();
    bool copy_to_device(PTO2Runtime* runtime, const void* host_data);
    bool copy_from_device(PTO2Runtime* runtime, void* host_data);
    
    // Access device memory
    void* device_ptr() { return device_ptr_; }
    const void* device_ptr() const { return device_ptr_; }
    
    // Access host memory (if allocated)
    void* host_ptr() { return host_data_.data(); }
    const void* host_ptr() const { return host_data_.data(); }
    
    // Reshape (view only, no data copy)
    TensorPTO2 reshape(const std::vector<int>& new_shape) const;
    
    // Clone (deep copy)
    TensorPTO2 clone() const;
    
    // Fill with value (on host)
    void fill(float value);
    
    // Random initialization (on host)
    void randn(float mean = 0.0f, float stddev = 1.0f);
    
    // Get PTO2 tensor region (for submitting tasks)
    PTOTensorRegion* get_pto2_region() { return pto2_region_; }
    const PTOTensorRegion* get_pto2_region() const { return pto2_region_; }
    
    // Set PTO2 tensor region (for output tensors)
    void set_pto2_region(PTOTensorRegion* region);
    
    // Debug: print shape and info
    std::string to_string() const;

private:
    std::vector<int> shape_;
    DataType dtype_;
    
    // Device memory (PTO2 managed)
    void* device_ptr_;
    PTOTensorRegion* pto2_region_;
    
    // Host memory (optional, for debugging/initialization)
    std::vector<uint8_t> host_data_;
    
    // Helper: calculate element size
    int element_size() const;
};

// Tensor operations namespace
namespace ops {

// Matrix multiplication: C = A @ B
// A: [M, K], B: [K, N] -> C: [M, N]
TensorPTO2 matmul(PTO2Runtime* runtime, const TensorPTO2& A, const TensorPTO2& B);

// Element-wise addition: C = A + B
TensorPTO2 add(PTO2Runtime* runtime, const TensorPTO2& A, const TensorPTO2& B);

// Element-wise multiplication: C = A * B
TensorPTO2 mul(PTO2Runtime* runtime, const TensorPTO2& A, const TensorPTO2& B);

// SiLU activation: y = x * sigmoid(x)
TensorPTO2 silu(PTO2Runtime* runtime, const TensorPTO2& x);

// Softmax along last dimension
TensorPTO2 softmax(PTO2Runtime* runtime, const TensorPTO2& x);

// RMS Normalization
TensorPTO2 rmsnorm(PTO2Runtime* runtime, const TensorPTO2& x, const TensorPTO2& weight, float eps = 1e-6f);

// RoPE (Rotary Position Embedding)
TensorPTO2 rope(PTO2Runtime* runtime, const TensorPTO2& x, const std::vector<int>& positions, float theta = 10000.0f);

} // namespace ops

} // namespace llama_pto2
} // namespace pypto
