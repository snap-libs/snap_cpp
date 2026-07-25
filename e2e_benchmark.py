import os
import time
import ctypes
import json
import sys
import argparse
import pathlib

# Ensure UTF-8 output on Windows
sys.stdout.reconfigure(encoding='utf-8')

# ── Path configuration ─────────────────────────────────────────────────
# Default paths are relative to this script's location.
# Override with --dll-dir / --weights-dir if running from a different layout.
_HERE = pathlib.Path(__file__).resolve().parent

parser = argparse.ArgumentParser(description="SNAP TTS C++ Engine Benchmark")
parser.add_argument("--dll-dir",
                    default=str(_HERE / "snap_cpp" / "build" / "Release"),
                    help="Directory containing snap_cpp.dll (default: <repo>/snap_cpp/build/Release)")
parser.add_argument("--weights-dir",
                    default=str(_HERE / "models"),
                    help="SNAP models directory (default: <repo>/models)")
parser.add_argument("--corpus-dir",
                    default=None,
                    help="Path to NIKL corpus directory containing JSON files for stress testing (Korean only).")
args = parser.parse_args()

dll_dir     = args.dll_dir
weights_dir = args.weights_dir

# Register DLL search directory
if hasattr(os, "add_dll_directory"):
    os.add_dll_directory(dll_dir)
else:
    os.environ["PATH"] = dll_dir + os.pathsep + os.environ["PATH"]

# Load snap_cpp shared library (.dll / .so)
lib_candidates = [
    os.path.join(dll_dir, "libsnap_cpp.so"),
    os.path.join(dll_dir, "snap_cpp.dll"),
    os.path.join(dll_dir, "snap_cpp.so")
]
dll_path = None
for cand in lib_candidates:
    if os.path.exists(cand):
        dll_path = cand
        break
if not dll_path:
    dll_path = os.path.join(dll_dir, "snap_cpp.dll" if sys.platform == "win32" else "libsnap_cpp.so")

try:
    snap = ctypes.CDLL(dll_path)
    print(f"[SUCCESS] Loaded snap_cpp library from {dll_path}\n")
except Exception as e:
    print(f"[ERROR] Failed to load snap_cpp library: {e}")
    sys.exit(1)

# C API function signatures
snap.snap_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
snap.snap_create.restype = ctypes.c_void_p  # engine handle

# snap_process returns a heap-allocated C string pointer.
# Using c_char_p would cause ctypes to auto-convert it to bytes, losing the
# raw pointer needed for snap_free. Use c_void_p and copy via ctypes.string_at.
snap.snap_process.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_process.restype = ctypes.c_void_p

snap.snap_free.argtypes = [ctypes.c_void_p]  # void* matches C API signature
snap.snap_free.restype = None
snap.snap_destroy.argtypes = [ctypes.c_void_p]
snap.snap_destroy.restype = None

# Test sentences (Korean/Japanese text is intentional input data)
test_cases = {
    "ko": [
        "안과에 갔다.",
        "커피 3잔을 마셨다.",
        "축구 경기에서 3:0으로 이겼다.",
        "독감과 감기 몸살로 오늘 약을 먹었다.",
        "15층에 살고 있으며 24년 1월 1일이다.",
        "BaaS를 $100에 구독하여 사용 중이다.",
        "서버 192.168.1.1에 접속을 시도한다.",
        "연락처 010-1234-5678 혹은 대표번호 1588-1234로 연락 바랍니다."
    ],
    "ja": [
        "今日は3:00に市場へ行きます。",
        "8センチの長さに切ります。",
        "昨日、日本へ行きました。",
        "私は1年生の時に、2人の友達と1つずつミカンを食べました。"
    ]
}

if args.corpus_dir:
    corpus_path = pathlib.Path(args.corpus_dir)
    print(f"Loading corpus from {corpus_path}...")
    corpus_sentences = []
    for json_file in corpus_path.rglob("*.json"):
        try:
            with open(json_file, "r", encoding="utf-8") as f:
                data = json.load(f)
            for doc in data.get("document", []):
                items = doc.get("utterance", doc.get("sentence", []))
                for item in items:
                    form = item.get("form", "").strip()
                    if form:
                        corpus_sentences.append(form)
        except Exception as e:
            pass
    if corpus_sentences:
        test_cases["ko"] = corpus_sentences
        print(f"Loaded {len(corpus_sentences)} sentences from corpus for 'ko'.")

def run_benchmark(lang):
    print(f"=== Benchmarking Language: {lang.upper()} ===")

    # Initialize engine
    init_start = time.perf_counter()
    handle = snap.snap_create(weights_dir.encode('utf-8'), lang.encode('utf-8'))
    init_time = (time.perf_counter() - init_start) * 1000

    if not handle:
        print(f"[ERROR] Failed to initialize engine for '{lang}'")
        return

    print(f"Engine initialized in {init_time:.2f} ms.\n")

    # Print table header if small dataset
    hide_details = len(test_cases[lang]) > 100
    if not hide_details:
        print(f"{'No.':<3} | {'Input Text':<45} | {'Char Count':<10} | {'Latency (ms)':<12} | {'Speed (chars/sec)':<18}")
        print("-" * 100)
    else:
        print(f"Running {len(test_cases[lang])} sentences... (details hidden due to large size)")

    total_chars = 0
    total_time = 0.0

    for idx, text in enumerate(test_cases[lang], 1):
        char_count = len(text)

        # Run inference and measure latency
        t0 = time.perf_counter()
        result_ptr = snap.snap_process(handle, text.encode('utf-8'))
        t1 = time.perf_counter()

        latency = (t1 - t0) * 1000

        if result_ptr:
            # Copy bytes from C++ heap pointer, then free it
            result_str = ctypes.string_at(result_ptr).decode('utf-8')
            snap.snap_free(result_ptr)
        else:
            result_str = "FAILED"
            latency = 0.0

        speed = (char_count / (latency / 1000.0)) if latency > 0 else 0

        if not hide_details:
            # Truncate long strings for display alignment
            short_text = text if len(text) <= 42 else text[:40] + "..."
            print(f"{idx:<3} | {short_text:<45} | {char_count:<10} | {latency:<12.2f} | {speed:<18.1f}")
        elif idx % 10000 == 0:
            print(f"Processed {idx}/{len(test_cases[lang])} sentences...")

        if latency > 0:
            total_chars += char_count
            total_time += (latency / 1000.0)

    print("-" * 100)
    avg_speed = total_chars / total_time if total_time > 0 else 0
    print(f"Total processed: {total_chars} chars in {total_time * 1000:.2f} ms")
    print(f"Average processing speed: {avg_speed:.2f} chars/sec\n")

    # Destroy engine instance
    snap.snap_destroy(handle)

if __name__ == "__main__":
    run_benchmark("ko")
    run_benchmark("ja")
