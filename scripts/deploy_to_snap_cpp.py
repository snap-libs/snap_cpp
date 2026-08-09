import os
import shutil
import tarfile
import zipfile
import subprocess
import sys

def main():
    base_dir = r'c:\work\snap'
    temp_dir = os.path.join(base_dir, 'temp_release_v1038')
    clean_repo_dir = os.path.join(base_dir, 'clean_snap_cpp')

    print("=== [Skill SOP] SNAP C++ Public Library Deployer ===")
    
    # Clean previous temp directories
    for d in [temp_dir, clean_repo_dir]:
        if os.path.exists(d):
            shutil.rmtree(d, ignore_errors=True)
            
    os.makedirs(temp_dir, exist_ok=True)

    # 1. Fetch latest release tag dynamically via gh CLI
    latest_tag = "v1.0.46"
    try:
        res = subprocess.run(['gh', 'release', 'list', '--repo', 'snap-libs/snap_core', '--limit', '1'], capture_output=True, text=True, check=True)
        if res.stdout.strip():
            latest_tag = res.stdout.strip().split('\t')[2]
    except Exception as e:
        print(f"Warning: Could not fetch latest release tag, defaulting to {latest_tag}. Error: {e}")

    print(f"\n[Step 1] Downloading Release {latest_tag} assets from snap_core...")
    subprocess.run(['gh', 'release', 'download', latest_tag, '--repo', 'snap-libs/snap_core', '-D', temp_dir], check=True)

    # 2. Extract assets
    print("\n[Step 2] Extracting platform binaries...")
    win_zip = os.path.join(temp_dir, 'snap_cpp-windows-x64.zip')
    linux_tar = os.path.join(temp_dir, 'snap_cpp-linux-x64.tar.gz')
    mac_tar = os.path.join(temp_dir, 'snap_cpp-macos.tar.gz')

    win_out = os.path.join(temp_dir, 'win_out')
    linux_out = os.path.join(temp_dir, 'linux_out')
    mac_out = os.path.join(temp_dir, 'mac_out')

    if os.path.exists(win_zip):
        with zipfile.ZipFile(win_zip, 'r') as z:
            z.extractall(win_out)

    if os.path.exists(linux_tar):
        with tarfile.open(linux_tar, 'r:gz') as t:
            t.extractall(linux_out, filter='data')

    if os.path.exists(mac_tar):
        with tarfile.open(mac_tar, 'r:gz') as t:
            t.extractall(mac_out, filter='data')

    # 3. Update snap_cpp/lib/
    print("\n[Step 3] Updating local snap_cpp/lib catalog...")
    win_target = os.path.join(base_dir, 'snap_cpp', 'lib', 'windows', 'x64', 'v1.0.0')
    linux_target = os.path.join(base_dir, 'snap_cpp', 'lib', 'linux', 'x64', 'v1.0.0')
    mac_target = os.path.join(base_dir, 'snap_cpp', 'lib', 'macos', 'v1.0.0')

    os.makedirs(win_target, exist_ok=True)
    os.makedirs(linux_target, exist_ok=True)
    os.makedirs(mac_target, exist_ok=True)

    # Copy Windows
    win_src = os.path.join(win_out, 'windows-x64')
    if os.path.exists(win_src):
        for f in os.listdir(win_src):
            if f.endswith('.dll') or f.endswith('.lib') or f.endswith('.json'):
                shutil.copy2(os.path.join(win_src, f), os.path.join(win_target, f))

    # Copy Linux
    linux_src = os.path.join(linux_out, 'linux-x64')
    if os.path.exists(linux_src):
        for f in os.listdir(linux_src):
            shutil.copy2(os.path.join(linux_src, f), os.path.join(linux_target, f))

    # Copy macOS
    mac_src = os.path.join(mac_out, 'macos')
    if os.path.exists(mac_src):
        for f in os.listdir(mac_src):
            shutil.copy2(os.path.join(mac_src, f), os.path.join(mac_target, f))

    # 4. Prepare public repository (Header, Lib, Examples ONLY — NO src/)
    print("\n[Step 4] Preparing public distribution structure (NO src/)...")
    os.makedirs(clean_repo_dir, exist_ok=True)
    src_dir = os.path.join(base_dir, 'snap_cpp')

    public_items = [
        'CMakeLists.txt',
        'LICENSE',
        'README.md',
        'README_kr.md',
        'test_e2e.cpp',
        '.gitignore',
        'docs',
        'examples',
        'include',
        'lib',
        'bin',
        'scripts',
        'setup',
        'run_quick_test_en.bat',
        'run_quick_test_ja.bat',
        'run_quick_test_ko.bat',
    ]

    for item in public_items:
        s = os.path.join(src_dir, item)
        d = os.path.join(clean_repo_dir, item)
        if os.path.exists(s):
            if os.path.isdir(s):
                shutil.copytree(s, d)
            else:
                shutil.copy2(s, d)

    # Write Public SDK-specific CMakeLists.txt (linking prebuilt lib)
    public_cmakelists = """cmake_minimum_required(VERSION 3.16)
project(snap_cpp_sdk CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(WIN32)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x64/v1.0.0)
elseif(APPLE)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/macos/v1.0.0)
else()
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/linux/x64/v1.0.0)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_e2e.cpp")
    add_executable(test_e2e test_e2e.cpp)
    target_link_libraries(test_e2e PRIVATE snap_cpp)
    
    if(WIN32)
        add_custom_command(TARGET test_e2e POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x64/v1.0.0/snap_cpp.dll"
            "$<TARGET_FILE_DIR:test_e2e>"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/windows/x64/v1.0.0/onnxruntime.dll"
            "$<TARGET_FILE_DIR:test_e2e>"
        )
    endif()
endif()
"""
    with open(os.path.join(clean_repo_dir, 'CMakeLists.txt'), 'w', encoding='utf-8') as f:
        f.write(public_cmakelists)

    # Clean unneeded obj/exe/go source files from clean_repo_dir
    setup_in_clean = os.path.join(clean_repo_dir, 'setup')
    if os.path.exists(setup_in_clean):
        for f in os.listdir(setup_in_clean):
            if f != 'SNAP_SETUP_MANUAL.md' and f != 'assets':
                p = os.path.join(setup_in_clean, f)
                if os.path.isdir(p):
                    shutil.rmtree(p, ignore_errors=True)
                else:
                    try:
                        os.remove(p)
                    except Exception:
                        pass

    for root, dirs, files in os.walk(clean_repo_dir):
        for f in files:
            if f.endswith('.obj') or f.endswith('.exe') or f.endswith('.exp'):
                try:
                    os.remove(os.path.join(root, f))
                except Exception:
                    pass

    # 5. Push to snap_cpp.git
    print("\n[Step 5] Pushing to https://github.com/snap-libs/snap_cpp.git...")
    subprocess.run(['git', 'init'], cwd=clean_repo_dir, check=True)
    subprocess.run(['git', 'checkout', '-b', 'main'], cwd=clean_repo_dir, check=True)
    subprocess.run(['git', 'add', '.'], cwd=clean_repo_dir, check=True)
    subprocess.run(['git', 'commit', '-m', f'build(lib): update prebuilt multi-platform binaries from release {latest_tag}'], cwd=clean_repo_dir, check=True)
    subprocess.run(['git', 'push', '--force', 'https://github.com/snap-libs/snap_cpp.git', 'main'], cwd=clean_repo_dir, check=True)

    # Cleanup temp
    shutil.rmtree(temp_dir, ignore_errors=True)
    shutil.rmtree(clean_repo_dir, ignore_errors=True)
    print(f"\n✅ Successfully deployed {latest_tag} prebuilt binaries and public SDK to https://github.com/snap-libs/snap_cpp.git!")

if __name__ == '__main__':
    main()
