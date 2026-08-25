# SNAP (Semantic Normalization via Attached Probes)

SNAP은 한국어·일본어·영어를 지원하는 실시간 음성 전처리(TTS Frontend & ITN) 엔진입니다. 문장의 문맥과 의미를 분석하여 발음 정규화와 운율 생성을 실시간으로 수행합니다.

[English](README.md) | [한국어](README_KO.md)  
[공식 웹사이트](https://snap-libs.github.io/snap/) | [텍스트 전처리 데모](https://huggingface.co/spaces/softguy777/snap-demo) | [MeloTTS 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo) | [Piper 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo2) | [F5-TTS 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

* [SNAP 기술 백서 (Technical White Paper)](docs/SNAP_White_Paper.md)
* [SNAP 다국어 텍스트 전처리 데모 (TN / G2P / Prosody)](https://huggingface.co/spaces/softguy777/snap-demo) — 문맥 기반 정규화(TN), 음운 변환(G2P), 운율 쉼표 예측 실시간 테스트

## SNAP + TTS 연동 Live Demo

많은 글로벌 오픈소스 및 상용 TTS 엔진들은 다국어 전처리기로 `espeak-ng`나 단순 규칙 기반 변환기를 사용합니다. 그러나 한국어와 일본어 같은 언어는 복잡한 음운 변동(비음화·유음화·경음화), 문맥에 따른 수사 및 한자 읽기 변별, 동철이음이의어 등으로 인해 기존 전처리기로는 발음 왜곡과 부자연스러운 끊어 읽기가 빈번하게 발생했습니다.

실제로 상용 서비스 현업에서도 이러한 발음 오류를 줄이기 위해 LLM을 거쳐 발음 기호를 생성하고 사람이 이를 재검수하는 과정을 거치기도 합니다.

아래 3가지 오픈소스 TTS(MeloTTS, Piper, F5-TTS)는 글로벌 환경에서 널리 사용되지만 전처리기 문제로 한국어 지원이 미흡했던 대표적인 모델들입니다. SNAP 엔진과의 결합을 통해 각 모델의 아키텍처 특성에 맞추어 올바른 발음과 운율을 생성하도록 구성한 데모입니다.

### MeloTTS 음성 합성 (BERT 공유형)
MeloTTS가 사용하는 BERT 모델을 전처리 단계에서 공유하여 결과를 음향 모델에 전달하므로 BERT를 중복 계산하지 않습니다. 기존에 사용하던 g2pk를 SNAP으로 대체합니다.
* [SNAP + MeloTTS 음성 합성 데모 바로가기](https://huggingface.co/spaces/softguy777/snap_voice_demo)

### Piper VITS 음성 합성 (경량 온디바이스형)
BERT를 내장하지 않은 경량 TTS와의 연동 예제입니다. 기존에 사용하던 espeak-ng를 SNAP으로 대체하여 22개 화자 스타일의 한국어 음성을 지원합니다.
* [SNAP + Piper VITS 22개 화자 데모 바로가기](https://huggingface.co/spaces/softguy777/snap_voice_demo2)

### F5-TTS 음성 합성 및 보이스 클로닝 (확산 모델 연동형)
Flow Matching 확산(Diffusion) 기반 최신 모델인 F5-TTS와의 연동 데모입니다. NFD 자모 분해 방식을 사용하는 F5-TTS에 맞추어 SNAP G2P 결과를 자모 분해하여 전달함으로써, 16개 프리셋 화자뿐만 아니라 사용자 참조 오디오를 활용한 한국어 Zero-Shot 보이스 클로닝(Voice Cloning)을 지원합니다.
* [SNAP + F5-TTS 보이스 클로닝 데모 바로가기](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

> **데모 실행 환경 안내**:
> * **MeloTTS / Piper**: 경량 모델로 저사양 2 vCPU 환경에서도 지연 없이 실시간 음성 생성이 가능합니다.
> * **F5-TTS**: 확산(Diffusion) 모델 특성상 Hugging Face GPU 동적 할당을 사용하므로, 실행 시마다 GPU 할당 대기 시간이 발생할 수 있으며 허깅페이스 월간 GPU 쿼터 초과 시 동작이 제한될 수 있습니다.

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
    "custom_dict": {"ABC Technology": "에이비씨 테크놀로지"},
    "config": {"lang": "ko", "prosody_format": "tags"}
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

### 상세 연동 문서 및 실전 예제
* [REST API 연동 상세 매뉴얼](SNAP_REST_API_MANUAL.md) — 파라미터 규격, 응답 스키마 및 상세 레시피
* [실전 예제 모음 (examples/)](examples/) — 단일 문장/사전 치환(`01`), 대용량 배치(`02`), 산업 도메인별 레시피(`03`), 다국어(`04`), cURL 모음

## Enterprise & On-Premise SDK (Docker)

보안, 망분리 환경 또는 대규모 트래픽 처리를 위해 로컬 배포가 필요한 환경을 대상으로 **독립 실행형 SDK Docker 컨테이너** 및 Native C++ SDK를 제공합니다.

* **네트워크 독립성**: 외부 통신 없는 오프라인/사내 폐쇄망 환경 지원
* **초저지연 온프레미스 연동**: REST API 및 C++ Direct Linkage 지원
* **제공 방식**: 사전 협약(Agreement) 기반 배포
* **Contact**: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)
