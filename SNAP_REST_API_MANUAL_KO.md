# 🌐 SNAP HTTP REST API Reference Manual

> **📌 문서 구분 안내**  
> * **본 문서 ([`SNAP_REST_API_MANUAL.md`](file:///c:/work/snap/SNAP_REST_API_MANUAL.md)):** 클라우드 기반 **웹/서버 HTTP REST API (FastAPI, JSON 엔드포인트)** 연동 규격서입니다.  
> * **C/C++ 네이티브 라이브러리 연동:** C-API 임베디드 DLL/SO 헤더 연동은 [`SNAP_SDK_API_MANUAL.md`](file:///c:/work/snap/SNAP_SDK_API_MANUAL.md)를 참조하세요.

---

## 1. 개요 (Overview)

SNAP HTTP REST API는 한국어 문맥 기반 텍스트 정규화(Text Normalization), 동철이음이의어 변별, 3단계 운율 쉼(`[P1]/[P2]/[P3]`) 예측, 국제음성기호(IPA) 변환 및 사용자 동적 커스텀 사전(`custom_dict`)을 웹 표준 JSON 프로토콜을 통해 초저지연(~20ms)으로 제공하는 마이크로서비스입니다.

* **운영 인프라:** Google Cloud Run (서울 리전: `asia-northeast3`)
* **라이브 Base URL:** `https://snap-api-673324870645.asia-northeast3.run.app`
* **대화형 Swagger 문서:** [https://snap-api-673324870645.asia-northeast3.run.app/docs](https://snap-api-673324870645.asia-northeast3.run.app/docs)
* **ReDoc 문서:** [https://snap-api-673324870645.asia-northeast3.run.app/redoc](https://snap-api-673324870645.asia-northeast3.run.app/redoc)
* **OpenAPI 스키마:** `/openapi.json`

---

## 2. 공통 HTTP 헤더 및 추적 (Headers & Tracing)

### 요청 헤더 (Request Headers)
| 헤더명 | 필수 여부 | 설명 | 예시 |
| :--- | :---: | :--- | :--- |
| `Content-Type` | **필수** | 요청 바디의 MIME 타입 | `application/json` |
| `X-Request-ID` | 선택 | 클라이언트 측 요청 추적 ID (미입력 시 서버 자동 발급) | `req_custom_12345` |
| `X-API-Key` | 선택 | 인증이 활성화된 인스턴스 접근 시 사용 | `snap_live_secret_key` |

### 응답 헤더 (Response Headers)
| 헤더명 | 설명 | 예시 |
| :--- | :--- | :--- |
| `X-Request-ID` | 해당 요청에 부여된 고유 식별자 | `req_12f76682d0fc` |
| `X-Response-Time` | 서버 내부 순수 연산 및 처리 지연시간 | `21.19ms` |

---

## 3. 엔드포인트 상세 규격 (Endpoints)

---

### 3.1. 서비스 헬스체크 (`GET /v1/health`)

API 서버의 동작 상태 및 C++ 모델 엔진의 적재 여부를 확인합니다.

* **Method:** `GET`
* **Path:** `/v1/health`

#### 응답 바디 (Response Body - 200 OK)
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "engine_loaded": true,
  "active_languages": ["ko"]
}
```

---

### 3.2. 지원 언어 및 기능 조회 (`GET /v1/languages`)

서버 인스턴스에서 활성화된 언어 목록과 지원 기능 플래그를 조회합니다.

* **Method:** `GET`
* **Path:** `/v1/languages`

#### 응답 바디 (Response Body - 200 OK)
```json
{
  "success": true,
  "default_language": "ko",
  "active_languages": ["ko"],
  "engine_version": "1.0.0",
  "features": {
    "prosody_tier": true,
    "ssml_break": true,
    "custom_dict": true,
    "ipa_output": true,
    "batch_processing": true
  }
}
```

---

### 3.3. 단일 문장 정규화 및 G2P (`POST /v1/normalize`)

단일 텍스트를 문맥 분석하여 수사 변환, 동철이음이의어 처리, 운율 쉼표 태깅, 표준 발음(G2P) 및 IPA를 생성합니다.

* **Method:** `POST`
* **Path:** `/v1/normalize`

#### 요청 파라미터 (Request Schema)
| 필드명 | 타입 | 기본값 | 설명 |
| :--- | :---: | :---: | :--- |
| `text` | `string` | **(필수)** | 정규화 및 발음 변환할 원문 (최대 5,000자) |
| `custom_dict` | `object` | `null` | 사용자 동적 커스텀 사전 (`{"원문단어": "치환발음"}`) |
| `config.lang` | `string` | `"ko"` | 대상 언어 코드 (`"ko"`, `"ja"`, `"en"`) |
| `config.prosody_format` | `string` | `"tags"` | 운율 쉼 표기 방식: `"tags"` (`[P1]/[P2]/[P3]`), `"ssml"` (`<break .../>`), `"none"` |
| `config.return_ipa` | `boolean` | `false` | 국제 음성 기호(IPA) 생성 및 반환 여부 |
| `config.vowel_length` | `boolean` | `true` | (한국어 전용) 모음 장단음 표기 여부 (`true`: '밤:' / `false`: '밤') |
| `config.pitch_accent` | `boolean` | `true` | (일본어 전용) 악센트 핵 표기 여부 |
| `config.script` | `string` | `"katakana"` | (일본어 전용) 출력 문자 체계: `"katakana"`, `"hiragana"`, `"romaji"` |

> 💡 **3단계 캐스케이딩 설정 계층 (Cascading Configuration Hierarchy)**:  
> 1. **기본값 보존**: 사용자가 `config`에 특정 필드를 생략하면, 서버 파일(`snap_config.json`)의 안전 기본값이 유지됩니다.  
> 2. **선택적 오버라이드**: 사용자가 넘겨준 필드(`return_ipa`, `prosody_format` 등)만 해당 요청에 한하여 외과적으로 덮어씌워집니다.  
> 3. **완벽한 하위 호환**: 기존 호출 방식(`{"text": "..."}`)은 아무런 코드 수정 없이 그대로 100% 호환됩니다.

#### 요청 예시 (Request Body)
```json
{
  "text": "2026년 8월 24일 2호선 3번 출구에서 만나 커피 2잔을 마셨습니다. ChatGPT와 LG CNS를 사용합니다.",
  "custom_dict": {
    "ChatGPT": "챗지피티",
    "LG CNS": "엘지씨엔에스"
  },
  "config": {
    "lang": "ko",
    "prosody_format": "tags",
    "return_ipa": false,
    "vowel_length": true
  }
}
```

#### 응답 예시 (Response Body - 200 OK)
```json
{
  "success": true,
  "data": {
    "original_text": "2026년 8월 24일 2호선 3번 출구에서 만나 커피 2잔을 마셨습니다. ChatGPT와 LG CNS를 사용합니다.",
    "normalized_text": "이천이십육년 팔월 이십사일 이호선 삼번 출구에서 만나 커피 두잔을 마셨습니다. 챗지피티와 엘지씨엔에스를 사용합니다.",
    "phonemes": "이처니심늉년[P1] 파뤌 이십싸일[P2] 이호선 삼번 출구에서 만나[P1] 커피 두자늘 마셛씀니다.[P3] 챋찌피티와[P1] 엘지씨에네스를 사용함니다.[P3]",
    "ipa": "[itɕʰʌnisimnjuŋnjʌn pʰaɾwʌɭ isip̚s͈aiɭ ihosʌn sampʌn tɕʰuɭkuesʌ manna kʰʌpʰi tudʑanɯɭ masʰjʌts͈ɯmnida. tɕʰɛt̚tɕ͈ipʰitʰiwa eɭtɕis͈ienesɯɾɯɭ sajoŋhamnita.]",
    "pos_tags": null
  },
  "meta": {
    "latency_ms": 22.45,
    "engine_version": "1.0.0"
  }
}
```

---

### 3.4. 배치 문장 정규화 (`POST /v1/normalize/batch`)

복수의 문장을 단일 HTTP 트랜잭션으로 묶어 고속 병렬 처리합니다.

* **Method:** `POST`
* **Path:** `/v1/normalize/batch`

#### 요청 예시 (Request Body)
```json
{
  "texts": [
    "제1차 회의는 오전 10시 15분에 시작합니다.",
    "물가 안정을 위해 500억 원의 재정을 투입합니다.",
    "서울에서 부산까지 KTX로 2시간 15분 걸립니다."
  ],
  "custom_dict": {
    "KTX": "케이티엑스"
  },
  "config": {
    "prosody_format": "none"
  }
}
```

#### 응답 예시 (Response Body - 200 OK)
```json
{
  "success": true,
  "total_count": 3,
  "results": [
    {
      "original_text": "제1차 회의는 오전 10시 15분에 시작합니다.",
      "normalized_text": "제일차 회의는 오전 열시 십오분에 시작합니다.",
      "phonemes": "제일차 회이는 오전 열시 시보부네 시자캄니다.",
      "ipa": null,
      "latency_ms": 21.39
    },
    {
      "original_text": "물가 안정을 위해 500억 원의 재정을 투입합니다.",
      "normalized_text": "물가 안정을 위해 오백억 원의 재정을 투입합니다.",
      "phonemes": "물가 안정을 위해 오배걱 워네 재정을 투이팜니다.",
      "ipa": null,
      "latency_ms": 20.88
    },
    {
      "original_text": "서울에서 부산까지 KTX로 2시간 15분 걸립니다.",
      "normalized_text": "서울에서 부산까지 케이티엑스로 두시간 십오분 걸립니다.",
      "phonemes": "서우레서 부산까지 케이티엑스로 두시간 시보분 걸림니다.",
      "ipa": null,
      "latency_ms": 21.05
    }
  ],
  "meta": {
    "latency_ms": 63.32,
    "engine_version": "1.0.0"
  }
}
```

---

## 4. 기능별 상세 옵션 가이드

### 4.1. 운율 포맷 (`prosody_format`)
| 값 | 출력 예시 | 설명 |
| :--- | :--- | :--- |
| `"tags"` (기본값) | `정부는[P1] 국무회의를 열고[P2] 확정했습니다.[P3]` | 음성합성 엔진 전용 3단계 쉼 태그 |
| `"ssml"` | `정부는<break time="150ms"/> 국무회의를...` | W3C 표준 SSML 음성 마크업 |
| `"none"` | `정부는 국무회의를 열고 확정했습니다.` | 쉼 표기 없는 순수 표준 발음 한글 |

### 4.2. 사용자 커스텀 사전 (`custom_dict`)
* **원리:** SNAP 기본 외래어/고유명사 사전보다 **절대 1순위로 우선 적용**됩니다.
* **최장 일치 원칙:** `{"LG": "엘지", "LG CNS": "엘지씨엔에스"}` 등록 시, `"LG CNS"`를 긴 단어로 먼저 매칭하여 완벽 치환합니다.
* **보안 격리:** 요청 1건을 처리하는 순간(인메모리)에만 적용되며, 다른 사용자에게 절대 공유되지 않습니다.

---

## 5. 표준 오류 응답 규격 (Error Envelope)

오류 발생 시 HTTP 상태 코드와 함께 일관된 JSON 봉투(Envelope)를 반환합니다:

```json
{
  "success": false,
  "error": {
    "code": "UNSUPPORTED_LANGUAGE",
    "message": "Language 'fr' is not active or supported in this instance. Active languages: ['ko']",
    "details": {
      "requested_lang": "fr",
      "active_languages": ["ko"]
    }
  },
  "request_id": "req_8df7a9c1",
  "timestamp": "2026-08-24T10:15:52.606Z"
}
```

---

## 6. 클라이언트 연동 코드 예제

### Python (`httpx` / `requests`)
```python
import httpx

url = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"
payload = {
    "text": "1차 회의는 3시에 2호선 3번 출구에서 시작합니다.",
    "custom_dict": {"1차": "일차"},
    "config": {"prosody_format": "tags", "return_ipa": True}
}

response = httpx.post(url, json=payload, timeout=10.0)
result = response.json()
print("정규화:", result["data"]["normalized_text"])
print("표준발음:", result["data"]["phonemes"])
```

### JavaScript / TypeScript (`fetch`)
```javascript
const response = await fetch("https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({
    text: "ChatGPT와 Claude 3.5 Sonnet을 활용합니다.",
    custom_dict: { "ChatGPT": "챗지피티", "Claude 3.5 Sonnet": "클로드 삼점오 소네트" }
  })
});
const data = await response.json();
console.log(data.data.normalized_text);
```

### cURL
```bash
curl -X POST "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "3번 버스를 타고 3번 갈아타세요.",
       "config": {"prosody_format": "ssml"}
     }'
```
