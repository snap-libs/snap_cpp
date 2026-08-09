"""
SNAP Python SDK Quickstart Demo (Environment-Variable-Free)
============================================================
Usage:
    python demo.py [weights_dir] [lang] [text]

Example:
    python demo.py . ko "2024년 5월 28일 오후 3시에 만납시다."
"""

import sys
import os

try:
    from snap.classifier import ContextClassifier
except ImportError:
    # If snap package is in current or parent directory
    sys.path.insert(0, os.path.abspath("."))
    sys.path.insert(0, os.path.abspath(".."))
    from snap.classifier import ContextClassifier

def main():
    weights_dir = sys.argv[1] if len(sys.argv) > 1 else "."
    lang = sys.argv[2] if len(sys.argv) > 2 else "ko"
    text = sys.argv[3] if len(sys.argv) > 3 else "2024년 5월 28일 오후 3시에 만납시다."

    print(f"[SNAP Python] Initializing engine for language '{lang}'...")
    print(f"[SNAP Python] Target folder path: {weights_dir}")

    # Explicit folder path initialization (Zero Environment Variable Dependency)
    try:
        classifier = ContextClassifier(weights_dir=weights_dir, lang=lang)
        print("[SNAP Python] Engine initialized successfully.\n")
    except Exception as e:
        print(f"[SNAP Python] ERROR initializing engine: {e}", file=sys.stderr)
        sys.exit(1)

    print("--- Input ---")
    print(text)
    print("\n--- G2P Result ---")
    
    result = classifier.process(text)
    print(result)

if __name__ == "__main__":
    main()
