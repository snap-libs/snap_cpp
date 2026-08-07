#pragma once

/**
 * @file snap_version.h
 * @brief SNAP C++ SDK Versioning & ABI Compatibility API
 */

#include "snap/snap_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Compile-time version constants */
#define SNAP_VERSION_MAJOR    1
#define SNAP_VERSION_MINOR    0
#define SNAP_VERSION_PATCH    0

#define SNAP_VERSION_STRING "1.0.0"

#define SNAP_VERSION_NUMBER \
    (SNAP_VERSION_MAJOR * 10000 + SNAP_VERSION_MINOR * 100 + SNAP_VERSION_PATCH)

/**
 * @brief Get runtime library version string
 * @return const char* Version string (e.g. "1.0.0")
 */
SNAP_API const char* snap_version(void);

/**
 * @brief Get runtime library version number
 * @return int Numeric version (e.g. 10000)
 */
SNAP_API int snap_version_number(void);

/**
 * @brief Get detailed version info
 */
SNAP_API void snap_version_info(int* major, int* minor, int* patch);

/**
 * @brief Check ABI compatibility against major.minor
 * @return int 1 if compatible, 0 otherwise
 */
SNAP_API int snap_version_check(int major, int minor);

/**
 * @brief Get verbose version information (compiler, build date, etc.)
 */
SNAP_API const char* snap_version_verbose(void);

#ifdef __cplusplus
}
#endif
