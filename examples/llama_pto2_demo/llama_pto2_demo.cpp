#include "engine/model_runner_llama_pto2.h"
#include "engine/sequence.h"
#include "common/types.h"
#include <iostream>
#include <memory>
#include <vector>

using namespace pypto;

void print_separator(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n\n";
}

void test_small_model() {
    print_separator("测试小模型 (PTO2)");
    
    // Create a small model config
    ModelConfig config;
    config.vocab_size = 1000;
    config.hidden_size = 256;
    config.num_layers = 2;  // Only 2 layers for quick test
    config.num_heads = 8;
    config.num_kv_heads = 4;  // GQA 2:1
    config.head_dim = 32;
    config.intermediate_size = 512;
    config.rope_theta = 10000.0f;
    config.max_position_embeddings = 512;
    config.eos_token_id = 2;
    
    std::cout << "模型配置:\n";
    std::cout << "  vocab_size: " << config.vocab_size << "\n";
    std::cout << "  hidden_size: " << config.hidden_size << "\n";
    std::cout << "  num_layers: " << config.num_layers << "\n";
    std::cout << "  num_heads: " << config.num_heads << "\n";
    std::cout << "  num_kv_heads: " << config.num_kv_heads << " (GQA)\n\n";
    
    // Create model runner
    std::cout << "创建 LLaMA PTO2 模型...\n";
    LLaMAModelRunnerPTO2 runner(config);
    
    std::cout << "执行模式: " << runner.get_execution_mode() << "\n";
    std::cout << "使用 PTO2: " << (runner.is_using_pto2() ? "是" : "否") << "\n\n";
    
    // Create a test sequence
    SamplingParams sampling_params;
    sampling_params.max_tokens = 100;
    sampling_params.temperature = 0.8f;
    sampling_params.top_p = 0.95f;
    
    auto seq = std::make_shared<Sequence>(
        "test-1",  // request_id
        std::vector<int>{1, 10, 20, 30},  // prompt tokens
        sampling_params
    );
    
    std::cout << "测试序列:\n";
    std::cout << "  Prompt tokens: [";
    for (size_t i = 0; i < seq->prompt_token_ids().size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << seq->prompt_token_ids()[i];
    }
    std::cout << "]\n";
    std::cout << "  Prompt length: " << seq->prompt_token_ids().size() << "\n\n";
    
    // Run prefill
    std::cout << "运行 Prefill...\n";
    std::vector<std::shared_ptr<Sequence>> seqs = {seq};
    auto logits = runner.run(seqs, true);
    
    std::cout << "✓ Prefill 完成\n";
    std::cout << "  输出 logits shape: [" << logits.size() << ", " << logits[0].size() << "]\n";
    std::cout << "  前5个 logits: [";
    for (int i = 0; i < 5 && i < static_cast<int>(logits[0].size()); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << logits[0][i];
    }
    std::cout << "]\n\n";
    
    // Generate a few tokens
    std::cout << "生成 tokens (Decode)...\n";
    for (int i = 0; i < 3; ++i) {
        // Add a mock generated token
        seq->append_token(100 + i);
        
        // Run decode
        logits = runner.run(seqs, false);
        
        std::cout << "  Step " << (i+1) << ": 生成 token " << (100 + i) 
                  << ", logits shape: [" << logits.size() << ", " << logits[0].size() << "]\n";
    }
    
    std::cout << "\n✓ 测试完成\n";
}

void test_tinyllama_config() {
    print_separator("测试 TinyLlama 配置 (PTO2)");
    
    // TinyLlama-1.1B config
    ModelConfig config;
    config.vocab_size = 32000;
    config.hidden_size = 2048;
    config.num_layers = 4;  // Use only 4 layers instead of 22 for testing
    config.num_heads = 32;
    config.num_kv_heads = 4;  // GQA 8:1
    config.head_dim = 64;
    config.intermediate_size = 5632;
    config.rope_theta = 10000.0f;
    config.max_position_embeddings = 2048;
    config.eos_token_id = 2;
    
    std::cout << "TinyLlama 配置 (简化版):\n";
    std::cout << "  vocab_size: " << config.vocab_size << "\n";
    std::cout << "  hidden_size: " << config.hidden_size << "\n";
    std::cout << "  num_layers: " << config.num_layers << " (原版 22 层)\n";
    std::cout << "  num_heads: " << config.num_heads << "\n";
    std::cout << "  num_kv_heads: " << config.num_kv_heads << " (GQA 8:1)\n\n";
    
    std::cout << "创建 LLaMA PTO2 模型...\n";
    LLaMAModelRunnerPTO2 runner(config);
    
    std::cout << "执行模式: " << runner.get_execution_mode() << "\n";
    std::cout << "✓ 模型创建成功\n\n";
    
    std::cout << "注意: 这是 PTO2 版本，使用任务图调度\n";
    std::cout << "      实际算子执行将在 PTO2 runtime 中进行\n";
}

int main() {
    print_separator("LLaMA PTO2 Demo");
    
    std::cout << "这个 demo 展示了 LLaMA 模型与 PTO2 runtime 的集成\n";
    std::cout << "PTO2 提供了任务图调度和硬件加速能力\n\n";
    
    try {
        // Test 1: Small model
        test_small_model();
        
        // Test 2: TinyLlama config
        test_tinyllama_config();
        
        print_separator("总结");
        std::cout << "✅ 所有测试通过\n\n";
        std::cout << "PTO2 集成状态:\n";
        std::cout << "  ✓ TensorPTO2 抽象层\n";
        std::cout << "  ✓ 所有 LLaMA 层 (Embedding, RMSNorm, RoPE, Attention, MLP)\n";
        std::cout << "  ✓ TransformerLayer 和 LLaMAModel\n";
        std::cout << "  ✓ LLaMAModelRunnerPTO2 集成到 Engine\n";
        std::cout << "  ✓ PTO2 runtime 初始化\n\n";
        
        std::cout << "下一步:\n";
        std::cout << "  1. 实现真实的 PTO2 算子调用 (matmul, softmax, etc.)\n";
        std::cout << "  2. 加载真实模型权重\n";
        std::cout << "  3. 性能优化和调试\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
