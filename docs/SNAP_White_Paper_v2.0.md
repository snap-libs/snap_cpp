# SNAP Korean v2.0 Technical White Paper

> **Production-Grade Korean Speech Pre-Processing via High-Performance C++ Engine and Distilled Mini BERT**  
> *Ultra-Low-Latency Hybrid Speech Frontend Combining Zero-Allocation Stack Architecture with Lightweight Neural Probing Heads*

**Author**: Jaihyuk Lee ([snap.leejh@gmail.com](mailto:snap.leejh@gmail.com))  
**Published**: September 2026 | **Version**: 2.0.0  

[English](SNAP_White_Paper_v2.0.md) | [한국어](SNAP_White_Paper_v2.0_KO.md)  
[Official Website](https://snap-libs.github.io/snap/) | [Live TTS Demo (Demo 4)](https://huggingface.co/spaces/softguy777/snap_voice_demo4) | [Functional Spec](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md) | [API Manual](SNAP_REST_API_MANUAL.md)

---

## 📑 Table of Contents
1. [Overview: From v1.0 Research to v2.0 Production Engine](#1-overview-from-v10-research-to-v20-production-engine)
2. [Core Architecture (Distilled Mini BERT & Probing Heads)](#2-core-architecture-distilled-mini-bert--probing-heads)
3. [High-Performance Native C++ Optimizations (Zero-Allocation)](#3-high-performance-native-c-optimizations-zero-allocation)
4. [Precision Phonological Rules & Exhaustive NIKL Verification](#4-precision-phonological-rules--exhaustive-nikl-verification)
5. [Latency and Throughput Benchmarks](#5-latency-and-throughput-benchmarks)
6. [Related Documentation & Resources](#6-related-documentation--resources)

---

## 1. Overview: From v1.0 Research to v2.0 Production Engine

SNAP v1.0 established the foundational 'Pure Context Probing' architecture across Korean, Japanese, and English. It successfully eliminated the hallucination risks and computational overhead inherent in generative LLMs while resolving the contextual ambiguities that limited traditional rule-based G2P engines.

**SNAP Korean v2.0** builds on this research foundation, undertaking a comprehensive production-grade overhaul to satisfy the demanding throughput, low latency, and zero-defect pronunciation standards required by large-scale enterprise TTS and real-time conversational AI pipelines:

* **Distilled Mini Backbone**: Introduces a custom Distilled Mini 4-Layer BERT backbone, preserving full semantic context disambiguation while drastically slashing neural inference latency.
* **Zero-Allocation C++ Engine**: Eliminates heap allocations during runtime inference loops and incorporates zero-copy tensor buffers with ping-pong buffer swaps, achieving over 500+ FPS on a single CPU thread.
* **Exhaustive NIKL Coverage**: Provides comprehensive support for all 30 articles of the National Institute of Korean Language (NIKL) Standard Pronunciation Rules, accompanied by colloquial speech style (`speech_style`) conversions and phonological vowel length controls.

---

## 2. Core Architecture (Distilled Mini BERT & Probing Heads)

SNAP v2.0 employs task-specific neural Probing Heads to classify contextual linguistic attributes without generating unconstrained text:

```
[Raw Input Text]
       │
       ▼
┌────────────────────────────────────────┐
│  Distilled Mini 4-Layer BERT Backbone │ (ONNX Runtime / Zero-Copy)
└────────────────────────────────────────┘
       │
       ├─► Counter Head   : Contextual Sino vs. Native Korean numeral/counter classification
       ├─► Heteronym Head : Disambiguation across 9 major Korean homographs
       ├─► Semiotic Head  : Contextual spoken normalization of special symbols and units
       └─► Morph Head     : Context-aware morphological part-of-speech (POS) tagging
       │
       ▼
┌────────────────────────────────────────┐
│   C++ Deterministic Phonology Engine   │ (Zero-Allocation Rule Engine)
└────────────────────────────────────────┘
       │
       ▼
[Normalized Text / G2P Phonemes / Prosodic Break Tags]
```

---

## 3. High-Performance Native C++ Optimizations (Zero-Allocation)

In real-time streaming speech synthesis, frontend pre-processing latency directly dictates the Time-To-First-Token (TTFT) and initial audio playback response. The SNAP v2.0 native C++ engine adheres to strict zero-overhead design rules:

1. **Zero-Allocation Tensor Pipeline**: Dynamic heap operations (`malloc`, `free`, and resizing `std::vector`) are strictly eliminated inside the inference loop. Fixed-size 76KB pre-allocated buffers are continuously recycled.
2. **Double-Buffered Morphological Post-Processing**: Pointer-swapping double buffers eliminate intermediate buffer allocations during token decoding and morpheme transformations.
3. **Static Hash & Whitelist Acceleration**: Costly regular expressions (`std::regex`) are entirely prohibited; phonological rules are implemented via bitmasks, flat arrays, and static lookup tables.

---

## 4. Precision Phonological Rules & Exhaustive NIKL Verification

SNAP Korean v2.0 implements systematic handling across Articles 1 through 30 of the official Korean Standard Pronunciation Rules:

* **Coda Neutralization & Cluster Simplification (Articles 8–16)**: Deterministic representation of representative obstruents, consonant cluster reduction, and liaison distinction between lexical morphemes and grammatical particles.
* **Assimilation & Tensification (Articles 17–28)**: Strict handling of nasalization, liquidization, inter-vocalic tensification (Sai-sori), and verbal stem tensification.
* **Homograph Disambiguation**: Perfect discrimination of homographs whose pronunciations depend on grammatical semantics (e.g., `물가` [mul-kka] (cost of living) vs. `물가` [mul-ga] (waterside)).
* **Specification Details**: For exact rule mappings, conditions, and unit normalization tables, please refer to the [SNAP Korean v2.0 Functional Specification](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md).

---

## 5. Latency and Throughput Benchmarks

* **Average Latency**: **1.86 ms** per sentence (32 characters baseline)
* **Processing Throughput**: Approximately **530 sentences per second** on a single CPU thread (500+ FPS)
* **Golden Baseline Verification**: 100% regression pass across 10,033 sentences from the Korean Golden Baseline Evaluation Dataset (97.12%+ exact phonetic match)

---

## 6. Related Documentation & Resources

* 📖 **[SNAP Korean v2.0 Functional Specification](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md)**: Exhaustive 30-article rule breakdown and unit normalization catalog
* 📋 **[SNAP v2.0 REST API Manual](SNAP_REST_API_MANUAL.md)**: Request/response schemas, cascading parameters, and code recipes
* 💻 **[SNAP Native C/C++ SDK Manual](SNAP_SDK_API_MANUAL.md)**: C-API lifecycle, ABI definitions, and direct linkage guidelines
* 🎮 **[SNAP v2.0 Live Interactive Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo4)**: Web evaluation sandbox for Korean v2.0
