"""
01. Single Sentence Normalization & Custom Dictionary Example
===========================================================
Demonstrates single-sentence text normalization, G2P phoneme conversion,
and dynamic user-defined custom dictionary (custom_dict) substitution.
"""

import requests

API_URL = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"

def main():
    # Input text with numbers, dates, times, transit lines, and English brand term
    payload = {
        "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABC Technology 본사에서 만나요.",
        "custom_dict": {
            "ABC Technology": "에이비씨 테크놀로지"
        },
        "config": {
            "lang": "ko",
            "prosody_format": "tags",
            "return_ipa": True
        }
    }

    print("=== [SNAP API] Single Sentence Normalization ===")
    print("Input Text:", payload["text"])
    print("Custom Dict:", payload["custom_dict"])
    print("\nSending request to SNAP Cloud API...")

    response = requests.post(API_URL, json=payload, timeout=10.0)
    res = response.json()

    if res.get("success"):
        data = res["data"]
        meta = res.get("meta", {})
        print("\n--- Results ---")
        print("Normalized Text :", data["normalized_text"])
        print("Phonemes (G2P)  :", data["phonemes"])
        print("IPA Phonetics   :", data.get("ipa"))
        print(f"Engine Latency  : {meta.get('latency_ms')} ms")
    else:
        print("Error:", res.get("error"))

if __name__ == "__main__":
    main()
