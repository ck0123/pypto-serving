#pragma once

#include "tensor_pto2.h"
#include "layers_pto2.h"
#include "common/types.h"
#include <memory>

struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

// MLP (Feed-Forward Network) with SwiGLU activation
class MLPPTO2 {
public:
    MLPPTO2(const ModelConfig& config)
        : hidden_size_(config.hidden_size),
          intermediate_size_(config.intermediate_size),
          gate_proj_(config.hidden_size, config.intermediate_size, false),
          up_proj_(config.hidden_size, config.intermediate_size, false),
          down_proj_(config.intermediate_size, config.hidden_size, false) {
    }
    
    // Forward pass
    // x: [batch, seq_len, hidden_size]
    // output: [batch, seq_len, hidden_size]
    TensorPTO2 forward(PTO2Runtime* runtime, const TensorPTO2& x);
    
    // Access projection layers (for weight loading)
    LinearPTO2& gate_proj() { return gate_proj_; }
    LinearPTO2& up_proj() { return up_proj_; }
    LinearPTO2& down_proj() { return down_proj_; }

private:
    int hidden_size_;
    int intermediate_size_;
    
    LinearPTO2 gate_proj_;  // Gate projection
    LinearPTO2 up_proj_;    // Up projection
    LinearPTO2 down_proj_;  // Down projection
};

} // namespace llama_pto2
} // namespace pypto
