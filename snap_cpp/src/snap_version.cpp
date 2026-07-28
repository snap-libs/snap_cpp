/**
 * @file snap_version.cpp
 * @brief Implementation of SNAP C++ SDK versioning functions
 */

#include "snap/snap_api.h"
#include "snap/snap_version.h"
#include <stdio.h>
#include <string.h>

#ifndef SNAP_GIT_HASH
#define SNAP_GIT_HASH "main"
#endif

#ifndef SNAP_BUILD_DATE
#define SNAP_BUILD_DATE __DATE__
#endif

static const char g_verbose_version[] =
    SNAP_VERSION_STRING
    "-" SNAP_GIT_HASH
    " (" SNAP_BUILD_DATE ")";

static const char g_version[] = SNAP_VERSION_STRING;

extern "C" {

SNAP_API const char* snap_version(void) {
    return g_version;
}

SNAP_API int snap_version_number(void) {
    return SNAP_VERSION_NUMBER;
}

SNAP_API void snap_version_info(int* major, int* minor, int* patch) {
    if (major) *major = SNAP_VERSION_MAJOR;
    if (minor) *minor = SNAP_VERSION_MINOR;
    if (patch) *patch = SNAP_VERSION_PATCH;
}

SNAP_API int snap_version_check(int major, int minor) {
    if (SNAP_VERSION_MAJOR != major) {
        return 0;
    }
    if (SNAP_VERSION_MINOR < minor) {
        return 0;
    }
    return 1;
}

SNAP_API const char* snap_version_verbose(void) {
    return g_verbose_version;
}

}  // extern "C"
