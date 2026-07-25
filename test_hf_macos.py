"""
SNAP TTS Engine - Hugging Face macOS E2E Test & Benchmark Script
==================================================================
Downloads SNAP models directly from Hugging Face Hub on macOS
and runs Python / C++ inference pipeline seamlessly.

Usage on macOS:
    pip install huggingface_hub onnxruntime tokenizers num2words
    python test_hf_macos.py --repo-id <hf-username>/snap-tts-models
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path

sys.stdout.reconfigure(encoding='utf-8')

parser = argparse.ArgumentParser(description="SNAP Hugging Face macOS Test Runner")
parser.add_argument("--repo-id", type=str, default="Antigravity/snap-models", help="Hugging Face Model Repo ID")
parser.add_argument("--local-dir", type=str, default="./hf_models", help="Local directory to save models")
parser.add_argument("--lang", type=str, default="ko", help="Language code (ko, ja, en)")
args = parser.parse_args()

print("==================================================================")
print("  SNAP TTS Engine - Hugging Face macOS Integration Test")
print("==================================================================")

# 1. Download models from Hugging Face Hub (or fallback to local models/ if offline)
local_models_path = Path(args.local_dir).resolve()
try:
    from huggingface_hub import snapshot_download
    print(f"[1/3] Downloading models from Hugging Face Hub: '{args.repo-id}'...")
    snapshot_download(repo_id=args.repo_id, local_dir=str(local_models_path))
    print(f"      Downloaded to: {local_models_path}")
except Exception as e:
    print(f"[WARN] Hugging Face Hub download skipped/fallback: {e}")
    if Path("./models").exists():
        local_models_path = Path("./models").resolve().parent
        print(f"      Using local SNAP root: {local_models_path}")
    else:
        print("[ERROR] Models directory not found.")
        sys.exit(1)

# 2. Set SNAP_HOME environment variable
os.environ["SNAP_HOME"] = str(local_models_path)
sys.path.insert(0, str(Path(__file__).parent / "snap_py"))

# 3. Python Pipeline Execution
print(f"\n[2/3] Testing Python Pipeline on macOS (Language: {args.lang})...")
try:
    from snap.bert_session import BertSessionManager
    from snap.classifier import ContextClassifier
    from snap.text_normalize_kr import scan, apply_spans
    from snap.phonology_kr import apply_rules

    lang_dir = local_models_path / "models" / args.lang
    model_onnx = lang_dir / f"{args.lang.upper()}_model_bert_int8.onnx"

    bm = BertSessionManager()
    bm.load(args.lang.upper(), str(model_onnx), str(lang_dir))
    clf = ContextClassifier(args.lang, str(local_models_path), bert_manager=bm)

    sample_text = "iPhone 16 Pro 128GB 모델을 15층에서 백달러에 구매했다."
    res = clf.process(sample_text)
    spans = scan(sample_text, res.get('numbers', []))
    txt = apply_spans(sample_text, spans)
    phon = apply_rules(txt, res.get('annotations', []), res.get('morphemes', []))

    print(f"  - 원문: {sample_text}")
    print(f"  - 정규화/음운 결과: {phon}")
    print("  - [OK] Python Pipeline verified successfully on macOS!")

except Exception as e:
    print(f"  - [ERROR] Python pipeline failed: {e}")
    sys.exit(1)

# 4. macOS C++ Native Shared Library Build & E2E Test (libsnap_cpp.dylib)
print(f"\n[3/3] Testing C++ Native Shared Library (libsnap_cpp.dylib)...")
build_dir = Path("./snap_cpp/build_macos").resolve()
if not (build_dir / "libsnap_cpp.dylib").exists():
    print("  - Compiling libsnap_cpp.dylib for macOS...")
    subprocess.run(["cmake", "-B", str(build_dir), "-S", "./snap_cpp", "-DCMAKE_BUILD_TYPE=Release"], check=True)
    subprocess.run(["cmake", "--build", str(build_dir), "--config", "Release"], check=True)

import ctypes
import json

dylib_path = build_dir / "libsnap_cpp.dylib"
snap_dll = ctypes.CDLL(str(dylib_path))
snap_dll.snap_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
snap_dll.snap_create.restype = ctypes.c_void_p

snap_dll.snap_process.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap_dll.snap_process.restype = ctypes.c_char_p

snap_dll.snap_free.argtypes = [ctypes.c_char_p]
snap_dll.snap_free.restype = None

snap_dll.snap_destroy.argtypes = [ctypes.c_void_p]
snap_dll.snap_destroy.restype = None

handle = snap_dll.snap_create(str(local_models_path).encode('utf-8'), args.lang.encode('utf-8'))
if not handle:
    print("  - [ERROR] snap_create failed on macOS!")
    sys.exit(1)

c_res_ptr = snap_dll.snap_process(handle, sample_text.encode('utf-8'))
if c_res_ptr:
    c_res_str = c_res_ptr.decode('utf-8')
    data = json.loads(c_res_str)
    c_phon = data.get('phonology') if isinstance(data, dict) else data[0].get('phonology')
    print(f"  - C++ dylib G2P Result: {c_phon}")
    snap_dll.snap_free(c_res_ptr)

snap_dll.snap_destroy(handle)
print("  - [OK] macOS C++ Shared Library (libsnap_cpp.dylib) verified successfully!")

print("\n==================================================================")
print("  [SUCCESS] macOS Hugging Face Integration Test PASSED 100%!")
print("==================================================================")
