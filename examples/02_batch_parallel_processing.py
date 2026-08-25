"""
02. High-Throughput Batch Parallel Processing Example
====================================================
Demonstrates batch sentence normalization for large text documents,
ebooks, news articles, or multi-turn conversational dialogs in a single request.
"""

import requests

BATCH_API_URL = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize/batch"

def main():
    # Multi-sentence batch payload covering diverse domains
    payload = {
        "texts": [
            "2026년 8월 25일 오전 10시 30분에 3호선 신사역 1번 출구에서 1차 회의가 있습니다.",
            "물가 안정을 위해 정부는 500억 원의 긴급 예산을 편성했습니다.",
            "서울시 강남구 테헤란로 123번길 45 건물 402호로 배송 부탁드립니다.",
            "ABC Technology 3호관에서 미팅을 3번 진행하고 35,000원을 결제했습니다."
        ],
        "custom_dict": {
            "ABC Technology": "에이비씨 테크놀로지"
        },
        "config": {
            "lang": "ko",
            "prosody_format": "tags",
            "return_ipa": False
        }
    }

    print("=== [SNAP API] High-Throughput Batch Processing ===")
    print(f"Batch Size: {len(payload['texts'])} sentences")
    print("\nSending batch request to SNAP Cloud API...")

    response = requests.post(BATCH_API_URL, json=payload, timeout=15.0)
    res = response.json()

    if res.get("success"):
        results = res.get("results", [])
        meta = res.get("meta", {})
        print(f"\n--- Batch Results (Total Sentences: {res.get('total_count')}, Latency: {meta.get('latency_ms')} ms) ---")
        for i, item in enumerate(results, 1):
            print(f"\n[{i}] Original  : {item['original_text']}")
            print(f"    Normalized: {item['normalized_text']}")
            print(f"    Phonemes  : {item['phonemes']}")
    else:
        print("Error:", res.get("error"))

if __name__ == "__main__":
    main()
