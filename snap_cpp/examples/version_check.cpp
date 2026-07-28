#include "snap/snap_version.h"
#include "snap/snap_api.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("=== SNAP C++ SDK Version Check ===\n\n");
    
    printf("📦 Compile-time Version: %s (Number: %d)\n", SNAP_VERSION_STRING, SNAP_VERSION_NUMBER);
    printf("🚀 Runtime DLL Version:  %s (Number: %d)\n", snap_version(), snap_version_number());
    printf("💡 Verbose Info:        %s\n\n", snap_version_verbose());
    
    if (strcmp(SNAP_VERSION_STRING, snap_version()) != 0) {
        printf("⚠️ Version Mismatch Detected!\n");
        return 1;
    }
    
    if (snap_version_check(1, 0)) {
        printf("✅ ABI Compatibility Check: Compatible with v1.0\n");
    } else {
        printf("❌ ABI Compatibility Check: Incompatible\n");
        return 1;
    }
    
    printf("✅ All Version Checks Passed!\n");
    return 0;
}
