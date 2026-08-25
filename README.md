# SNAP (Semantic Normalization via Attached Probes)

SNAP is a real-time, context-aware multilingual speech pre-processing (TTS Frontend & ITN) engine supporting Korean, Japanese, and English. It analyzes sentence semantics and context to perform phonetic normalization and prosodic pause prediction in real time.

[English](README.md) | [한국어](README_KO.md)  
[Official Website](https://snap-libs.github.io/snap/) | [Text Frontend Demo](https://huggingface.co/spaces/softguy777/snap-demo) | [MeloTTS Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo) | [Piper Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo2) | [F5-TTS Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

* [SNAP Project Detailed Whitepaper (KO)](docs/Snap%20Project%20Introduction.md)
* [SNAP Multilingual Text Frontend Demo (TN / G2P / Prosody)](https://huggingface.co/spaces/softguy777/snap-demo) — Real-time evaluation of context normalization (TN), phonetic conversion (G2P), and prosodic pauses

## SNAP + Open-Source TTS Live Demos

Many global TTS engines rely on generic pre-processors like `espeak-ng` or simplistic rule-based converters. However, East Asian languages such as Korean and Japanese present complex phonetic variations (nasalization, tensification, liaison), contextual numeral/kanji readings, and homographs, often resulting in distorted pronunciations and awkward phrasing.

In production environments, engineering teams frequently resort to manual pipelines that prompt LLMs to generate phonetic transcripts and employ human reviewers to fix errors.

The three open-source TTS models below (MeloTTS, Piper, F5-TTS) are widely recognized globally but previously lacked robust Korean support due to frontend limitations. Combined with the SNAP engine, each model now produces accurate pronunciation and natural prosody tailored to its architecture.

### MeloTTS Integration (Shared BERT Pipeline)
Shares the underlying RoBERTa backbone between the SNAP frontend and MeloTTS acoustic model in a single forward pass, eliminating redundant BERT compute and minimizing latency. Replaces the legacy g2pk pre-processor.
* [Launch SNAP + MeloTTS Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo)

### Piper VITS Integration (Lightweight On-Device)
Replaces `espeak-ng` with SNAP's precise phonetic conversion, enabling natural Korean speech synthesis across 22 distinct voice styles on lightweight on-device VITS models.
* [Launch SNAP + Piper VITS Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo2)

### F5-TTS Integration & Voice Cloning (Diffusion-Based)
Couples SNAP G2P with NFD jamo decomposition for the state-of-the-art Flow Matching diffusion model F5-TTS. Supports 16 preset speakers and enables accurate **Korean Zero-Shot Voice Cloning** using user reference audio.
* [Launch SNAP + F5-TTS Voice Cloning Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo3)

> **Demo Execution Environment Notice**:
> * **MeloTTS / Piper**: Lightweight models running on 2 vCPU environments with instantaneous real-time generation.
> * **F5-TTS**: Utilizes Hugging Face dynamic GPU allocation for diffusion compute. Queuing delays may occur per generation, and operations may be throttled if monthly GPU quotas are reached.

## SNAP Cloud API (Free)

Public evaluation access for SNAP multilingual (Korean, Japanese, English) text pre-processing is currently open without authentication.

### Key Capabilities
* **Context-Aware Text Normalization**: Contextual numeral disambiguation, dates, time, units, and currency
* **G2P & 3-Tier Prosodic Tagging**: Standard phonetic conversion and 3-tier pause tags (`[P1]~[P3]`, SSML breaks)
* **Dynamic Custom Dictionary (`custom_dict`)**: Instant per-request brand and terminology pronunciation overrides

### Quick Start

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
print("Normalized:", res["data"]["normalized_text"])
print("Phonemes  :", res["data"]["phonemes"])
# Output: 이처니심늉년 파뤌 이시보일 오후 세시 삼십뿌네 이호선 삼번 출구 압 에이비씨 테크놀로지 본사에서 만나요.
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

### Documentation & Recipes
* [REST API Specification Manual](SNAP_REST_API_MANUAL.md) — Parameters, response schemas, and code recipes
* [Example Recipes (examples/)](examples/) — Single sentence (`01`), batch processing (`02`), domain recipes (`03`), and multilingual test (`04`)

## Enterprise & On-Premise SDK (Docker)

For air-gapped, security-critical, or high-throughput enterprise environments, **Standalone SDK Docker Containers** and Native C++ SDKs are available.

* **Air-Gapped & Offline**: Complete network isolation support
* **Ultra-Low Latency**: Direct C++ linkage and local REST microservice
* **Distribution**: Available via prior Service Agreement
* **Contact**: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)
