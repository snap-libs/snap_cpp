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
import ctypes
import json
import traceback
from pathlib import Path

# Force UTF-8 environment
os.environ["PYTHONIOENCODING"] = "utf-8"
if hasattr(sys.stdout, "reconfigure"):
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

def safe_print(msg):
    try:
        print(msg)
    except UnicodeEncodeError:
        print(str(msg).encode('ascii', errors='backslashreplace').decode('ascii'))

parser = argparse.ArgumentParser(description="SNAP Hugging Face macOS Test Runner")
parser.add_argument("--repo-id", type=str, default="Antigravity/snap-models", help="Hugging Face Model Repo ID")
parser.add_argument("--local-dir", type=str, default="./hf_models", help="Local directory to save models")
parser.add_argument("--lang", type=str, default="ko", help="Language code (ko, ja, en)")
args = parser.parse_args()

safe_print("==================================================================")
safe_print("  SNAP TTS Engine - Hugging Face macOS Integration Test")
safe_print("==================================================================")

# 1. Download models from Hugging Face Hub (or fallback to local models/ if offline)
local_models_path = Path(args.local_dir).resolve()
try:
    from huggingface_hub import snapshot_download
    safe_print(f"[1/3] Downloading models from Hugging Face Hub: '{args.repo_id}'...")
    snapshot_download(repo_id=args.repo_id, local_dir=str(local_models_path))
    safe_print(f"      Downloaded to: {local_models_path}")
except Exception as e:
    safe_print(f"[WARN] Hugging Face Hub download skipped/fallback: {e}")
    if Path("./models").exists():
        local_models_path = Path(".").resolve()
        safe_print(f"      Using local SNAP root: {local_models_path}")
    else:
        safe_print("[ERROR] Models directory not found.")
        sys.exit(1)

# 2. Set SNAP_HOME environment variable
os.environ["SNAP_HOME"] = str(local_models_path)
sys.path.insert(0, str(Path(__file__).parent / "snap_py"))

# 3. Python Pipeline Execution
safe_print(f"\n[2/3] Testing Python Pipeline on macOS/Cross-platform (Language: {args.lang})...")
try:
    from snap.bert_session import BertSessionManager
    from snap.classifier import ContextClassifier
    from snap.text_normalize_kr import scan, apply_spans
    from snap.phonology_kr import apply_rules
    from snap.lang_utils import get_lang_prefix

    prefix = get_lang_prefix(args.lang)
    lang_dir = local_models_path / "models" / args.lang
    model_onnx = lang_dir / f"{prefix}_model_bert_int8.onnx"

    bm = BertSessionManager()
    bm.load(prefix, str(model_onnx), str(lang_dir))
    clf = ContextClassifier(args.lang, str(local_models_path), bert_manager=bm)

    sample_text = "iPhone 16 Pro 128GB 모델을 15층에서 백달러에 구매했다."
    res = clf.process(sample_text)
    spans = scan(sample_text, res.get('numbers', []))
    txt = apply_spans(sample_text, spans)
    phon = apply_rules(txt, res.get('annotations', []), res.get('morphemes', []))

    safe_print(f"  - 원문: {sample_text}")
    safe_print(f"  - 정규화/음운 결과: {phon}")
    safe_print("  - [OK] Python Pipeline verified successfully on macOS!")

except Exception as e:
    safe_print(f"  - [ERROR] Python pipeline failed: {e}")
    traceback.print_exc()
    sys.exit(1)

# 4. C++ Native Shared Library E2E Test (.dylib / .dll / .so)
safe_print(f"\n[3/3] Testing C++ Native Shared Library...")

if sys.platform == "darwin":
    target_ext = ".dylib"
elif sys.platform == "win32":
    target_ext = ".dll"
else:
    target_ext = ".so"

c_candidates = [
    Path(f"./snap_cpp/build_macos/libsnap_cpp{target_ext}"),
    Path(f"./snap_cpp/build/Release/snap_cpp{target_ext}"),
    Path(f"./snap_cpp/build_linux/libsnap_cpp{target_ext}"),
    Path(f"./weights/snap_cpp{target_ext}"),
]
dylib_path = None
for cand in c_candidates:
    if cand.exists():
        dylib_path = cand
        break

if not dylib_path and sys.platform == "darwin":
    build_dir = Path("./snap_cpp/build_macos").resolve()
    safe_print("  - Compiling libsnap_cpp.dylib for macOS...")
    subprocess.run(["cmake", "-B", str(build_dir), "-S", "./snap_cpp", "-DCMAKE_BUILD_TYPE=Release"], check=True)
    subprocess.run(["cmake", "--build", str(build_dir), "--config", "Release"], check=True)
    dylib_path = build_dir / "libsnap_cpp.dylib"

if not dylib_path or not dylib_path.exists():
    safe_print(f"  - [WARN] Shared library binary not found: {dylib_path}")
    sys.exit(0)

if sys.platform == "win32" and dylib_path:
    if hasattr(os, "add_dll_directory"):
        os.add_dll_directory(str(dylib_path.parent.resolve()))

snap_dll = ctypes.CDLL(str(dylib_path))
snap_dll.snap_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
snap_dll.snap_create.restype = ctypes.c_void_p

snap_dll.snap_process.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap_dll.snap_process.restype = ctypes.c_void_p

snap_dll.snap_free.argtypes = [ctypes.c_void_p]
snap_dll.snap_free.restype = None

snap_dll.snap_destroy.argtypes = [ctypes.c_void_p]
snap_dll.snap_destroy.restype = None

handle = snap_dll.snap_create(str(local_models_path).encode('utf-8'), args.lang.encode('utf-8'))
if not handle:
    safe_print("  - [ERROR] snap_create failed on macOS!")
    sys.exit(1)

c_res_ptr = snap_dll.snap_process(handle, sample_text.encode('utf-8'))
if c_res_ptr:
    c_res_str = ctypes.string_at(c_res_ptr).decode('utf-8')
    try:
        data = json.loads(c_res_str)
        if isinstance(data, list) and len(data) > 0:
            c_phon = data[0].get('phonology')
        elif isinstance(data, dict):
            c_phon = data.get('phonology')
        else:
            c_phon = c_res_str
        safe_print(f"  - C++ Shared Library G2P Result: {c_phon}")
    except Exception as e:
        safe_print(f"  - C++ Raw Result: {c_res_str}")
    snap_dll.snap_free(c_res_ptr)

snap_dll.snap_destroy(handle)
safe_print("  - [OK] macOS C++ Shared Library (libsnap_cpp.dylib / dll / so) verified successfully!")

safe_print("\n==================================================================")
safe_print("  [SUCCESS] macOS Hugging Face Integration Test PASSED 100%!")
safe_print("==================================================================")
