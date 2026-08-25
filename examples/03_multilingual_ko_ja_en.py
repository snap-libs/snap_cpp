"""
03. Multilingual (Korean, Japanese, English) Showcase
=====================================================
Demonstrates context-aware phonetic normalization across 3 languages:
- Korean: Numeral disambiguation & context-dependent tensification
- Japanese: Contextual Kanji reading & counter euphonic changes
- English: Part-of-speech heteronyms & date/currency formatting
"""

import requests

API_URL = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"

cases = [
    {
        "lang": "ko",
        "title": "Korean: Numeral Disambiguation (Route #3 vs 3 Times)",
        "text": "여기서 3번 버스를 타고 3번 갈아타세요.",
        "config": {"lang": "ko", "prosody_format": "tags"}
    },
    {
        "lang": "ja",
        "title": "Japanese: Date vs Duration Kanji Reading",
        "text": "1日は休みで、1日中雨が降りました。",
        "config": {"lang": "ja", "prosody_format": "tags"}
    },
    {
        "lang": "en",
        "title": "English: Part-of-Speech Heteronym (Verb vs Adjective)",
        "text": "I live near a live concert on March 15th.",
        "config": {"lang": "en", "prosody_format": "tags", "return_ipa": True}
    }
]

def main():
    print("=== [SNAP API] Multilingual Context Disambiguation ===")

    for case in cases:
        print(f"\n--- {case['title']} ---")
        print("Input Text:", case["text"])

        payload = {
            "text": case["text"],
            "config": case["config"]
        }

        try:
            res = requests.post(API_URL, json=payload, timeout=10.0).json()
            if res.get("success"):
                data = res["data"]
                print("Normalized Text :", data["normalized_text"])
                print("Phonemes (G2P)  :", data["phonemes"])
                if data.get("ipa"):
                    print("IPA Phonetics   :", data["ipa"])
            else:
                print("Error:", res.get("error"))
        except Exception as e:
            print("Request Failed:", e)

if __name__ == "__main__":
    main()
