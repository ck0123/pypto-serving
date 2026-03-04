#pragma once

#include "tensor.h"
#include "layers.h"
#include "common/types.h"
#include <memory>

namespace pypto {
namespace llama {

// MLP (Feed-Forward Network) with SwiGLU activation
class MLP : public Layer {
public:
    MLP(const ModelConfig& config)
        : hidden_size_(config.hidden_size),
          intermediate_size_(config.intermediate_size) {
        
        // LLaMA uses SwiGLU: gate_proj and up_proj, then down_proj
        gate_proj_ = std::make_unique<Linear>(hidden_size_, intermediate_size_, false);
        up_proj_ = std::make_unique<Linear>(hidden_size_, intermediate_size_, false);
        down_proj_ = std::make_unique<Linear>(intermediate_size_, hidden_size_, false);
    }
    
    // Forward: input [batch, seq_len, hidden_size] -> output [batch, seq_len, hidden_size]
    Tensor forward(const Tensor& input) override {
        // SwiGLU: silu(gate_proj(x)) * up_proj(x)
        Tensor gate = gate_proj_->forward(input);
        Tensor up = up_proj_->forward(input);
        
        // Apply SiLU to gate
        gate = ops::silu(gate);
        
        // Element-wise multiplication
        Tensor hidden = ops::mul(gate, up);
        
        // Down projection
        Tensor output = down_proj_->forward(hidden);
        
        return output;
    }

private:
    int hidden_size_;
    int intermediate_size_;
    
    std::unique_ptr<Linear> gate_proj_;
    std::unique_ptr<Linear> up_proj_;
    std::unique_ptr<Linear> down_proj_;
};

} // namespace llama
} // namespace pypto
