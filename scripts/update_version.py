#!/usr/bin/env python3
"""
SNAP Model & Dictionary Automatic Versioning Tool
Usage:
    python scripts/update_version.py --lang ko --type dict --version v1.2.0
"""

import os
import sys
import json
import argparse
import hashlib
import shutil
from datetime import datetime

def calculate_sha256(filepath):
    hasher = hashlib.sha256()
    with open(filepath, 'rb') as f:
        while chunk := f.read(65536):
            hasher.update(chunk)
    return hasher.hexdigest()

def update_version(base_dir, lang, target_type, new_version):
    manifest_path = os.path.join(base_dir, "manifest.json")
    if not os.path.exists(manifest_path):
        print(f"[ERROR] manifest.json not found at {manifest_path}")
        sys.exit(1)

    with open(manifest_path, 'r', encoding='utf-8') as f:
        manifest = json.load(f)

    lang_dict = manifest.setdefault("languages", {}).setdefault(lang, {})

    if target_type == "dict":
        lang_dict["active_dict_version"] = new_version
        print(f"[SUCCESS] Updated {lang} active_dict_version -> {new_version}")
    elif target_type == "model":
        lang_dict["active_model_version"] = new_version
        print(f"[SUCCESS] Updated {lang} active_model_version -> {new_version}")
    else:
        print(f"[ERROR] Unknown type: {target_type}")
        sys.exit(1)

    manifest["updated_at"] = datetime.now().strftime("%Y-%m-%d")

    with open(manifest_path, 'w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"[SUCCESS] Saved updated manifest to {manifest_path}")

def main():
    parser = argparse.ArgumentParser(description="SNAP Version Update Utility")
    parser.add_argument("--base-dir", default="models", help="Path to models directory")
    parser.add_argument("--lang", required=True, choices=["ko", "ja", "en"], help="Target language")
    parser.add_argument("--type", required=True, choices=["dict", "model"], help="Update target type")
    parser.add_argument("--version", required=True, help="New version tag (e.g. v1.2.0)")

    args = parser.parse_args()
    update_version(args.base_dir, args.lang, args.type, args.version)

if __name__ == "__main__":
    main()
