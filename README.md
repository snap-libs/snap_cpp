# SNAP (Semantic Normalization via Attached Probes)

SNAP은 한국어·일본어·영어를 지원하는 실시간 음성 전처리(TTS Frontend & ITN) 엔진입니다. 문장의 문맥과 의미를 분석하여 발음 정규화와 운율 생성을 실시간으로 수행합니다.

[공식 웹사이트](https://snap-libs.github.io/snap/) | [프로젝트 소개서](docs/Snap%20Project%20Introduction.md) | [TN 데모](https://huggingface.co/spaces/softguy777/snap-demo) | [MeloTTS 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo) | [Piper 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo2) | [F5-TTS 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

* [SNAP 프로젝트 상세 소개서](docs/Snap%20Project%20Introduction.md)

## Snap Live Demo

### 다국어 텍스트 전처리 (TN / G2P / Prosody)
한국어, 일본어, 영어 3개 국어의 문맥 기반 텍스트 정규화(TN), 음운 변환(G2P), 운율 쉼표(Prosodic Pause) 예측을 테스트할 수 있는 실시간 데모입니다.
* [SNAP Frontend 다국어(한·일·영) 데모 바로가기](https://huggingface.co/spaces/softguy777/snap-demo)

### MeloTTS 음성 합성 (BERT 공유형)
MeloTTS가 사용하는 BERT 모델을 전처리 단계에서 공유하여 결과를 음향 모델에 전달하므로 BERT를 중복 계산하지 않습니다. 기존에 사용하던 g2pk를 SNAP으로 대체합니다.
* [SNAP + MeloTTS 음성 합성 데모 바로가기](https://huggingface.co/spaces/softguy777/snap_voice_demo)

### Piper VITS 음성 합성 (경량 온디바이스형)
BERT를 내장하지 않은 TTS와의 연동 예제입니다. 기존에 사용하던 espeak-ng를 SNAP으로 대체하여 22개 화자 스타일의 한국어 음성을 지원합니다.
* [SNAP + Piper VITS 22개 화자 데모 바로가기](https://huggingface.co/spaces/softguy777/snap_voice_demo2)

### F5-TTS 음성 합성 (확산 모델 연동형)
확산(Diffusion) 기반 최신 모델인 F5-TTS와의 연동 데모입니다. NFD 자모 분해 방식을 사용하는 F5-TTS에 맞추어 SNAP G2P 결과를 자모 분해하여 전달하며, 16개 프리셋 화자 음성을 지원합니다.
* [SNAP + F5-TTS 16개 화자 데모 바로가기](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

## SNAP Cloud API (Free)

현재 SNAP의 다국어(한국어·일본어·영어) 전처리 기능 테스트를 위해 별도의 가입이나 조건 없이 자유롭게 호출할 수 있도록 오픈되어 있습니다.

### 주요 제공 기능
* **문맥 기반 텍스트 정규화**: 수사, 날짜, 시간, 단위, 기호 등의 문맥별 발음 정규화
* **G2P 및 운율 태깅**: 표준 발음 변환 및 3단계 운율 쉼표(`[P1]~[P3]`, SSML) 태그 출력
* **동적 커스텀 사전 (`custom_dict`)**: 브랜드명, 신조어, 고유명사를 요청 단위로 즉시 등록 및 발음 치환

### 빠른 시작 (Quick Start)

**Python**
```python
import requests

url = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"
payload = {
    "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABC Technology 본사에서 만나요.",
    "custom_dict": {"ABC Technology": "에이비씨 테크놀로지"}
}
res = requests.post(url, json=payload).json()
print("정규화:", res["data"]["normalized_text"])
print("발음  :", res["data"]["phonemes"])
# 출력: 이처니심늉년 파뤌 이시보일 오후 세시 삼십뿌네 이호선 삼번 출구 압 에이비씨 테크놀로지 본사에서 만나요.
```

**cURL (Terminal)**
```bash
curl -X POST "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABC Technology 본사에서 만나요.",
       "custom_dict": {"ABC Technology": "에이비씨 테크놀로지"}
     }'
```

### 상세 연동 문서
* [REST API 연동 상세 매뉴얼](SNAP_REST_API_MANUAL.md) — 파라미터 규격, 응답 스키마, Python/JavaScript 호출 예제

## Enterprise & On-Premise SDK (Docker)

보안, 망분리 환경 또는 대규모 트래픽 처리를 위해 로컬 배포가 필요한 환경을 대상으로 **독립 실행형 SDK Docker 컨테이너** 및 Native C++ SDK를 제공합니다.

* **네트워크 독립성**: 외부 통신 없는 오프라인/사내 폐쇄망 환경 지원
* **초저지연 온프레미스 연동**: REST API 및 C++ Direct Linkage 지원
* **제공 방식**: 사전 협약(Agreement) 기반 배포
* **Contact**: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)
