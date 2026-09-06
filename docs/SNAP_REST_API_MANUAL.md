# 🌐 SNAP HTTP REST API Reference Manual

[English](SNAP_REST_API_MANUAL.md) | [한국어](SNAP_REST_API_MANUAL_KO.md)

> **📌 Document Guidance**  
> * **This Document (`SNAP_REST_API_MANUAL.md`):** Cloud-based **Web / Server HTTP REST API (FastAPI, JSON Endpoints)** Reference Specification.  
> * **Korean Version (한국어 매뉴얼):** Please refer to [`SNAP_REST_API_MANUAL_KO.md`](SNAP_REST_API_MANUAL_KO.md).  
> * **C/C++ Native SDK:** For embedded C-API headers, DLL/SO, and memory management, please refer to [`SNAP_SDK_API_MANUAL.md`](SNAP_SDK_API_MANUAL.md).

---

## 1. Overview

The SNAP HTTP REST API provides context-aware Text Normalization (TN), numeral expansion, heteronym disambiguation, 3-tier prosodic pause tagging (`[P1]/[P2]/[P3]`), International Phonetic Alphabet (IPA) transcription, and dynamic custom dictionary substitution (`custom_dict`) via web-standard JSON endpoints with ultra-low latency (~20ms).

* **Hosting Infrastructure:** Google Cloud Run (Seoul Region: `asia-northeast3`)
* **Live Base URL:** `https://snap-api-673324870645.asia-northeast3.run.app`
* **Interactive Swagger UI:** [https://snap-api-673324870645.asia-northeast3.run.app/docs](https://snap-api-673324870645.asia-northeast3.run.app/docs)
* **ReDoc Documentation:** [https://snap-api-673324870645.asia-northeast3.run.app/redoc](https://snap-api-673324870645.asia-northeast3.run.app/redoc)
* **OpenAPI Specification:** `/openapi.json`

---

## 2. Common HTTP Headers & Tracing

### Request Headers
| Header | Required | Description | Example |
| :--- | :---: | :--- | :--- |
| `Content-Type` | **Required** | MIME type of request body | `application/json` |
| `X-Request-ID` | Optional | Client-side distributed tracing ID (auto-generated if omitted) | `req_custom_12345` |
| `X-API-Key` | Optional | Authorization key when authentication is enabled | `snap_live_secret_key` |

### Response Headers
| Header | Description | Example |
| :--- | :--- | :--- |
| `X-Request-ID` | Unique transaction ID assigned to this request | `req_12f76682d0fc` |
| `X-Response-Time` | Pure server-side engine processing latency | `21.19ms` |

---

## 3. API Endpoints

---

### 3.1. Service Health Check (`GET /v1/health`)

Verify service health and C++ model engine loading status.

* **Method:** `GET`
* **Path:** `/v1/health`

#### Response Body (200 OK)
```json
{
  "status": "healthy",
  "version": "1.0.0",
  "engine_loaded": true,
  "active_languages": ["ko", "ja", "en"]
}
```

---

### 3.2. Language & Feature Discovery (`GET /v1/languages`)

Retrieve active languages and available engine feature flags.

* **Method:** `GET`
* **Path:** `/v1/languages`

#### Response Body (200 OK)
```json
{
  "success": true,
  "default_language": "ko",
  "active_languages": ["ko", "ja", "en"],
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

### 3.3. Single Sentence Normalization & G2P (`POST /v1/normalize`)

Perform neural context analysis, numeral expansion, loanword preservation, heteronym disambiguation, and G2P phonetic transcription on a single text string.

* **Method:** `POST`
* **Path:** `/v1/normalize`

#### Request Parameters (Schema)
| Field | Type | Default | Description |
| :--- | :---: | :---: | :--- |
| `text` | `string` | **(Required)** | Input text to normalize and convert to phonemes (max 5,000 chars) |
| `custom_dict` | `object` | `null` | Dynamic custom dictionary (`{"surface": "reading"}`) |
| `config.lang` | `string` | `"ko"` | Target language code (`"ko"`, `"ja"`, `"en"`) |
| `config.prosody_format` | `string` | `"tags"` | Prosody pause format: `"tags"` (`[P1]/[P2]/[P3]`), `"ssml"` (`<break .../>`), `"none"` |
| `config.return_ipa` | `boolean` | `false` | Generate and return International Phonetic Alphabet (IPA) |
| `config.vowel_length` | `boolean` | `true` | (Korean) Enable vowel length colon notation (`:`) |
| `config.unit_style` | `string` | `"standard"` | (Korean) Unit normalization style: `"standard"` (default e.g. 120km/h -> 백이십킬로미터), `"full"` (e.g. 백이십킬로미터퍼아워), `"short"` (colloquial e.g. 백이십키로, 70kg -> 칠십키로, 100% -> 백프로) |
| `config.speech_style` | `string` | `"original"` | (Korean) Conversational speech style for endings and pronouns: `"original"` (default), `"haeyo"` (polite informal), `"banmal"` (casual), `"hapsio"` (formal polite) |
| `config.pitch_accent` | `boolean` | `true` | (Japanese) Enable pitch accent kernel notation |
| `config.script` | `string` | `"katakana"` | (Japanese) Orthography type: `"katakana"`, `"hiragana"`, `"romaji"` |

> 💡 **3-Tier Cascading Configuration Hierarchy**:  
> 1. **Default Preservation**: Any omitted field in `config` safely retains server defaults (`snap_config.json`).  
> 2. **Selective Override**: Explicitly provided fields (`return_ipa`, `prosody_format`, etc.) surgically override settings for that request only.  
> 3. **100% Backward Compatible**: Default calls (`{"text": "..."}`) work seamlessly without code changes.

#### Request Example (Request Body)
```json
{
  "text": "NVIDIA RTX 4090 GPU에서 v1.2.0 C++ SDK를 100% 빌드 완료했습니다.",
  "custom_dict": {
    "NVIDIA": "엔비디아"
  },
  "config": {
    "lang": "ko",
    "prosody_format": "tags",
    "return_ipa": false,
    "vowel_length": true
  }
}
```

#### Response Example (Response Body - 200 OK)
```json
{
  "success": true,
  "data": {
    "original_text": "NVIDIA RTX 4090 GPU에서 v1.2.0 C++ SDK를 100% 빌드 완료했습니다.",
    "normalized_text": "엔비디아 알티엑스 사공구공 지피유에서 브이 일쩜이쩜영 씨플러스플러스 에스디케이를 백퍼센트 빌드 완료했습니다.",
    "phonemes": "엔비디아 알티엑쓰 사공구공 지피유에서[P1] 브이 일쩌미쩌명 씨플러스플러스 에스디케이를[P2] 백퍼센트 빌드 왈료핻씀니다.[P3]",
    "ipa": null,
    "pos_tags": null
  },
  "meta": {
    "latency_ms": 21.45,
    "engine_version": "1.0.0"
  }
}
```

---

### 3.4. Batched Sentence Normalization (`POST /v1/normalize/batch`)

Process multiple sentences in a single batched HTTP transaction using SIMD-vectorized neural forward passes.

* **Method:** `POST`
* **Path:** `/v1/normalize/batch`

#### Request Example (Request Body)
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

#### Response Example (Response Body - 200 OK)
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
      "latency_ms": 7.12
    },
    {
      "original_text": "물가 안정을 위해 500억 원의 재정을 투입합니다.",
      "normalized_text": "물가 안정을 위해 오백억 원의 재정을 투입합니다.",
      "phonemes": "물가 안정을 위해 오배걱 워네 재정을 투이팜니다.",
      "ipa": null,
      "latency_ms": 7.12
    },
    {
      "original_text": "서울에서 부산까지 KTX로 2시간 15분 걸립니다.",
      "normalized_text": "서울에서 부산까지 케이티엑스로 두시간 십오분 걸립니다.",
      "phonemes": "서우레서 부산까지 케이티엑스로 두시간 시보분 걸림니다.",
      "ipa": null,
      "latency_ms": 7.12
    }
  ],
  "meta": {
    "total_latency_ms": 21.36,
    "avg_latency_ms": 7.12,
    "engine_version": "1.0.0"
  }
}
```

---

## 4. Advanced Configuration Options

### 4.1. Unit Reading Style (`unit_style`)
Select numerical unit expansion format based on target domain (Navigation, News, Conversational Assistant):

| Mode | Option Value | `120km/h` | `70kg` | `180cm` | `100%` | `16GB` | Target Domain |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **Standard** | `"standard"` (default) | 백이십킬로미터 | 칠십킬로그램 | 백팔십센티미터 | 백퍼센트 | 십육기가바이트 | News, In-car navigation |
| **Full Name** | `"full"` | 백이십킬로미터퍼아워 | 칠십킬로그램 | 백팔십센티미터 | 백퍼센트 | 십육기가바이트 | Technical, Academic papers |
| **Colloquial Short** | `"short"` | 백이십키로 | 칠십키로 | 백팔십센티 | 백프로 | 십육기가 | Voice AI assistant, Casual Chat |

### 4.2. Conversational Speech Style (`speech_style`)
Convert formal news-tone declarative sentence endings into friendly polite (`haeyo`), casual (`banmal`), or formal polite (`hapsio`) conversational speech styles with automatic G2P liaison and contraction:

| Style | Option Value | Ending Transformation Example | Pronoun Contraction & Agreement |
| :--- | :--- | :--- | :--- |
| **Original** | `"original"` (default) | `도착했다`, `학생이다`, `어디 가십니까?`, `앉으십시오!` | `그것은`, `무엇을`, `저는` (Preserves original) |
| **Haeyo (Polite)** | `"haeyo"` | `도착했어요`, `학생이에요` / `의사예요`, `어디 가요?`, `앉으세요!` | `그건`, `뭘`, `저는` (Polite friendly conversational) |
| **Banmal (Casual)** | `"banmal"` | `도착했어`, `학생이야` / `의사야`, `어디 가?`, `앉아!` | `그건`, `뭘`, `나는`, `내가`, `우리` (Informal friendly flat style) |
| **Hapsio (Formal)** | `"hapsio"` | `도착했습니다`, `학생입니다`, `어디 가십니까?`, `앉으십시오!` | `그건`, `뭘`, `저는` (Polite formal style) |

---

## 5. Error Handling & Unified Error Envelope

When an error occurs, SNAP returns a standard RFC 7807 compliant error envelope:

```json
{
  "success": false,
  "error": {
    "code": "UNSUPPORTED_LANGUAGE",
    "message": "Language 'fr' is not active or supported. Active languages: ['ko', 'ja', 'en']",
    "details": {
      "requested_lang": "fr",
      "active_languages": ["ko", "ja", "en"]
    }
  },
  "request_id": "req_84d7281f9a0c",
  "timestamp": "2026-08-29T00:35:00.000Z"
}
```

### Standard Error Codes
| HTTP Status | Error Code | Description |
| :---: | :--- | :--- |
| `400` | `INVALID_INPUT` | Text is empty or exceeds length limit (5,000 chars) |
| `400` | `UNSUPPORTED_LANGUAGE` | Requested language is not supported or not loaded |
| `422` | `UNPROCESSABLE_ENTITY` | Schema validation error in JSON payload |
| `500` | `INTERNAL_SERVER_ERROR` | Unhandled engine or server exception |

---

## 6. Client Integration Examples

### cURL
```bash
curl -X POST "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "NVIDIA RTX 4090 GPU에서 v1.2.0 C++ SDK를 100% 빌드 완료했습니다.",
       "config": {
         "lang": "ko",
         "prosody_format": "tags",
         "return_ipa": false
       }
     }'
```

### Python (`requests`)
```python
import requests

url = "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize"
payload = {
    "text": "2026년 8월 24일 오후 3시 30분에 2호선 3번 출구에서 만나요.",
    "config": {
        "prosody_format": "tags",
        "return_ipa": False
    }
}
resp = requests.post(url, json=payload)
data = resp.json()
print("Phonemes:", data["data"]["phonemes"])
```

### JavaScript / Node.js (`fetch`)
```javascript
const response = await fetch("https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({
    text: "물가 안정을 위해 500억 원의 재정을 투입합니다.",
    config: { prosody_format: "none" }
  })
});
const result = await response.json();
console.log("Phonemes:", result.data.phonemes);
```
