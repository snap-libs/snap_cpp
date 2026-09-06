# SNAP (Semantic Normalization via Attached Probes)

SNAP은 문맥 인지형 실시간 음성 전처리(TTS Frontend & ITN) 엔진입니다.  
다국어(한국어·일본어·영어) v1.0 연구 기반에 이어, 성능 최적화와 대화체 어미 변환 옵션을 적용한 **한국어 전용 TTS 전처리 엔진 v2.0**을 제공합니다. 일본어/영어 v2.0은 추후 공개할 예정입니다.

[English](README.md) | [한국어](README_KO.md) | [공식 웹사이트](https://snap-libs.github.io/snap/)

---

## 🔗 주요 문서 및 데모 링크

### 🚀 SNAP v2.0 (한국어)
* 🎮 **[SNAP v2.0 실시간 TTS 연동 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo4)**: 한국어 v2.0 전처리 및 음성 합성 라이브 테스트
* 📑 **[SNAP v2.0 한국어 기술문서](docs/SNAP_White_Paper_v2.0_KO.md)**: Distilled Mini BERT 백본, 고성능 C++ 네이티브 최적화 및 벤치마크
* 📖 **[SNAP v2.0 한국어 기능 명세서](docs/SNAP_KO_v2.0_FUNCTIONAL_SPEC.md)**: 표준 발음법 30개 조항 전수 지원, 문맥 변별 및 단위 정규화 세부 기능 명세
* 📋 **[SNAP v2.0 API 문서](docs/SNAP_REST_API_MANUAL_KO.md)**: 공개 REST API 파라미터 및 연동 규격

### 🏛️ SNAP v1.0 (다국어 연구 기반)
* 📑 **[SNAP v1.0 기술문서](docs/SNAP_White_Paper_v1.0_KO.md)**: 한·일·영 하이브리드 Probing Head 원천 연구 백서
* 🎮 **[SNAP v1.0 다국어 프론트엔드 데모](https://huggingface.co/spaces/softguy777/snap-demo)**: 한·일·영 텍스트 정규화 및 G2P
* 🎙️ **오픈소스 TTS 연동 데모**: [MeloTTS](https://huggingface.co/spaces/softguy777/snap_voice_demo) | [Piper](https://huggingface.co/spaces/softguy777/snap_voice_demo2) | [F5-TTS](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

---

## 📌 한국어 v2.0 주요 특징

* **실시간 처리 성능**: Distilled Mini 4-Layer BERT 백본과 C++ 네이티브 최적화를 적용하여 문장당 평균 1.86ms (평균 32자 기준, CPU 단일 스레드 초당 약 530문장) 처리
* **발음 및 정규화 정확도**: 
  - 국립국어원 표준 발음법 전수 검증
  - Head 구조 개편을 통한 정밀한 의미 분석으로 높은 정확도 달성
  - 한국인이 발음하는 자연스러운 영어 단어 발음
* **다양한 사용자 옵션**: 
  - **자동 끊어읽기**: 텍스트 분석 기반 자동 마침표, 쉼표 및 운율 쉼표(`[P1]~[P3]`, SSML `<break>`) 생성
  - **대화체 선택 (`speech_style`)**: 정중(`formal` / 하십시오체), 친절(`polite` / 해요체), 반말(`plain` / 해라체)
  - **장단음 표기 (`vowel_length`)**: 한국어 표준 모음 장단음 변별 표기 지원
  - **심볼 읽기 형식 (`unit_style`)**: `km/h` → `키로`, `키로미터`, `키로미터퍼아워` 등 상황에 맞는 단위 표기 선택
* **공개 REST API**: 별도 인증 키 없이 누구나 즉시 테스트 가능한 엔드포인트 제공

---

## 🎙️ SNAP + TTS 연동

TTS 엔진마다 요구하는 텍스트 입력 규격(일반 텍스트, 음소 ID, 자모 단위 등)은 제각각입니다. SNAP은 텍스트 정규화(TN), 음운 변동(G2P), 운율 태그를 표준 발음 텍스트, 음소 시퀀스, NFD 자모 등 다양한 형태로 출력하므로 대부분의 TTS 아키텍처와 쉽게 연동됩니다.

SNAP v2.0에서는 REST API를 사용하여 연동하는 것이 표준이며, 다음과 같은 연동 방식을 지원합니다:
1. **클라우드 REST API를 통한 연동**: 별도 설치 없이 공개 API 엔드포인트를 호출하여 연동
2. **로컬 REST API 서버를 통한 연동**: SNAP Docker 컨테이너를 로컬 서버에 설치 후 연동
3. **SNAP SDK를 이용한 연동**: SNAP C++ Shared Library를 통한 직접 링크 연동 (온프레미스 환경용)

다음은 SNAP API 서버와 주요 TTS 모델들과의 연동 사례입니다.
* **클라우드 TTS (Edge-TTS)**: Edge-TTS는 일반 텍스트 원문만으로도 어느 정도 수준의 발음을 합성하지만, SNAP을 전처리기로 연동하면 문맥에 따른 수사·단위 변별, 동철이음어 구분, 사용자 정의 사전(`custom_dict`) 등 SNAP의 정밀한 의미 분석 기능이 더해져 훨씬 더 정확하고 완성도 높은 발음을 구현합니다.
* **Piper TTS**: Piper의 기본 전처리기인 `espeak-ng`는 다국어 지원으로 널리 쓰이지만 한국어·일본어의 복잡한 음운 변동을 처리하기에는 다소 아쉬운 점이 있습니다. SNAP의 음소(Phoneme) ID를 모델 인코더에 직접 매핑하여 이를 보완할 수 있습니다.
* **F5-TTS**: 텍스트를 자모 단위로 처리하는 F5-TTS 특성에 맞춰, SNAP G2P로 음운 변동을 먼저 반영한 뒤 유니코드 NFD(초성·중성·종성)로 분해하여 모델에 전달합니다.

### 모델 학습 시 권장 사항
기존 사전학습 모델의 추론(Inference) 단계에 SNAP을 연결하는 것만으로도 발음을 상당 부분 개선할 수 있습니다. 

다만, **가장 자연스러운 음질과 운율을 얻으려면 음향 모델 학습 단계부터 데이터셋 텍스트 라벨을 SNAP G2P 결과로 통일하여 학습시키는 것이 좋습니다.**

음향 모델은 학습 데이터셋의 전처리 표기 방식에 맞춰 소리의 길이와 억양을 학습합니다. 처음부터 정밀한 한국어 음운 규칙이 반영된 SNAP G2P로 학습 데이터를 구축하면, 모델이 음소 경계와 운율을 더 일관되게 학습할 수 있습니다.

---

## 🚀 SNAP Cloud API

SNAP Cloud API는 SNAP의 표준 연동 API입니다.  
현재 Open SNAP API 서버를 운영하고 있으며, 별도 인증 키 없이 누구나 즉시 호출하여 테스트해 볼 수 있습니다.  
API 주소는 경우에 따라 변경될 수 있으며, 맨 처음 요청(Request) 시에는 Idle 상태에서 깨어나는 시간(Cold Start) 때문에 잠시 딜레이가 생길 수도 있습니다.  
기본 기능은 [SNAP v2.0 실시간 데모](https://huggingface.co/spaces/softguy777/snap_voice_demo4)를 사용하여 확인하는 것이 편리하며, 프로그램 또는 터미널에서 직접 테스트하는 방법은 다음과 같습니다.

* **URL**: `https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize`
* **Method**: `POST`
* **Header**: `Content-Type: application/json`

### Python 예제
```python
import requests

url = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"
payload = {
    "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABCTechnology 본사에서 만나요.",
    "custom_dict": {
        "ABCTechnology": "에이비씨 테크놀로지"
    },
    "config": {
        "lang": "ko",
        "prosody_format": "tags"
    }
}

res = requests.post(url, json=payload).json()
print("정규화:", res["data"]["normalized_text"])
print("발음  :", res["data"]["phonemes"])
# 정규화: 이천이십육년 팔월 이십오일 오후 세시 삼십분에 이호선 삼번 출구 앞 에이비씨 테크놀로지 본사에서 만나요.
# 발음  : 이처니심늉년 파뤌 이시보일 오후 세시 삼십뿌네 이호선 삼번 출구 압 에이비씨 테크놀로지 본사에서 만나요.
```

### cURL 예제
```bash
curl -X POST "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABCTechnology 본사에서 만나요.",
       "custom_dict": {"ABCTechnology": "에이비씨 테크놀로지"}
     }'
```

더 자세한 설정 및 파라미터는 [SNAP v2.0 API 문서](docs/SNAP_REST_API_MANUAL_KO.md)를 참고하세요.

---

## 🏢 Enterprise & On-Premise SDK

폐쇄망 환경 및 대규모 처리를 위한 독립 실행형 Docker 컨테이너 및 C++ Native SDK를 제공합니다.

* **오프라인 환경**: 외부 네트워크 통신 없는 로컬 폐쇄망 구동 지원
* **인터페이스**: C ABI 직접 링크(DLL / so) 및 로컬 REST 마이크로서비스 지원
* **배치 처리 API**: 텐서 일괄 처리(`snap_process_batch`) 지원
* **문의**: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)

---

## 📜 라이선스

* 본 저장소의 예제 코드 및 문서는 [MIT 라이선스](LICENSE)가 적용됩니다.
* SNAP 코어 엔진, 모델 가중치 및 온프레미스 SDK는 별도 계약 자산입니다.
