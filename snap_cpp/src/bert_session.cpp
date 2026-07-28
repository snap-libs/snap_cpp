/**
 * SNAP BertSession — ONNX Runtime BERT inference
 * ================================================
 * Manages BERT ONNX session and produces hidden states.
 */

#include "snap/bert_session.h"
#include <onnxruntime_cxx_api.h>
#include <stdexcept>
#include <array>
#include <thread>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
static std::wstring to_wstring(const std::string& utf8) {
    if (utf8.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                  static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring ws(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                        static_cast<int>(utf8.size()), ws.data(), len);
    return ws;
}
#endif

namespace snap {

// ─── PIMPL ───

struct BertSession::Impl {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "SNAP_BERT"};
    std::unique_ptr<Ort::Session> session;
    std::vector<std::string> in_names;  // cached at load(); not re-queried per inference
    std::string out_name;               // cached at load()
};

BertSession::BertSession() : impl_(std::make_unique<Impl>()) {}
BertSession::~BertSession() = default;
BertSession::BertSession(BertSession&&) noexcept = default;
BertSession& BertSession::operator=(BertSession&&) noexcept = default;

// ─── Load ───

bool BertSession::load(const std::string& model_path,
                       const std::string& tokenizer_path,
                       int num_threads,
                       const std::string& device) {
    if (!tokenizer_.load(tokenizer_path)) return false;

    try {
        int target_threads = num_threads;
        if (target_threads <= 0) {
            unsigned int cores = std::thread::hardware_concurrency();
            target_threads = (cores > 1) ? (cores / 2) : 1;
            target_threads = (target_threads < 8) ? target_threads : 8;
        }

        Ort::SessionOptions opts;
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        opts.SetIntraOpNumThreads(target_threads);
        opts.SetInterOpNumThreads(1);

        // Try enabling CUDA dynamically to prevent compilation linkage issues
        if (device == "cuda" || device == "gpu") {
#ifdef _WIN32
            HMODULE hMod = GetModuleHandleA("onnxruntime.dll");
            if (hMod) {
                // Signature: OrtStatus* ORT_API_CALL fn(OrtSessionOptions*, int device_id)
                typedef OrtStatus* (ORT_API_CALL* PFN_AppendCUDA)(OrtSessionOptions*, int);
                PFN_AppendCUDA pfnAppend = reinterpret_cast<PFN_AppendCUDA>(
                    GetProcAddress(hMod, "OrtSessionOptionsAppendExecutionProvider_CUDA"));
                if (pfnAppend) {
                    // Ort::SessionOptions has operator OrtSessionOptions*() via Ort::Base<T>
                    OrtSessionOptions* raw_opts = opts;
                    pfnAppend(raw_opts, 0);
                }
            }
#endif
        }

#ifdef _WIN32
        auto wpath = to_wstring(model_path);
        impl_->session = std::make_unique<Ort::Session>(
            impl_->env, wpath.c_str(), opts);
#else
        impl_->session = std::make_unique<Ort::Session>(
            impl_->env, model_path.c_str(), opts);
#endif
        // Cache I/O names once — avoids per-inference AllocatorWithDefaultOptions overhead
        {
            Ort::AllocatorWithDefaultOptions alloc;
            size_t num_inputs = impl_->session->GetInputCount();
            impl_->in_names.clear();
            for (size_t i = 0; i < num_inputs; i++) {
                auto ptr = impl_->session->GetInputNameAllocated(i, alloc);
                impl_->in_names.emplace_back(ptr.get());
            }
            auto out_ptr = impl_->session->GetOutputNameAllocated(0, alloc);
            impl_->out_name = out_ptr.get();
        }
    } catch (const Ort::Exception& e) {
        return false;
    }
    return true;
}

// ─── Inference ───

BertSession::Output BertSession::get_hidden_states(const std::string& text) {
    auto enc = tokenizer_.encode(text);
    int64_t seq_len = static_cast<int64_t>(enc.input_ids.size());

    auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::array<int64_t, 2> shape = {1, seq_len};

    auto ids_tensor = Ort::Value::CreateTensor<int64_t>(
        mem, enc.input_ids.data(), enc.input_ids.size(),
        shape.data(), shape.size());

    auto mask_tensor = Ort::Value::CreateTensor<int64_t>(
        mem, enc.attention_mask.data(), enc.attention_mask.size(),
        shape.data(), shape.size());

    auto type_tensor = Ort::Value::CreateTensor<int64_t>(
        mem, enc.token_type_ids.data(), enc.token_type_ids.size(),
        shape.data(), shape.size());

    // Use pre-cached I/O names (set once in load(); no per-call allocation)
    std::vector<const char*> in_names;
    in_names.reserve(impl_->in_names.size());
    for (const auto& n : impl_->in_names) in_names.push_back(n.c_str());
    const char* out_name = impl_->out_name.c_str();

    std::vector<Ort::Value> inputs;
    inputs.push_back(std::move(ids_tensor));
    inputs.push_back(std::move(mask_tensor));
    if (impl_->in_names.size() >= 3) {
        inputs.push_back(std::move(type_tensor));
    }

    auto outputs = impl_->session->Run(
        Ort::RunOptions{nullptr},
        in_names.data(), inputs.data(), inputs.size(),
        &out_name, 1);

    // Parse output: [1, seq_len, hidden_dim]
    auto& tensor = outputs[0];
    auto tinfo = tensor.GetTensorTypeAndShapeInfo();
    auto tshape = tinfo.GetShape();

    Output result;
    result.seq_len = static_cast<int>(tshape[1]);
    result.hidden_dim = static_cast<int>(tshape[2]);
    result.offsets = enc.offsets;

    size_t total = static_cast<size_t>(result.seq_len) * result.hidden_dim;
    const float* data = tensor.GetTensorData<float>();
    result.hidden_states.assign(data, data + total);

    return result;
}

}  // namespace snap
