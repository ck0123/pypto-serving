#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pypto {

// Basic types
using TokenId = int32_t;
using TokenIds = std::vector<TokenId>;
using BlockId = int32_t;
using RequestId = std::string;

// Sampling parameters
struct SamplingParams {
    float temperature = 1.0f;
    int top_k = -1;  // -1 means disabled
    float top_p = 1.0f;
    int max_tokens = 16;
    bool ignore_eos = false;

    SamplingParams() = default;
    
    SamplingParams(float temp, int k = -1, float p = 1.0f, int max_tok = 16, bool ignore = false)
        : temperature(temp), top_k(k), top_p(p), max_tokens(max_tok), ignore_eos(ignore) {}
};

// Sequence status
enum class SequenceStatus {
    WAITING,   // In waiting queue
    RUNNING,   // Being processed
    FINISHED,  // Generation complete
    PREEMPTED  // Temporarily paused
};

// Finish reason
enum class FinishReason {
    NONE,
    LENGTH,  // Reached max_tokens
    EOS,     // Generated EOS token
    STOP     // Hit stop sequence
};

// Model configuration
struct ModelConfig {
    int vocab_size = 32000;
    int hidden_size = 4096;
    int num_layers = 32;
    int num_heads = 32;
    int num_kv_heads = 32;  // For GQA
    int head_dim = 128;
    int intermediate_size = 11008;
    float rope_theta = 10000.0f;
    int max_position_embeddings = 2048;
    TokenId eos_token_id = 2;
    TokenId pad_token_id = 0;

    ModelConfig() = default;
};

// Cache configuration
struct CacheConfig {
    int block_size = 16;      // Tokens per block
    int num_gpu_blocks = 1024; // Total blocks available
    int num_cpu_blocks = 0;    // CPU offload (not implemented yet)
    bool enable_prefix_caching = true;

    CacheConfig() = default;
    
    CacheConfig(int bs, int gpu_blocks)
        : block_size(bs), num_gpu_blocks(gpu_blocks) {}
};

// Scheduler configuration
struct SchedulerConfig {
    int max_num_seqs = 256;           // Max concurrent sequences
    int max_num_batched_tokens = 2048; // Max tokens per batch
    int max_model_len = 2048;          // Max sequence length

    SchedulerConfig() = default;
    
    SchedulerConfig(int max_seqs, int max_tokens, int max_len)
        : max_num_seqs(max_seqs), max_num_batched_tokens(max_tokens), max_model_len(max_len) {}
};

} // namespace pypto
