#!/usr/bin/env bash
# ==============================================================================
# SNAP Cloud REST API - cURL Quick Reference Recipes
# ==============================================================================

API_BASE="https://snap-api-673324870645.asia-northeast3.run.app"

echo "=== 1. Health Check ==="
curl -s -X GET "${API_BASE}/v1/health" | jq .
echo -e "\n"

echo "=== 2. Single Normalization with Custom Dictionary ==="
curl -s -X POST "${API_BASE}/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABC Technology 본사에서 만나요.",
       "custom_dict": {
         "ABC Technology": "에이비씨 테크놀로지"
       },
       "config": {
         "lang": "ko",
         "prosody_format": "tags",
         "return_ipa": true
       }
     }' | jq .
echo -e "\n"

echo "=== 3. High-Throughput Batch Processing ==="
curl -s -X POST "${API_BASE}/v1/normalize/batch" \
     -H "Content-Type: application/json" \
     -d '{
       "texts": [
         "1차 회의는 3호선 신사역에서 진행합니다.",
         "물가 안정을 위해 500억 원을 투입합니다."
       ],
       "config": {
         "lang": "ko",
         "prosody_format": "tags"
       }
     }' | jq .
echo -e "\n"

echo "=== 4. Multilingual: Japanese (Kanji Yomi) ==="
curl -s -X POST "${API_BASE}/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "1日は休みで、3本のペンを買いました。",
       "config": {
         "lang": "ja",
         "prosody_format": "tags"
       }
     }' | jq .
echo -e "\n"
