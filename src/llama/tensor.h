#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace pypto {
namespace llama {

// Simple Tensor class for mock implementation
// In real implementation, this would use PTO2 tensors
class Tensor {
public:
    // Shape: [batch, seq_len, hidden_size] or [batch, num_heads, seq_len, head_dim]
    std::vector<int> shape;
    std::vector<float> data;
    
    Tensor() = default;
    
    // Constructor with shape
    explicit Tensor(const std::vector<int>& shape_) : shape(shape_) {
        int total_size = 1;
        for (int dim : shape) {
            total_size *= dim;
        }
        data.resize(total_size, 0.0f);
    }
    
    // Constructor with shape and data
    Tensor(const std::vector<int>& shape_, const std::vector<float>& data_)
        : shape(shape_), data(data_) {
        int expected_size = 1;
        for (int dim : shape) {
            expected_size *= dim;
        }
        if (data.size() != expected_size) {
            throw std::runtime_error("Tensor data size mismatch");
        }
    }
    
    // Get total number of elements
    int numel() const {
        int total = 1;
        for (int dim : shape) {
            total *= dim;
        }
        return total;
    }
    
    // Get element at index (for 1D access)
    float& operator[](int idx) {
        return data[idx];
    }
    
    const float& operator[](int idx) const {
        return data[idx];
    }
    
    // Get element at multi-dimensional index
    float& at(const std::vector<int>& indices) {
        return data[flatten_index(indices)];
    }
    
    const float& at(const std::vector<int>& indices) const {
        return data[flatten_index(indices)];
    }
    
    // Reshape (must preserve total size)
    void reshape(const std::vector<int>& new_shape) {
        int new_size = 1;
        for (int dim : new_shape) {
            new_size *= dim;
        }
        if (new_size != numel()) {
            throw std::runtime_error("Reshape size mismatch");
        }
        shape = new_shape;
    }
    
    // Clone tensor
    Tensor clone() const {
        return Tensor(shape, data);
    }
    
    // Fill with value
    void fill(float value) {
        std::fill(data.begin(), data.end(), value);
    }
    
    // Fill with random values (for initialization)
    void randn(float mean = 0.0f, float stddev = 1.0f) {
        for (auto& val : data) {
            // Simple Box-Muller transform for normal distribution
            float u1 = static_cast<float>(rand()) / RAND_MAX;
            float u2 = static_cast<float>(rand()) / RAND_MAX;
            val = mean + stddev * std::sqrt(-2.0f * std::log(u1)) * std::cos(2.0f * M_PI * u2);
        }
    }

private:
    // Convert multi-dimensional index to flat index
    int flatten_index(const std::vector<int>& indices) const {
        if (indices.size() != shape.size()) {
            throw std::runtime_error("Index dimension mismatch");
        }
        int flat_idx = 0;
        int stride = 1;
        for (int i = shape.size() - 1; i >= 0; --i) {
            if (indices[i] >= shape[i]) {
                throw std::runtime_error("Index out of bounds");
            }
            flat_idx += indices[i] * stride;
            stride *= shape[i];
        }
        return flat_idx;
    }
};

// Tensor operations (mock implementations)
namespace ops {

// Matrix multiplication: C = A @ B
// A: [batch, m, k], B: [batch, k, n] -> C: [batch, m, n]
inline Tensor matmul(const Tensor& a, const Tensor& b) {
    if (a.shape.size() != 3 || b.shape.size() != 3) {
        throw std::runtime_error("matmul requires 3D tensors");
    }
    if (a.shape[0] != b.shape[0] || a.shape[2] != b.shape[1]) {
        throw std::runtime_error("matmul shape mismatch");
    }
    
    int batch = a.shape[0];
    int m = a.shape[1];
    int k = a.shape[2];
    int n = b.shape[2];
    
    Tensor c({batch, m, n});
    
    for (int b_idx = 0; b_idx < batch; ++b_idx) {
        for (int i = 0; i < m; ++i) {
            for (int j = 0; j < n; ++j) {
                float sum = 0.0f;
                for (int kk = 0; kk < k; ++kk) {
                    sum += a.at({b_idx, i, kk}) * b.at({b_idx, kk, j});
                }
                c.at({b_idx, i, j}) = sum;
            }
        }
    }
    
    return c;
}

// Element-wise addition
inline Tensor add(const Tensor& a, const Tensor& b) {
    if (a.shape != b.shape) {
        throw std::runtime_error("add shape mismatch");
    }
    Tensor c = a.clone();
    for (size_t i = 0; i < c.data.size(); ++i) {
        c.data[i] += b.data[i];
    }
    return c;
}

// Element-wise multiplication
inline Tensor mul(const Tensor& a, const Tensor& b) {
    if (a.shape != b.shape) {
        throw std::runtime_error("mul shape mismatch");
    }
    Tensor c = a.clone();
    for (size_t i = 0; i < c.data.size(); ++i) {
        c.data[i] *= b.data[i];
    }
    return c;
}

// SiLU activation: x * sigmoid(x)
inline Tensor silu(const Tensor& x) {
    Tensor y = x.clone();
    for (auto& val : y.data) {
        val = val / (1.0f + std::exp(-val));
    }
    return y;
}

// Softmax along last dimension
inline Tensor softmax(const Tensor& x) {
    Tensor y = x.clone();
    
    // For simplicity, assume shape is [batch, seq_len, dim]
    if (x.shape.size() != 3) {
        throw std::runtime_error("softmax requires 3D tensor");
    }
    
    int batch = x.shape[0];
    int seq_len = x.shape[1];
    int dim = x.shape[2];
    
    for (int b = 0; b < batch; ++b) {
        for (int s = 0; s < seq_len; ++s) {
            // Find max for numerical stability
            float max_val = -1e9f;
            for (int d = 0; d < dim; ++d) {
                max_val = std::max(max_val, x.at({b, s, d}));
            }
            
            // Compute exp and sum
            float sum = 0.0f;
            for (int d = 0; d < dim; ++d) {
                float exp_val = std::exp(x.at({b, s, d}) - max_val);
                y.at({b, s, d}) = exp_val;
                sum += exp_val;
            }
            
            // Normalize
            for (int d = 0; d < dim; ++d) {
                y.at({b, s, d}) /= sum;
            }
        }
    }
    
    return y;
}

} // namespace ops

} // namespace llama
} // namespace pypto
