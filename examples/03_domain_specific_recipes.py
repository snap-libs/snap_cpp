"""
03. Real-World Domain Recipes Showcase
======================================
Demonstrates SNAP normalization across diverse real-world industry domains:
1. Business & Meeting: Sino vs Native numbers, transit line & exits, custom brand
2. Finance & Payments: Large currency amounts with commas, account transactions
3. Mobility & Navigation: Street addresses, road numbers, floor/room notations
"""

import requests

API_URL = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"

recipes = [
    {
        "domain": "1. Business & Meeting (비즈니스 미팅 / 일정 안내)",
        "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABC Technology 본사에서 만나요.",
        "custom_dict": {"ABC Technology": "에이비씨 테크놀로지"}
    },
    {
        "domain": "2. Finance & Payments (금융 결제 / 예산 공시)",
        "text": "물가 안정을 위해 정부는 1차 긴급 예산 500억 원과 150,000,000원의 지원금을 투입했습니다.",
        "custom_dict": None
    },
    {
        "domain": "3. Mobility & Navigation (내비게이션 / 도로명 주소)",
        "text": "목적지인 서울시 강남구 테헤란로 123번길 45 건물 402호로 이동합니다.",
        "custom_dict": None
    },
    {
        "domain": "4. Counter & Frequency Disambiguation (수사 변별 / 횟수 vs 호수)",
        "text": "여기서 3번 버스를 타고 3번 갈아탄 뒤 101동 302호로 가세요.",
        "custom_dict": None
    }
]

def main():
    print("=== [SNAP API] Real-World Domain Recipes ===")

    for r in recipes:
        print(f"\n[{r['domain']}]")
        print("  Input Text :", r["text"])

        payload = {
            "text": r["text"],
            "custom_dict": r["custom_dict"],
            "config": {
                "lang": "ko",
                "prosody_format": "tags",
                "return_ipa": False
            }
        }

        try:
            res = requests.post(API_URL, json=payload, timeout=10.0).json()
            if res.get("success"):
                data = res["data"]
                meta = res.get("meta", {})
                print("  Normalized :", data["normalized_text"])
                print("  Phonemes   :", data["phonemes"])
                print(f"  Latency    : {meta.get('latency_ms')} ms")
            else:
                print("  Error      :", res.get("error"))
        except Exception as e:
            print("  Request Failed:", e)

if __name__ == "__main__":
    main()
