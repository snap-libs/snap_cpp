#!/usr/bin/env python3
"""
SNAP Live Cloud API Multi-Scenario Tester (Python)
Base URL: https://snap-api-673324870645.asia-northeast3.run.app
"""

import urllib.request
import json
import time

BASE_URL = "https://snap-api-673324870645.asia-northeast3.run.app"

def request_json(endpoint: str, payload: dict = None):
    url = f"{BASE_URL}{endpoint}"
    headers = {"User-Agent": "SNAP-Client-Python/1.0"}
    data = None
    if payload is not None:
        headers["Content-Type"] = "application/json"
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    
    req = urllib.request.Request(url, data=data, headers=headers)
    t0 = time.perf_counter()
    with urllib.request.urlopen(req, timeout=15) as resp:
        duration_ms = (time.perf_counter() - t0) * 1000.0
        body = json.loads(resp.read().decode("utf-8"))
        server_lat = resp.headers.get("X-Response-Time", "N/A")
        return body, duration_ms, server_lat

def main():
    print("=" * 80)
    print("🚀 SNAP Cloud REST API Live Tester (Python)")
    print(f"🔗 Target Base URL: {BASE_URL}")
    print("=" * 80)

    # 1. Health Check
    print("\n[1] Checking Service Health (`GET /v1/health`)...")
    health, rtt, _ = request_json("/v1/health")
    print(f"Status  : {health.get('status')}")
    print(f"Version : {health.get('version')}")
    print(f"Engine  : {'Loaded' if health.get('engine_loaded') else 'Offline'}")
    print(f"Active Languages: {health.get('active_languages')}")
    print(f"⚡ RTT: {rtt:.2f}ms")

    # 2. Single Sentence Normalization with Custom Dictionary & SSML
    print("\n[2] Single Sentence Normalization with Custom Dictionary & SSML (`POST /v1/normalize`)...")
    text = "2026년 8월 24일 2호선 3번 출구에서 만나 2잔의 커피를 마셨습니다. ChatGPT와 LG CNS를 사용합니다."
    custom_dict = {
        "ChatGPT": "챗지피티",
        "LG CNS": "엘지씨엔에스"
    }
    payload = {
        "text": text,
        "custom_dict": custom_dict,
        "config": {
            "prosody_format": "tags",
            "return_ipa": True
        }
    }
    res, rtt, s_lat = request_json("/v1/normalize", payload)
    data = res["data"]
    print(f"Original Text   : {data['original_text']}")
    print(f"Normalized Text : {data['normalized_text']}")
    print(f"Phonemes (G2P)  : {data['phonemes']}")
    print(f"IPA Phonetics   : {data['ipa']}")
    print(f"⚡ Engine Latency: {res['meta']['latency_ms']}ms (Server Header: {s_lat}, Client RTT: {rtt:.2f}ms)")

    # 3. Batch Sentence Processing
    print("\n[3] Batch Sentence Processing (`POST /v1/normalize/batch`)...")
    batch_payload = {
        "texts": [
            "제1차 회의는 오전 10시 15분에 시작합니다.",
            "물가 안정을 위해 500억 원의 재정을 투입합니다.",
            "서울에서 부산까지 KTX로 2시간 15분 걸립니다."
        ],
        "custom_dict": {"KTX": "케이티엑스"},
        "config": {"prosody_format": "none"}
    }
    b_res, b_rtt, b_slat = request_json("/v1/normalize/batch", batch_payload)
    print(f"Total Processed: {b_res['total_count']} items")
    for idx, item in enumerate(b_res["results"], 1):
        print(f" [{idx}] {item['original_text']} ➔ {item['normalized_text']} ({item['latency_ms']}ms)")
    print(f"⚡ Batch Client RTT: {b_rtt:.2f}ms")

    print("\n" + "=" * 80)
    print("✅ All SNAP Live API tests finished successfully!")
    print("=" * 80)

if __name__ == "__main__":
    main()
