/**
 * SNAP C API — DLL entry points
 * ===============================
 */

#include "snap/snap_api.h"
#include "snap/classifier.h"
#include <string>
#include <mutex>
#include <iostream>
#include <stdexcept>
#include <locale>

extern "C" {

SNAP_API void* snap_create(const char* weights_dir, const char* lang) {
    try {
        std::locale::global(std::locale("C"));
        snap::ContextClassifier* classifier = new snap::ContextClassifier();
        if (!classifier->init(weights_dir, lang)) {
            delete classifier;
            return nullptr;
        }
        return static_cast<void*>(classifier);
    } catch (...) {
        return nullptr;
    }
}

SNAP_API const char* snap_process(void* handle, const char* text_utf8) {
    if (!handle || !text_utf8) return nullptr;
    try {
        snap::ContextClassifier* classifier = static_cast<snap::ContextClassifier*>(handle);
        auto result = classifier->process(text_utf8);
        std::string json_str = result.to_json();
        // Return a heap-allocated copy so caller can snap_free it
        char* buf = new char[json_str.size() + 1];
        std::copy(json_str.begin(), json_str.end(), buf);
        buf[json_str.size()] = '\0';
        return buf;
    } catch (const std::exception& e) {
        std::cerr << "[snap_process] Exception: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cerr << "[snap_process] Unknown exception." << std::endl;
        return nullptr;
    }
}

SNAP_API const char* snap_normalize(void* handle, const char* text_utf8) {
    if (!handle || !text_utf8) return nullptr;
    try {
        snap::ContextClassifier* classifier = static_cast<snap::ContextClassifier*>(handle);
        std::string normalized = classifier->normalize_text(text_utf8);
        char* buf = new char[normalized.size() + 1];
        std::copy(normalized.begin(), normalized.end(), buf);
        buf[normalized.size()] = '\0';
        return buf;
    } catch (const std::exception& e) {
        std::cerr << "[snap_normalize] Exception: " << e.what() << std::endl;
        return nullptr;
    } catch (...) {
        std::cerr << "[snap_normalize] Unknown exception." << std::endl;
        return nullptr;
    }
}

SNAP_API void snap_free(const void* result) {
    if (result) {
        delete[] static_cast<const char*>(result);
    }
}

SNAP_API void snap_destroy(void* handle) {
    if (handle) {
        snap::ContextClassifier* classifier = static_cast<snap::ContextClassifier*>(handle);
        delete classifier;
    }
}

}  // extern "C"
