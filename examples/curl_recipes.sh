#!/usr/bin/env bash
# ==============================================================================
# SNAP Cloud REST API cURL Recipes
# Live Base URL: https://snap-api-673324870645.asia-northeast3.run.app
# ==============================================================================

BASE_URL="https://snap-api-673324870645.asia-northeast3.run.app"

echo "=== 1. Health Check ==="
curl -s -X GET "${BASE_URL}/v1/health" | jq .

echo -e "\n=== 2. Single Sentence Normalization (Numeral + Custom Dictionary) ==="
curl -s -X POST "${BASE_URL}/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "여기서 3번 버스를 타고 3번 갈아타세요. ChatGPT와 LG CNS를 사용합니다.",
       "custom_dict": {
         "ChatGPT": "챗지피티",
         "LG CNS": "엘지씨엔에스"
       },
       "config": {
         "prosody_format": "tags",
         "return_ipa": true
       }
     }' | jq .

echo -e "\n=== 3. Batch Sentence Normalization ==="
curl -s -X POST "${BASE_URL}/v1/normalize/batch" \
     -H "Content-Type: application/json" \
     -d '{
       "texts": [
         "제1차 회의는 오전 10시 15분에 시작합니다.",
         "물가 안정을 위해 500억 원의 재정을 투입합니다.",
         "서울에서 부산까지 KTX로 2시간 15분 걸립니다."
       ],
       "custom_dict": {
         "KTX": "케이티엑스"
       }
     }' | jq .
