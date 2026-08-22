#pragma once
#include <string>
#include <onnxruntime_cxx_api.h>

namespace snap {

/// Information about resolved hardware device
struct DeviceInfo {
    std::string requested = "auto";  // e.g. "auto", "cuda", "directml", "coreml", "cpu"
    std::string resolved = "cpu";    // e.g. "cuda", "directml", "coreml", "cpu"
    int device_id = 0;
};

/// Resolve device using resolution priority:
/// 1. explicit_device (if not empty and != "")
/// 2. SNAP_DEVICE environment variable
/// 3. snap_config.json "device"
/// 4. "auto" (default)
DeviceInfo resolve_device(const std::string& explicit_device = "", const std::string& config_input = "");

/// Configure Ort::SessionOptions with the resolved Execution Provider
/// Sets IntraOpNumThreads, graph optimization, and attaches EP (CUDA/DirectML/CoreML/OpenVINO)
/// Gracefully falls back to CPU if requested EP fails or is unavailable.
bool configure_session_options(Ort::SessionOptions& opts, DeviceInfo& dev_info, int num_threads = 0);

} // namespace snap
