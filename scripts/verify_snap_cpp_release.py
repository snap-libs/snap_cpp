import os
import shutil
import subprocess
import sys

def main():
    base_dir = r'c:\work\snap'
    temp_verify_dir = os.path.join(base_dir, 'temp_cpp_verify')
    models_dir = os.path.join(base_dir, 'models', 'ko', 'v1.0.0')

    print("=== [Skill SOP] SNAP C++ Public Release Smoke Test ===")
    
    # 1. Clean previous temp verify dir
    if os.path.exists(temp_verify_dir):
        shutil.rmtree(temp_verify_dir, ignore_errors=True)

    # 2. Git Clone snap_cpp repository
    print("\n[Step 1] Cloning https://github.com/snap-libs/snap_cpp.git to temp folder...")
    subprocess.run(['git', 'clone', 'https://github.com/snap-libs/snap_cpp.git', temp_verify_dir], check=True)

    # 3. Setup models directory inside temp folder for test execution
    temp_models_ko = os.path.join(temp_verify_dir, "models", "ko")
    os.makedirs(temp_models_ko, exist_ok=True)
    
    src_ko = os.path.join(base_dir, "models", "ko")
    if not os.path.exists(src_ko):
        src_ko = os.path.join(base_dir, "models", "ko", "v1.0.0")
        
    if os.path.exists(src_ko):
        for root, dirs, files in os.walk(src_ko):
            for file in files:
                rel_p = os.path.relpath(os.path.join(root, file), src_ko)
                dest_p = os.path.join(temp_models_ko, rel_p)
                os.makedirs(os.path.dirname(dest_p), exist_ok=True)
                shutil.copy2(os.path.join(root, file), dest_p)

    # Copy global config if present
    global_cfg = os.path.join(base_dir, "models", "snap_config.json")
    if os.path.exists(global_cfg):
        os.makedirs(os.path.join(temp_verify_dir, "models"), exist_ok=True)
        shutil.copy2(global_cfg, os.path.join(temp_verify_dir, "models", "snap_config.json"))

    # 4. Copy weights onnxruntime.dll to lib/windows/x64/v1.0.0 if missing
    win_lib_dir = os.path.join(temp_verify_dir, 'lib', 'windows', 'x64', 'v1.0.0')
    weights_ort = os.path.join(base_dir, 'weights', 'onnxruntime.dll')
    if os.path.exists(weights_ort) and os.path.exists(win_lib_dir):
        shutil.copy2(weights_ort, os.path.join(win_lib_dir, 'onnxruntime.dll'))

    # 5. Compile example test_e2e.cpp using CMake
    print("\n[Step 2] Building example test_e2e against prebuilt library via CMake...")
    subprocess.run(['cmake', '-B', 'build', '-DCMAKE_BUILD_TYPE=Release'], cwd=temp_verify_dir, check=True)
    subprocess.run(['cmake', '--build', 'build', '--config', 'Release'], cwd=temp_verify_dir, check=True)
    
    out_exe = os.path.join(temp_verify_dir, 'build', 'Release', 'test_e2e.exe')
    if not os.path.exists(out_exe):
        out_exe = os.path.join(temp_verify_dir, 'build', 'test_e2e')

    # 6. Execute test sentence
    print("\n[Step 3] Running E2E test sentence on newly downloaded public library...")
    test_sentence = "고가도로를 타고 3번 버스를 타고 3번 갈아타야 갈 수 있어"
    
    cmd = [out_exe, temp_verify_dir, 'ko', test_sentence]
    res = subprocess.run(cmd, cwd=os.path.dirname(out_exe), capture_output=True, text=True)
    print(res.stdout)

    # 7. Validation assertion
    if "세번" in res.stdout and "삼번" in res.stdout:
        print("\n✅ [PASS] Public release snap_cpp verified successfully! (3번 버스->삼번, 3번 갈아타야->세번)")
    else:
        print("\n❌ [FAIL] Verification failed! Unexpected output.")

    # 8. Cleanup temp verify dir
    shutil.rmtree(temp_verify_dir, ignore_errors=True)
    print("Cleaned up temporary test folder.")

if __name__ == '__main__':
    main()
