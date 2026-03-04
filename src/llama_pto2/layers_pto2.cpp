#include "layers_pto2.h"
#include <stdexcept>
#include <cstring>

// Forward declare PTO2 types
struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

// =============================================================================
// Embedding Layer
// =============================================================================

TensorPTO2 EmbeddingPTO2::forward(PTO2Runtime* runtime, const TensorPTO2& token_ids) {
    // token_ids: [batch_size]
    // output: [batch_size, embedding_dim]
    
    int batch_size = token_ids.shape()[0];
    TensorPTO2 output({batch_size, embedding_dim_});
    output.allocate_device_memory(runtime);
    
    // TODO: Submit PTO2 embedding lookup task
    // For now, mock implementation
    output.fill(0.0f);
    
    return output;
}

TensorPTO2 EmbeddingPTO2::forward(PTO2Runtime* runtime, const std::vector<int>& token_ids) {
    // Create tensor from token_ids
    TensorPTO2 ids({static_cast<int>(token_ids.size())}, DataType::INT32);
    ids.allocate_host_memory();
    memcpy(ids.host_ptr(), token_ids.data(), token_ids.size() * sizeof(int));
    
    return forward(runtime, ids);
}

// =============================================================================
// RMSNorm Layer
// =============================================================================

TensorPTO2 RMSNormPTO2::forward(PTO2Runtime* runtime, const TensorPTO2& x) {
    // x: [*, hidden_size]
    // output: [*, hidden_size]
    
    TensorPTO2 output = ops::rmsnorm(runtime, x, weight_, eps_);
    return output;
}

// =============================================================================
// Linear Layer
// =============================================================================

TensorPTO2 LinearPTO2::forward(PTO2Runtime* runtime, const TensorPTO2& x) {
    // x: [batch, seq_len, in_features] or [batch, in_features]
    // weight: [out_features, in_features]
    // output: [batch, seq_len, out_features] or [batch, out_features]
    
    // Reshape x to 2D for matmul
    std::vector<int> orig_shape = x.shape();
    int last_dim = orig_shape.back();
    
    if (last_dim != in_features_) {
        throw std::runtime_error("Linear: input feature dimension mismatch");
    }
    
    // Flatten all but last dimension
    int batch_size = 1;
    for (size_t i = 0; i < orig_shape.size() - 1; ++i) {
        batch_size *= orig_shape[i];
    }
    
    TensorPTO2 x_2d = x.reshape({batch_size, in_features_});
    
    // y = x @ weight^T
    // x_2d: [batch, in_features], weight: [out_features, in_features]
    // We need: [batch, in_features] @ [in_features, out_features] = [batch, out_features]
    // So we need to transpose weight or use different matmul convention
    
    TensorPTO2 y_2d = ops::matmul(runtime, x_2d, weight_);  // Mock for now
    
    if (has_bias_) {
        y_2d = ops::add(runtime, y_2d, bias_);
    }
    
    // Reshape back to original shape
    std::vector<int> output_shape = orig_shape;
    output_shape.back() = out_features_;
    
    return y_2d.reshape(output_shape);
}

// =============================================================================
// RoPE
// =============================================================================

TensorPTO2 RoPEPTO2::forward(PTO2Runtime* runtime, const TensorPTO2& x, const std::vector<int>& positions) {
    // x: [batch, num_heads, seq_len, head_dim]
    // Apply rotary position embedding
    
    return ops::rope(runtime, x, positions, theta_);
}

} // namespace llama_pto2
} // namespace pypto
