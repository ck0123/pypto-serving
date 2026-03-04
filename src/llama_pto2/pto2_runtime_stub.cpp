// Stub implementation of PTO2 runtime for linking
// This allows the code to compile without the full PTO2 runtime library

#include <cstdlib>
#include <iostream>

extern "C" {

enum PTO2RuntimeMode {
    PTO2_MODE_EXECUTE = 0,
    PTO2_MODE_SIMULATE = 1,
    PTO2_MODE_GRAPH_ONLY = 2
};

struct PTO2Runtime {
    int mode;
    void* user_data;
};

PTO2Runtime* pto2_runtime_create(int mode) {
    std::cout << "[PTO2 Stub] Creating runtime in mode " << mode << std::endl;
    
    PTO2Runtime* rt = new PTO2Runtime();
    rt->mode = mode;
    rt->user_data = nullptr;
    
    return rt;
}

void pto2_runtime_destroy(PTO2Runtime* rt) {
    if (rt) {
        std::cout << "[PTO2 Stub] Destroying runtime" << std::endl;
        delete rt;
    }
}

} // extern "C"
