#include "pto2_config.h"
#include "common/logger.h"
#include <cstdlib>

namespace pypto {
namespace pto2 {

PTO2Config PTO2Config::auto_detect() {
    // Check environment variable for explicit mode
    const char* mode_env = std::getenv("PTO2_MODE");
    if (mode_env) {
        std::string mode_str(mode_env);
        if (mode_str == "simulator" || mode_str == "sim") {
            LOG_INFO("PTO2_MODE=simulator, using simulator mode");
            return simulator();
        } else if (mode_str == "device") {
            LOG_INFO("PTO2_MODE=device, using device mode");
            return device();
        } else if (mode_str == "mock") {
            LOG_INFO("PTO2_MODE=mock, using mock mode");
            return mock();
        }
    }
    
    // Auto-detect based on platform
#ifdef __APPLE__
    // Mac: no device, check if simulator is available
    // For now, default to mock on Mac
    LOG_INFO("Mac detected, using mock mode (simulator not yet supported on Mac)");
    return mock();
#else
    // Linux: check for device, then simulator
    // TODO: Add actual device detection
    
    // For now, prefer simulator on Linux
    LOG_INFO("Linux detected, defaulting to simulator mode");
    return simulator();
#endif
}

} // namespace pto2
} // namespace pypto
