#include <gtest/gtest.h>
#include "llama/tensor.h"
#include "llama/layers.h"
#include "llama/attention.h"
#include "llama/mlp.h"
#include "llama/transformer.h"

using namespace pypto;
using namespace pypto::llama;

// Test Tensor operations
TEST(LLaMATest, TensorCreation) {
    Tensor t({2, 3, 4});
    EXPECT_EQ(t.shape.size(), 3);
    EXPECT_EQ(t.shape[0], 2);
    EXPECT_EQ(t.shape[1], 3);
    EXPECT_EQ(t.shape[2], 4);
    EXPECT_EQ(t.numel(), 24);
}

TEST(LLaMATest, TensorOperations) {
    Tensor a({2, 3});
    Tensor b({2, 3});
    
    a.fill(1.0f);
    b.fill(2.0f);
    
    Tensor c = ops::add(a, b);
    EXPECT_FLOAT_EQ(c.data[0], 3.0f);
    
    Tensor d = ops::mul(a, b);
    EXPECT_FLOAT_EQ(d.data[0], 2.0f);
}

// Test Embedding layer
TEST(LLaMATest, Embedding) {
    Embedding emb(100, 64);
    
    std::vector<int> token_ids = {1, 2, 3};
    Tensor output = emb.forward(token_ids, 1, 3);
    
    EXPECT_EQ(output.shape.size(), 3);
    EXPECT_EQ(output.shape[0], 1);   // batch
    EXPECT_EQ(output.shape[1], 3);   // seq_len
    EXPECT_EQ(output.shape[2], 64);  // hidden_size
}

// Test RMSNorm
TEST(LLaMATest, RMSNorm) {
    RMSNorm norm(64);
    
    Tensor input({1, 4, 64});
    input.randn(0.0f, 1.0f);
    
    Tensor output = norm.forward(input);
    
    EXPECT_EQ(output.shape, input.shape);
    
    // Check that RMS is approximately 1.0
    float sum_sq = 0.0f;
    for (int i = 0; i < 64; ++i) {
        float val = output.at({0, 0, i});
        sum_sq += val * val;
    }
    float rms = std::sqrt(sum_sq / 64);
    EXPECT_NEAR(rms, 1.0f, 0.1f);
}

// Test RoPE
TEST(LLaMATest, RoPE) {
    RoPE rope(16, 64);
    
    Tensor input({1, 4, 4, 16});  // [batch, num_heads, seq_len, head_dim]
    input.randn(0.0f, 1.0f);
    
    std::vector<int> positions = {0, 1, 2, 3};
    Tensor output = rope.apply(input, positions);
    
    EXPECT_EQ(output.shape, input.shape);
}

// Test Linear layer
TEST(LLaMATest, Linear) {
    Linear linear(64, 128);
    
    Tensor input({1, 4, 64});
    input.randn(0.0f, 1.0f);
    
    Tensor output = linear.forward(input);
    
    EXPECT_EQ(output.shape.size(), 3);
    EXPECT_EQ(output.shape[0], 1);
    EXPECT_EQ(output.shape[1], 4);
    EXPECT_EQ(output.shape[2], 128);
}

// Test MLP
TEST(LLaMATest, MLP) {
    ModelConfig config;
    config.hidden_size = 64;
    config.intermediate_size = 128;
    
    MLP mlp(config);
    
    Tensor input({1, 4, 64});
    input.randn(0.0f, 1.0f);
    
    Tensor output = mlp.forward(input);
    
    EXPECT_EQ(output.shape, input.shape);
}

// Test KV Cache
TEST(LLaMATest, KVCache) {
    KVCache cache(1, 4, 64, 16);  // batch=1, num_heads=4, max_seq=64, head_dim=16
    
    EXPECT_EQ(cache.seq_lens[0], 0);
    
    // Append some keys and values
    Tensor key({1, 4, 3, 16});
    Tensor value({1, 4, 3, 16});
    key.fill(1.0f);
    value.fill(2.0f);
    
    cache.append(key, value, 0);
    EXPECT_EQ(cache.seq_lens[0], 3);
    
    // Get cached values
    auto [cached_k, cached_v] = cache.get(0);
    EXPECT_EQ(cached_k.shape[2], 3);
    EXPECT_FLOAT_EQ(cached_k.data[0], 1.0f);
    EXPECT_FLOAT_EQ(cached_v.data[0], 2.0f);
}

// Test Attention
TEST(LLaMATest, Attention) {
    ModelConfig config;
    config.hidden_size = 64;
    config.num_heads = 4;
    config.num_kv_heads = 4;
    config.head_dim = 16;
    config.max_position_embeddings = 64;
    
    Attention attn(config);
    
    Tensor input({1, 4, 64});
    input.randn(0.0f, 1.0f);
    
    KVCache cache(1, 4, 64, 16);
    std::vector<int> positions = {0, 1, 2, 3};
    
    Tensor output = attn.forward_with_cache(input, cache, positions, true);
    
    EXPECT_EQ(output.shape, input.shape);
    EXPECT_EQ(cache.seq_lens[0], 4);
}

// Test TransformerLayer
TEST(LLaMATest, TransformerLayer) {
    ModelConfig config;
    config.hidden_size = 64;
    config.num_heads = 4;
    config.num_kv_heads = 4;
    config.head_dim = 16;
    config.intermediate_size = 128;
    config.max_position_embeddings = 64;
    
    TransformerLayer layer(config);
    
    Tensor input({1, 4, 64});
    input.randn(0.0f, 1.0f);
    
    KVCache cache(1, 4, 64, 16);
    std::vector<int> positions = {0, 1, 2, 3};
    
    Tensor output = layer.forward(input, cache, positions, true);
    
    EXPECT_EQ(output.shape, input.shape);
}

// Test full LLaMA model
TEST(LLaMATest, LLaMAModel) {
    ModelConfig config;
    config.vocab_size = 100;
    config.hidden_size = 64;
    config.num_layers = 2;
    config.num_heads = 4;
    config.num_kv_heads = 4;
    config.head_dim = 16;
    config.intermediate_size = 128;
    config.max_position_embeddings = 64;
    
    LLaMAModel model(config);
    
    std::vector<int> token_ids = {1, 10, 20, 30};
    std::vector<int> positions = {0, 1, 2, 3};
    
    Tensor logits = model.forward(token_ids, 1, 4, positions, true);
    
    EXPECT_EQ(logits.shape.size(), 3);
    EXPECT_EQ(logits.shape[0], 1);
    EXPECT_EQ(logits.shape[1], 4);
    EXPECT_EQ(logits.shape[2], 100);
}

// Test decode with KV cache
TEST(LLaMATest, DecodeWithCache) {
    ModelConfig config;
    config.vocab_size = 100;
    config.hidden_size = 64;
    config.num_layers = 2;
    config.num_heads = 4;
    config.num_kv_heads = 4;
    config.head_dim = 16;
    config.intermediate_size = 128;
    config.max_position_embeddings = 64;
    
    LLaMAModel model(config);
    model.reset_kv_caches();
    
    // Prefill
    std::vector<int> prompt = {1, 5, 10};
    std::vector<int> positions1 = {0, 1, 2};
    Tensor logits1 = model.forward(prompt, 1, 3, positions1, true);
    EXPECT_EQ(logits1.shape[1], 3);
    
    // Decode
    std::vector<int> next_token = {15};
    std::vector<int> positions2 = {3};
    Tensor logits2 = model.forward(next_token, 1, 1, positions2, false);
    EXPECT_EQ(logits2.shape[1], 1);
}
