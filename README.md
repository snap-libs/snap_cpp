# SNAP (Semantic Normalization via Attached Probes)

SNAP is a real-time, context-aware speech pre-processing (TTS Frontend & ITN) engine.  
Building upon the multilingual (Korean, Japanese, English) v1.0 research foundation, we introduce **Korean TTS Frontend Engine v2.0**, engineered for production environments with enhanced C++ performance optimizations and colloquial speech style transformation options. Japanese and English v2.0 will be released in subsequent updates.

[English](README.md) | [한국어](README_KO.md) | [Official Website](https://snap-libs.github.io/snap/)

---

## 🔗 Key Documentation & Live Demos

### 🚀 SNAP v2.0 (Korean)
* 🎮 **[SNAP v2.0 Real-Time TTS Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo4)**: Live interactive evaluation of Korean v2.0 text pre-processing and voice synthesis
* 📑 **[SNAP v2.0 Korean Technical White Paper](docs/SNAP_White_Paper_v2.0.md)**: Distilled Mini BERT backbone, high-performance C++ native optimizations, and latency benchmarks
* 📖 **[SNAP v2.0 Korean Functional Specification](docs/SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md)**: Complete coverage of 30 NIKL standard pronunciation articles, contextual disambiguation, and unit normalization
* 📋 **[SNAP v2.0 API Manual](docs/SNAP_REST_API_MANUAL.md)**: Public REST API parameters and integration specifications

### 🏛️ SNAP v1.0 (Multilingual Research Foundation)
* 📑 **[SNAP v1.0 Technical White Paper](docs/SNAP_White_Paper_v1.0.md)**: Original research white paper on Korean-Japanese-English hybrid Probing Heads
* 🎮 **[SNAP v1.0 Multilingual Frontend Demo](https://huggingface.co/spaces/softguy777/snap-demo)**: Real-time multilingual text normalization and G2P
* 🎙️ **Open-Source TTS Demos**: [MeloTTS](https://huggingface.co/spaces/softguy777/snap_voice_demo) | [Piper](https://huggingface.co/spaces/softguy777/snap_voice_demo2) | [F5-TTS](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

---

## 📌 Korean v2.0 Key Features

* **Real-Time Processing Throughput**: Distilled Mini 4-Layer BERT backbone combined with C++ native optimization achieves an average latency of 1.86 ms per sentence (~530 sentences per second on a single CPU thread, 32 characters baseline).
* **Pronunciation & Normalization Accuracy**: 
  - Exhaustive verification against the National Institute of Korean Language (NIKL) Standard Pronunciation Rules
  - Restructured Probing Heads for refined semantic analysis and superior disambiguation accuracy
  - Natural loanword pronunciation as spoken by native Korean speakers
* **Versatile User Configuration Options**: 
  - **Automatic Prosodic Phrasing**: Algorithmic generation of punctuation and 3-tier pause tags (`[P1]~[P3]`, SSML `<break>`)
  - **Speech Style Selection (`speech_style`)**: Formal (`formal` / Hasipsio-che), Polite (`polite` / Haeyo-che), Plain (`plain` / Haera-che)
  - **Vowel Length Notation (`vowel_length`)**: Standard Korean phonological vowel length marking
  - **Unit Reading Styles (`unit_style`)**: Flexible reading conversions (e.g., `km/h` → `kiro`, `kiromiteo`, or `kiromiteopeoawa`)
* **Public REST API**: Openly accessible endpoint for instantaneous evaluation without requiring authentication keys.

---

## 🎙️ SNAP + TTS Integration

TTS engines have varied requirements for text input representations—ranging from raw text to phoneme IDs and graphemes. SNAP flexibly exports normalized text, G2P phoneme sequences, prosodic break tags, and decomposed NFD jamo, making it seamlessly compatible with virtually any TTS architecture.

In SNAP v2.0, REST API communication is the standard integration paradigm, supporting three deployment modes:
1. **Cloud REST API Integration**: Immediate invocation via the public API endpoint without local installation.
2. **Local REST API Server Integration**: On-premise deployment via standalone SNAP Docker containers.
3. **SNAP SDK Direct Linkage**: Ultra-low-latency direct linking via SNAP C++ Shared Libraries (`.so`, `.dll`).

The following are real-world integration examples connecting SNAP with major TTS architectures:
* **Cloud TTS (Edge-TTS)**: While Edge-TTS handles raw Korean text reasonably well, integrating SNAP as a frontend provides precision contextual numeral readings, heteronym disambiguation, and dynamic custom dictionaries (`custom_dict`), substantially elevating overall pronunciation quality.
* **Piper TTS**: Piper's default pre-processor `espeak-ng` provides broad multilingual support but lacks comprehensive handling of complex Korean phonological mutations. Directly mapping SNAP phoneme IDs into Piper's encoder resolves these pronunciation artifacts.
* **F5-TTS**: Tailored to F5-TTS's character/jamo-level flow matching pipeline, SNAP pre-applies phonological variations and decomposes results into Unicode NFD (Leading Consonant, Vowel, Trailing Consonant) prior to synthesis.

### Best Practices for Acoustic Model Training
While hooking SNAP into the inference pipeline of pre-trained models immediately resolves major pronunciation errors, **we strongly recommend standardizing training dataset text labels with SNAP G2P representations during the acoustic model training phase.**

Acoustic models learn duration and pitch contours directly from the transcript conventions of the dataset. Training on consistent, phonologically accurate SNAP G2P transcripts ensures tighter phoneme alignment boundaries and more natural prosodic cadence.

---

## 🚀 SNAP Cloud API

SNAP Cloud API is the standard integration interface for SNAP.  
A public Open SNAP API server is currently operational, allowing anyone to test text normalization in real time without authentication keys.  
*Note: The API endpoint is subject to change, and initial requests may experience brief cold-start latency if the instance is waking from an idle state.*  
For interactive evaluation, using the [SNAP v2.0 Live Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo4) is recommended. Alternatively, test the API via script or terminal as shown below:

* **URL**: `https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize`
* **Method**: `POST`
* **Header**: `Content-Type: application/json`

### Python Example
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
print("Normalized:", res["data"]["normalized_text"])
print("Phonemes  :", res["data"]["phonemes"])
# Normalized: 이천이십육년 팔월 이십오일 오후 세시 삼십분에 이호선 삼번 출구 앞 에이비씨 테크놀로지 본사에서 만나요.
# Phonemes  : 이처니심늉년 파뤌 이시보일 오후 세시 삼십뿌네 이호선 삼번 출구 압 에이비씨 테크놀로지 본사에서 만나요.
```

### cURL Example
```bash
curl -X POST "https://snap-api-673324870645.asia-northeast3.run.app/v1/normalize" \
     -H "Content-Type: application/json" \
     -d '{
       "text": "2026년 8월 25일 오후 3시 30분에 2호선 3번 출구 앞 ABCTechnology 본사에서 만나요.",
       "custom_dict": {"ABCTechnology": "에이비씨 테크놀로지"}
     }'
```

For complete parameter specifications and advanced configurations, please consult the [SNAP v2.0 API Manual](docs/SNAP_REST_API_MANUAL.md).

---

## 🏢 Enterprise & On-Premise SDK

For air-gapped environments, strict data privacy, and ultra-high-throughput enterprise workloads, SNAP provides standalone Docker containers and native C++ SDK binaries.

* **Air-Gapped Operation**: Full offline deployment without external network egress
* **Flexible Interfaces**: Direct C ABI linking (`.dll`, `.so`) and local REST microservices
* **Batch Processing**: Zero-overhead batch tensor inference (`snap_process_batch`)
* **Inquiries**: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)

---

## 📜 License

* Client examples, scripts, and documentation in this repository are licensed under the [MIT License](LICENSE).
* The SNAP Core Engine, model weights, and On-Premise SDKs are proprietary assets governed by separate commercial agreements.
