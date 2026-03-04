#include "mlp_pto2.h"

// Forward declare PTO2 types
struct PTO2Runtime;

namespace pypto {
namespace llama_pto2 {

TensorPTO2 MLPPTO2::forward(PTO2Runtime* runtime, const TensorPTO2& x) {
    // x: [batch, seq_len, hidden_size]
    
    // SwiGLU: gate_proj(x) * silu(up_proj(x))
    TensorPTO2 gate = gate_proj_.forward(runtime, x);  // [batch, seq_len, intermediate_size]
    TensorPTO2 up = up_proj_.forward(runtime, x);      // [batch, seq_len, intermediate_size]
    
    // Apply SiLU to up
    TensorPTO2 up_silu = ops::silu(runtime, up);
    
    // Element-wise multiply
    TensorPTO2 hidden = ops::mul(runtime, gate, up_silu);  // [batch, seq_len, intermediate_size]
    
    // Down projection
    TensorPTO2 output = down_proj_.forward(runtime, hidden);  // [batch, seq_len, hidden_size]
    
    return output;
}

} // namespace llama_pto2
} // namespace pypto
