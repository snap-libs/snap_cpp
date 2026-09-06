# SNAP Korean v2.0 Technical White Paper

> **Production-Grade Korean Speech Pre-Processing via High-Performance Native C++ Engine and Distilled BERT**  
> *A Hybrid Speech Frontend Combining a High-Performance C++ Native Pipeline with Lightweight Neural Probing Heads*

**Author**: Jaihyuk Lee ([snap.leejh@gmail.com](mailto:snap.leejh@gmail.com))  
**Published**: September 2026 | **Version**: 2.0.0  

[English](SNAP_White_Paper_v2.0.md) | [한국어](SNAP_White_Paper_v2.0_KO.md)  
[Official Website](https://snap-libs.github.io/snap/) | [Live TTS Demo (Demo 4)](https://huggingface.co/spaces/softguy777/snap_voice_demo4) | [Functional Spec](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md) | [API Manual](SNAP_REST_API_MANUAL.md)

---

## 📑 Table of Contents
1. [Overview](#1-overview)
   - 1.1. Why SNAP is Essential
   - 1.2. Milestones Achieved in SNAP v1.0
   - 1.3. Evolution into the SNAP v2.0 Production Engine
2. [Core Architecture: Hybrid Pipeline & Lightweight Backbone](#2-core-architecture-hybrid-pipeline--lightweight-backbone)
   - 2.1. Overall Hybrid Pipeline Architecture
   - 2.2. Lightweight Backbone via Knowledge Distillation (12L → 4L/3L)
3. [Korean Phonetic Conversion and Phonological System](#3-korean-phonetic-conversion-and-phonological-system)
4. [Quantitative Benchmarks and Comprehensive Performance Verification](#4-quantitative-benchmarks-and-comprehensive-performance-verification)
   - 4.1. Empirical Latency and Throughput Benchmarks across 1,000 Sentences
   - 4.2. Precision Comparison across Backbone Layer Depths (12L vs 4L vs 3L)
   - 4.3. Stress Test Results on Ultra-Long Sentences
   - 4.4. Full-Coverage Golden Baseline Verification (10,000 Sentences)
5. [Production Integration and Real-Time Pipeline Linkage](#5-production-integration-and-real-time-pipeline-linkage)
   - 5.1. REST API and Real-Time Streaming Architecture
   - 5.2. C/C++ Native SDK and ABI Interface
   - 5.3. Pipeline Linkage with Modern Acoustic Models (MeloTTS, Piper, F5-TTS)
   - 5.4. Zero-Downtime Hot-Patching with Dynamic Custom Dictionary (custom_dict)
6. [Conclusion and Future Roadmap](#6-conclusion-and-future-roadmap)
- [Related Documents & Resources](#-related-documents--resources)

---

## 1. Overview

### 1.1. Why SNAP is Essential

#### ① Cascading Error Propagation in TTS Pipelines
Text-to-Speech (TTS) systems typically follow a sequential pipeline: **Text Normalization (TN) → Grapheme-to-Phoneme (G2P) → Acoustic Model → Vocoder**.  
In this multi-stage architecture, any mispronunciation, unnormalized symbol, or non-standard phonetic transcription originating at the frontend (TN/G2P) stage propagates downstream to the acoustic model without any opportunity for downstream self-correction. Consequently, frontend pre-processing fidelity directly dictates the ultimate intelligibility and naturalness of the synthesized speech.

All natural languages inherently exhibit varying degrees of **Phonological Ambiguity**, where orthographically identical strings diverge dramatically in pronunciation depending on syntactic context and semantic nuance:
* **Korean**: Homographs and heteronyms whose tensification depends on context:  
  - `"희생의 대가를 치르다"` [대까] (cost / price) vs `"서예의 대가를 만나다"` [대가] (master / authority)
* **English**: Morpho-syntactic heteronyms varying by part-of-speech and tense:  
  - `"I read books every day"` [riːd] (present tense) vs `"I read the book yesterday"` [rɛd] (past tense)
* **Japanese**: Kanji orthographies alternating between Sino-Japanese (Onyomi) and native Japanese (Kunyomi) readings based on lexical composition:  
  - `"一日"` [ついたち (first day of the month)] vs [いちにち (one full day)]
* **Universal Numerals & Complex Formats**: Delimiter-separated numbers that shift phonetic realization based on contextual semantics:  
  - `"10:12"`: `"현재 시각은 10:12입니다"` **[열 시 십이 분]** (clock time) vs `"경기 결과 10:12로 끝났다"` **[십 대 십이]** (game score)

Statistically, ambiguous sentences constitute only **3% to 15%** of common conversational corpora. As a result, basic regular expressions (regex), small lookup tables, and elementary rules can process 85% to 97% of standard sentences acceptably.

**However, the persistent occurrence of this small fraction of pronunciation errors breaks downstream speech synthesis catastrophically.**  
Modern deep-learning acoustic models and neural vocoders can synthesize human-level timbre, subtle breathing, and intricate prosody. Yet, no matter how sophisticated the acoustic model is, **if the input text frontend feeds an incorrect reading, the entire speech synthesis pipeline fails.** High-fidelity TTS quality is ultimately gated by **frontend pre-processing integrity**.

For this reason, commercial audio production environments—such as audiobooks, broadcast dubbing, and AI Contact Centers (AICC)—have been unable to achieve end-to-end automation. Production teams remain burdened by **human-in-the-loop verification**, manually proofreading scripts, overriding mispronounced words with phonetic spellings, and inserting SSML tags by hand.

This limitation becomes critical in **Interactive Conversational Voice AI (LLM-to-Speech) and real-time streaming environments**, where pre-validation by humans is physically impossible. Unfiltered frontend errors are exposed directly to end users, severely compromising conversational trust.

#### ② Structural Limitations and Trade-Offs of Existing Approaches

Existing speech frontend methodologies each possess distinct advantages alongside fundamental architectural constraints:

* **1. Rule-Based and Finite-State Transducer (FST) Approaches**:
  * **Representative Systems**: OpenFST / Thrax (Google), espeak-ng, g2pK, Misaki, Gruut.
  * **Mechanism**: Executes linear pattern matching over surface strings using regular expressions, substitution tables, or compiled Finite State Transducers.
  * **Strengths**: Extremely low computational cost, microsecond-level execution latency, and 100% deterministic reproducibility.
  * **Limitations**: Relies strictly on surface character patterns and cannot respond to semantic context. Attempting to address contextual edge cases solely through heuristic rules creates deeply nested, brittle conditional branches that inevitably trigger cross-rule collisions.

* **2. Statistical Morphological Analysis & Dynamic Programming (Lattice DP / Viterbi)**:
  * **Representative Systems**: MeCab / MeCab-ko, Kuromoji, Sudachi, Jieba, Phonetisaurus, KoNLPy.
  * **Mechanism**: Generates a word lattice representing all segmentable morpheme candidates and applies dynamic programming (Viterbi algorithm on HMMs or Linear-chain CRFs) to select the optimal path minimizing transition and emission costs.
  * **Strengths**: Flexibly resolves tokenization boundaries and canonical part-of-speech (POS) tags by accounting for statistical morpheme transition patterns.
  * **Limitations**:
    1. **Local Markov Assumption**: Constrained to adjacent 1-to-2 token transitions, failing to model long-range semantic dependencies spanning entire sentences.
    2. **Inability to Disambiguate Identical POS Classes**: Statistical DP optimizes for the most probable POS tag sequence. However, homographs frequently share identical surface POS tags (e.g., both readings of *대가* are nouns). Because transition costs are identical, Lattice DP cannot structurally decide whether tensification (경음화) should apply based on semantic nuance.
    3. **Ambiguity in Numeral-Counter Systems**: Digit sequences combined with bound unit nouns share identical POS tags regardless of whether the predicate dictates native or Sino-Korean counting.

* **3. Neural Seq2Seq / Translation-Based Approaches**:
  * **Representative Systems**: NVIDIA NeMo (Neural TN/G2P), Google Transformer-TN, Deep-Phonemizer, g2pE.
  * **Mechanism**: End-to-end encoder-decoder translation networks (RNN, Transformer) trained to generate normalized text or phoneme strings directly from raw characters.
  * **Strengths**: Leverages self-attention to capture broad, flexible contextual nuances across entire sentences.
  * **Limitations**: Generative architectures inherently suffer from **token omission, hallucination, and arbitrary substitutions**. In high-reliability TTS pipelines, unconstrained generative drift is unacceptable.

* **4. Large Language Model (LLM) Approaches**:
  * **Representative Systems**: NeMo Canary, ChatTTS Text Frontend, Foundation Models (GPT-4o, Claude) via Few-Shot / Instruction Tuning.
  * **Mechanism**: Utilizes large pretrained foundation models to infer context and generate normalized transcripts.
  * **Strengths**: State-of-the-art semantic comprehension, world knowledge, and fluent colloquial reasoning.
  * **Limitations**: Massive parameter counts introduce **prohibitive compute costs and 200–500+ ms inference latencies**, making them impractical for millisecond-level conversational streaming and low-resource on-device environments.

#### ③ The SNAP Solution: Why BERT, Not Generative LLMs?

To overcome this dilemma, SNAP adopts a hybrid architecture that strictly decouples **"Neural Context & Semantic Representation"** from **"Deterministic Rule Execution"**.

This design stems from a foundational realization: **"The role required of a neural network in a speech frontend is not unconstrained text generation, but contextual Word Sense Disambiguation (WSD)."**

##### 1) The Essence of the Problem: Classification, Not Generation
Text Normalization (TN) and Grapheme-to-Phoneme (G2P) must strictly preserve the sentence structure and word order of the original text. The ambiguity is fundamentally a **Word Sense Disambiguation (WSD)** problem—identifying which semantic class an ambiguous token belongs to within a given context.  
Autoregressive generative decoders (e.g., GPT, Seq2Seq decoders) generate tokens sequentially, introducing unnecessary latency overhead and hallucination risks. An **Encoder-only** architecture that deeply encodes contextual representations is sufficient and optimal.

##### 2) Theoretical Grounding: BERT and Representation Probing
SNAP selects BERT (Bidirectional Encoder Representations from Transformers) as its backbone based on:
* **Bidirectional Contextualization**: Phonetic readings depend heavily on both preceding words and succeeding particles, units, and predicates. BERT's bidirectional self-attention provides the ideal mathematical representation for resolving phonological ambiguity.
* **Representation Probing Theory**: Extensive research (Tenney et al., 2019; Hewitt & Liang, 2019) has demonstrated that hidden states of pretrained BERT encoders densely internalize syntactic, morphological, and high-level lexical semantics in geometric vector space.
* **Single Forward Pass Low-Latency Inference**: Attaching single-layer linear classifiers (**Span-Level Probing Heads**) onto encoder hidden vectors enables microsecond-level semantic extraction via a single non-autoregressive forward pass.

##### 3) Strict Separation of Responsibilities
* **Neural Network = Context Provider (Span-Level Probing Heads)**:
  * Does not generate text or mutate phoneme sequences directly.
  * Operates on top of a frozen, distilled mini BERT backbone to classify semantic tags and POS attributes of ambiguous spans in microseconds, passing them directly to the rule engine.
* **Rule Engine = Final Deterministic Decision Maker (100% Deterministic Engine)**:
  * Using neural semantic tags, the native C++ engine executes standard phonological transformations (liaison, nasalization, tensification) in strict compliance with the National Institute of Korean Language (NIKL) rules.
  * Completely eliminates generative hallucination, guarantees deterministic repeatability, and allows instant correction via user custom dictionaries.
* **Flexible Deployment Modes**:
  * **Standalone Mode**: Compact memory footprint (~50–60MB) and millisecond latency (<3ms on CPU), fully satisfying real-time streaming constraints.
  * **Embedded Mode (BERT-Integrated TTS)**: In models that already run BERT internally for prosody modeling (e.g., MeloTTS, BERT-VITS2), SNAP shares the computed hidden states, reducing frontend inference latency to near-zero (microseconds).

---

### 1.2. Milestones Achieved in SNAP v1.0

SNAP v1.0 established the **'Pure Context Probing'** architecture across Korean, Japanese, and English, demonstrating that contextual ambiguity could be resolved without generative overhead:

* **Multilingual Frontend Probing Proof-of-Concept**:
  * Demonstrated that complex phonological ambiguities across Korean (Sino vs. Native numbers, tensified homographs), Japanese (Kanji Onyomi vs. Kunyomi vs. Jukujikun), and English (POS-dependent heteronyms) could be resolved with high accuracy using **1-layer lightweight Probing Heads attached to frozen BERT representations**.
* **Complete Replacement of Legacy Morphological Analyzers (Morph Head)**:
  * Overcame decades-long structural bottlenecks of legacy tokenizers (MeCab, Sudachi)—such as dictionary obsolescence, opaque Viterbi errors, and heavy native dependencies.
  * Combined static Trie lookup features with BERT representations into a dedicated **Morph Head** performing character-level BIO-POS tagging, cleanly segmenting neologisms and compound nouns without external analyzers.
* **Validation of Single-Encoder Multi-Head Architecture**:
  * Validated that a single unified BERT backbone supporting dedicated Probing Heads (Morph, Counter, Heteronym, Semiotic) handles all frontend pre-processing tasks efficiently.
* **Empirical Integration with Open-Source Acoustic Models**:
  * Verified end-to-end integration with MeloTTS, Piper, and F5-TTS, proving that speech intelligibility and naturalness improved substantially without manual script overriding.

---

### 1.3. Evolution into the SNAP v2.0 Production Engine

SNAP Korean v2.0 takes the core hybrid research from v1.0 and executes a comprehensive overhaul to achieve the **extreme throughput, resource efficiency, and linguistic fidelity required by enterprise production environments**:

* **Backbone Compression & Knowledge Distillation (KD)**:
  * Transferred contextual representations from a 12-Layer Base BERT teacher model into student models via Knowledge Distillation.
  * Compressed **3-Layer and 4-Layer Mini BERT backbones achieve identical semantic disambiguation accuracy** while slashing model size by half (54MB) and accelerating inference speed by 3x.
* **Decoupled Independent Sub-Heads & On-Demand Inference**:
  * In v1.0, multi-word homograph classes were grouped into a single unified head, causing negative cross-class interference across distinct decision boundaries.
  * v2.0 refactors these into **Decoupled Independent Sub-Heads** for each vocabulary cluster.
  * Incorporates **On-Demand Conditional Inference**, invoking sub-heads only when target keywords appear in text, entirely eliminating cross-class interference and redundant computations.
* **Native C++ Production Optimization**:
  * Completely transitioned runtime execution to a **native C++17 engine**.
  * Implements **pre-allocated buffer pooling**, a **single-pass LUT/bitmask phonology scanner** free from backtracking overhead, and **1D Flat DP (Viterbi path search)** to eliminate recursive stack costs, achieving sub-3ms latency on standard CPUs.
* **Exhaustive NIKL Standard Pronunciation Implementation (Articles 1–30)**:
  * Implemented all 30 articles of the National Institute of Korean Language (NIKL) Standard Pronunciation Rules.
  * Resolves complex phonological changes—such as Article 26 (tensification after Sino-Korean 'ㄹ'), Article 24 (tensification after verbal stems), and Article 30 (Sai-siot compound tensification)—by coupling them with neural Probing Heads, validated against a 10,000-sentence Golden Baseline.
* **Operational Control Options for Diverse Domains**:
  * Out-of-the-box controls tailored for AI assistants, AICC, audiobooks, and navigation:
    - **Speech Style Conversion (`speech_style`)**: Automatically mutates literary sentence-final endings into spoken styles (`haeyo` / `hapsio`).
    - **Vowel Length Control (`vowel_length`)**: Identifies standard long vowels and outputs W3C SSML prosody break tags for broadcast-grade recitation.
    - **Pronunciation Style (`pronunciation_style`)**: Configurable modes for strict standard grammar versus everyday conversational idioms.
    - **Dynamic Custom Dictionary (`custom_dict`)**: Instant zero-downtime hot-patching for domain neologisms and brand names.

| Metric / Dimension | SNAP v1.0 (Research Prototype) | SNAP Korean v2.0 (Production Release) | Engineering Significance |
| :--- | :--- | :--- | :--- |
| **Core Engine** | Python research pipeline | **C++17 Native Optimized Engine** | Drastic latency reduction, seamless multithreaded / embedded linkage |
| **Neural Backbone** | 12-Layer Base BERT (115MB) | **Distilled Mini 4L / 3L BERT** | 50% size reduction (54MB), 3x faster inference at identical accuracy |
| **Probing Structure** | Unified multi-class head | **Decoupled Sub-Heads & On-Demand** | Eliminates inter-class interference, conditional compute execution |
| **Memory Architecture**| Dynamic heap object allocation | **Pre-Allocated Buffer Pooling & Reuse** | Zero runtime heap contention, optimal CPU cache locality |
| **Phonology Matching** | Multi-pass regex (`re`) | **Single-Pass Bitmask & Static LUT** | Zero regex backtracking overhead, microsecond rule matching |
| **Path Optimization** | Recursive DFS morpheme traversal | **1D Flat DP (Viterbi Approach)** | Zero stack-overflow risk, strict linear-time path resolution |
| **Standard Coverage** | 18 major phonological rules | **Exhaustive 30 NIKL Articles Covered** | 100% compliance across 10,000-sentence Golden Baseline |
| **Operational Modes** | Single standard phonetic output | **Speech styles, vowel length, custom dict** | Tailored for AICC, chatbots, audiobooks, and navigation |

---

## 2. Core Architecture: Hybrid Pipeline & Lightweight Backbone

### 2.1. Overall Hybrid Pipeline Architecture

The SNAP v2.0 pipeline processes incoming text through dedicated, decoupled stages:

```
[Raw Input Text (UTF-8)]
          │
          ▼
┌────────────────────────────────────────────────────────┐
│  C++ Native Single-Pass LUT/Bitmask Scanner            │
│  - Primary scan for symbols, numerals, English, models │
└────────────────────────────────────────────────────────┘
          │
          ▼
┌────────────────────────────────────────────────────────┐
│  Distilled Mini BERT Backbone (3L / 4L, INT8 Quantized)│
│  - ONNX Runtime Zero-Copy Tensor Bindings              │
│  - High-dimensional hidden context via single forward  │
└────────────────────────────────────────────────────────┘
          │
          ├─► ① Counter Head   : Contextual Sino vs Native numeral classification
          ├─► ② Heteronym Head : Disambiguation across 9 major homograph clusters
          ├─► ③ Semiotic Head  : Spoken normalization of symbols & delimiter formats
          └─► ④ Morph Head     : Context-aware BIO-POS tagging & boundary detection
          │
          ▼
┌────────────────────────────────────────────────────────┐
│  C++ Deterministic Phonology Engine                    │
│  - 1D Flat DP (Viterbi) morpheme junction path search  │
│  - Full 30-Article NIKL standard pronunciation tables  │
│  - High-speed bitmask phonological feature LUTs        │
└────────────────────────────────────────────────────────┘
          │
          ▼
[Normalized Text / Standard G2P Phonemes / Prosodic Break Tags / SSML]
```

---

### 2.2. Lightweight Backbone via Knowledge Distillation (12L → 4L/3L)

To achieve low-latency throughput on commodity CPUs, SNAP v2.0 compresses the 12-Layer Base BERT teacher model into 4-Layer and 3-Layer student models via **Knowledge Distillation**:

* **Distillation Pipeline Stages**:
  1. **Logit Distillation**: Minimizes Kullback-Leibler (KL) divergence between teacher and student softmax output distributions.
  2. **Hidden State Alignment**: Maps intermediate representation vectors across transformer layers to preserve contextual representations.
  3. **Task-Specific Head Fine-Tuning**: Connects probing heads onto the distilled backbone for end-to-end task-specific optimization.

* **Model Compression Results**:
  * **12-Layer (Base Baseline)**: ~115 MB
  * **4-Layer (KD 12to4)**: ~61 MB (50% size reduction with high throughput)
  * **3-Layer (KD 12to3)**: ~54 MB (Ultra-compact footprint and ultra-low latency)

This distillation pipeline preserves contextual disambiguation precision while enabling fast CPU inference.

---

## 3. Korean Phonetic Conversion and Phonological System

Comprehensive rule specifications and conversion examples are cataloged in the [SNAP Korean v2.0 Functional Specification](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md).

The engine integrates a multi-layered linguistic pre-processing system designed for conversational AI:

1. **Standard Pronunciation Norm Baseline**:
   * Implements all 30 articles across 7 chapters of the NIKL Standard Pronunciation Rules (neutralization, nasalization, palatalization, etc.) in a native C++ table engine.
2. **Context- & POS-Driven Phonological Disambiguation**:
   * Accurately resolves complex rules that defeat naive string replacement: Article 26 tensification after Sino-Korean 'ㄹ' (*갈등[갈뜽]* vs *발달[발달]*), Article 24 tensification after verbal stems (*신고[신꼬]* vs *신고식[신고식]*), Article 27 adnominal endings, homographs (*대가, 물가*), and compound Sai-siot insertion, driven by neural Probing Head tags.
3. **Conversational Colloquialization of Loanwords and Formats**:
   * Deconstructs novel Out-of-Vocabulary (OOV) English terms (*CloudNative* [클라우드네이티브]), parses alphanumeric product codes (*iPhone 16 Pro* [아이폰 십육 프로]), and adapts everyday loanwords (*버스* [뻐스], *효과* [효꽈]).
   * Converts currencies, 100+ physical units, dates, times, fractions, phone numbers, and mathematical symbols into spoken Korean.
4. **Domain-Specific Speech Style and Prosodic Control**:
   * Offers automated mutation to spoken honorific endings (`speech_style`: `haeyo` / `hapsio`), news-grade vowel length tags (`vowel_length`), and zero-downtime hot-patching via custom dictionaries (`custom_dict`).

#### 📊 Empirical Benchmark Accuracy across Core Probing Heads

Heteronyms distinguishable by part-of-speech alone are processed via Morph Head's 158 detailed/compound BIO-POS tags without invoking specialized heads. Difficult homographs sharing identical POS categories alongside complex numerals and semiotic delimiters are handled by dedicated heads. Empirical accuracies against held-out golden datasets are summarized below:

| Head Category | Target Challenge & Scope | Evaluation Dataset Size | Measured Accuracy | Analysis & Findings |
| :--- | :--- | :---: | :---: | :--- |
| **Counter Head** | Sino vs. Native classification for 8 major counters (대, 번, 장, 기, etc.) | 6,959 instances | **`99.76%`** | Matches 12L Teacher accuracy (`99.65%`) |
| **Semiotic Head** | Contextual colloquialization for 8 symbol formats (`:`, `/`, `~`, etc.) | 2,151 instances | **`99.40%`** | Disambiguates time, dates, ratios, scores |
| **Heteronym Head** | 9 major homographs sharing identical POS tags (*대가, 시가, 잠자리*, etc.) | 1,943 instances | **`93.40%`** | Decoupled sub-heads eliminate cross-class interference |
| **Morph Head** | Contextual 158 detailed BIO-POS tagging & boundary detection | 12,432 sentences (243K chars) | **`93.98%`** | Evaluated on NIKL 2025 refined Jamo-DP holdout |

> **💡 Morph Head Error Breakdown & Practical Phonological Impact**:  
> Of the observed 6.02% error rate, **the vast majority (~4.5%) consists of phonologically neutral ambiguities (e.g., proper vs. common noun confusion) or terms deterministically resolved by loanword/proper-noun dictionaries**. Genuine verb-noun confusions (`VV↔NNG`) that could alter liaison or tensification represent only **~0.48%** of characters, and are highly likely to be corrected downstream by the native C++ 1D Flat DP phonological rule engine.

---

## 4. Quantitative Benchmarks and Comprehensive Performance Verification

> **Benchmark Environment & Prerequisites**:  
> All latency, throughput, and stress test measurements in this section were evaluated strictly in an **Intel Core i7-13700K CPU environment (ONNX Runtime CPU Execution Provider)** without GPU acceleration.

### 4.1. Empirical Latency and Throughput Benchmarks across 1,000 Sentences

Continuous inference metrics measured across a representative 1,000-sentence evaluation corpus:

* **Hardware & Runtime**: Windows 11 (x64), Intel Core i7-13700K CPU, ONNX Runtime 1.17.1 (CPU Execution Provider)
* **Dataset**: 1,000 sentences (measured continuously after 50 warm-up cycles)

| Word Count Segment | Proportion | Average Characters | Mean Latency | Median (p50) | 95th Percentile (p95) | Throughput |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **$\le$ 10 words (Ultra-Short)** | 285 (28.5%) | 32.1 chars | **`1.856 ms`** | `1.834 ms` | `2.301 ms` | **`538.8` FPS** |
| **$\le$ 20 words (Short)** | 403 (40.3%) | 64.8 chars | **`2.571 ms`** | `2.504 ms` | `3.257 ms` | **`389.0` FPS** |
| **$\le$ 30 words (Standard)** | 227 (22.7%) | 104.4 chars | **`3.322 ms`** | `3.254 ms` | `4.176 ms` | **`301.1` FPS** |
| **$\le$ 50 words (Medium-Long)** | 85 (8.5%) | 153.9 chars | **`4.334 ms`** | `4.247 ms` | `5.448 ms` | **`230.7` FPS** |
| **Overall Total** | **1,000 (100%)** | **72.1 chars** | **`2.687 ms`** | **`2.550 ms`** | **`4.338 ms`** | **`370.8` FPS** |

For typical conversational utterances (10–20 words), the engine achieves **1.8 to 2.5 ms latency per sentence**, delivering **370 to 538 FPS throughput** on standard CPUs.

---

### 4.2. Precision Comparison across Backbone Layer Depths (12L vs 4L vs 3L)

Resource consumption and accuracy across backbone layer configurations evaluated over 10,000 golden baseline sentences:

| Evaluation Metric | 12-Layer Base (Teacher) | 4-Layer (Production Default) | 3-Layer (Edge Profile) |
| :--- | :---: | :---: | :---: |
| **Model Binary Size** | 115.0 MB | 61.2 MB | **54.1 MB** |
| **Single Sentence Latency (46 chars)** | 6.20 ms | 2.15 ms | **1.99 ms** |
| **Long Sentence Latency (618 chars)** | 28.68 ms | 11.32 ms | **9.67 ms** |
| **10,000-Sentence Throughput** | ~400 FPS | ~580 FPS | **~650 FPS** |
| **Text Normalization (TN) Match Rate**| 99.62% | **99.68%** | 99.64% |
| **Final Pronunciation (G2P) Match Rate**| 100.00% (Baseline) | **98.54%** | **98.44%** |
| **Morpheme Corruption / Ill-Formed Errors**| 0.00% | **0.00%** | **0.00%** |

The 3-Layer and 4-Layer distilled backbones achieve a 3x speedup over the 12-Layer teacher baseline while keeping phonetic discrepancy within 1.5%.

* **Production Sweet Spot (Production Default: 4-Layer)**: By expending only 7.1MB of additional memory over the 3-Layer model, the 4-Layer model achieves a higher G2P pronunciation match rate (98.54%) with a negligible latency difference of just 0.16ms (2.15ms vs. 1.99ms). Consequently, it serves as the **standard recommended default for enterprise-scale cloud APIs and AICC production services**.
* **Ultra-Lightweight Edge Profile (3-Layer)**: The 3-Layer model delivers an ultra-compact 54.1MB binary size and top-tier 1.99ms speed, serving as an optimal edge profile for memory-constrained on-device, mobile embedded, and low-power IoT environments.

---

### 4.3. Stress Test Results on Ultra-Long Sentences

Stress test performance scaling across sentence concatenations ranging from 46 to 618 characters (120 words):

| Concatenated Sentences | Character Length | 12-Layer Latency | 4-Layer Latency | 3-Layer Latency | Word Accuracy (All Models) |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **1 Sentence** | 46 chars | 6.20 ms | 2.15 ms | **`1.99 ms`** | **100.0%** |
| **2 Sentences** | 85 chars | 7.08 ms | 3.14 ms | **`2.51 ms`** | **100.0%** |
| **4 Sentences** | 179 chars | 10.81 ms | 4.92 ms | **`3.79 ms`** | **100.0%** |
| **8 Sentences** | 353 chars | 15.92 ms | 7.05 ms | **`5.85 ms`** | **100.0%** |
| **16 Sentences** | 618 chars | 28.68 ms | 11.32 ms | **`9.67 ms`** | **100.0%** |

Even on extreme 618-character inputs, the 3-Layer engine completes full inference in **9.67 ms**, demonstrating robust scalability for audiobooks and continuous document narration.

---

### 4.4. Full-Coverage Golden Baseline Verification (10,000 Sentences)

Regression testing was conducted across a dedicated 10,000-sentence Golden Baseline (`snap_golden_baseline_standard_v2.json`) covering all 30 NIKL articles. Both the C++ native engine and Python implementation achieved **100.00% perfect match rate (10,000 / 10,000 sentences)**.

---

## 5. Production Integration and Real-Time Pipeline Linkage

### 5.1. REST API and Real-Time Streaming Architecture

SNAP v2.0 adopts a **real-time REST API as its standard TTS integration interface** for microservices (MSA) and cloud deployments:

* **Endpoints**:
  - Single sentence real-time pre-processing: `POST /v1/normalize`
  - High-throughput batch processing: `POST /v1/normalize/batch` (**Batching natively supported**)
* **Configurable Parameters**: Request payloads (`config`) support `speech_style`, `pronunciation_style`, `prosody_format`, `vowel_length`, and `return_ipa`.
* **Containerized Deployment (Docker)**:
  - **Open Sandbox API**: A public test API hosted on Google Cloud Run is available for immediate developer evaluation.
  - **Air-Gapped & Enterprise On-Premise**: Pre-packaged Docker container images can be deployed in secure corporate networks and air-gapped financial/public infrastructures.

* **Sample Request & Response**:
```json
{
  "text": "오늘 회의 결과 3분기 매출이 150억 원 증가했습니다.",
  "config": {
    "speech_style": "haeyo",
    "pronunciation_style": "modern_standard",
    "prosody_format": "tags",
    "return_ipa": false
  }
}
```
```json
{
  "success": true,
  "data": {
    "original_text": "오늘 회의 결과 3분기 매출이 150억 원 증가했습니다.",
    "normalized_text": "오늘 회의 결과 삼분기 매출이 백오십억 원 증가했어요.",
    "phonemes": "오늘 회이 결과 삼분기 매추리 배고시벅 원 증가해써요.",
    "ipa": null,
    "pos_tags": null
  },
  "meta": {
    "latency_ms": 2.14,
    "engine_version": "2.0.0"
  }
}
```

---

### 5.2. C/C++ Native SDK and ABI Interface

For latency-critical edge devices and on-premise deployments requiring zero network overhead, SNAP v2.0 provides direct linkage via **shared libraries (`snap_cpp.dll` / `libsnap_cpp.so`)**:

Compatible with all languages offering C ABI FFI (C++, Rust, Go, C#, Java, Swift), enabling microsecond in-process execution:

```c
#include "snap_api.h"
#include <stdio.h>

int main() {
    // 1. Initialize SNAP engine instance (weights path and language code)
    void* handle = snap_create("./models/ko", "ko");
    if (!handle) {
        fprintf(stderr, "Failed to initialize SNAP engine\n");
        return -1;
    }

    // 2. Perform text pre-processing and phonetic normalization (single call)
    const char* result_json = snap_process(handle, "3번 버스가 10분 뒤 도착합니다.");
    if (result_json) {
        // Output JSON: {"normalized_text": "...", "phonology": "...", ...}
        printf("Pre-processing Result: %s\n", result_json);

        // 3. Free returned string buffer (required)
        snap_free((void*)result_json);
    }

    // 4. Apply dynamic options (speech style mutation, etc.): snap_process_ext
    const char* ext_result = snap_process_ext(handle, "오늘 회의 결과가 나왔습니다.", 
                                              "{\"speech_style\": \"haeyo\"}");
    if (ext_result) {
        printf("Option Applied Result: %s\n", ext_result);
        snap_free((void*)ext_result);
    }

    // 5. Release engine instance resources
    snap_destroy(handle);
    return 0;
}
```

---

### 5.3. Pipeline Linkage with Modern Acoustic Models (MeloTTS, Piper, F5-TTS)

SNAP v2.0 supports target-optimized outputs across open-source and proprietary acoustic models:

```
[SNAP v2.0 Frontend]
         │
         ├─► MeloTTS Integration : Automatic morpheme tagging and prosodic pause insertion
         ├─► Piper TTS Integration : phoneme-id mapping and whitespace normalization
         └─► F5-TTS / CosyVoice : Colloquial normalization and numeral transliteration
```

---

### 5.4. Zero-Downtime Hot-Patching with Dynamic Custom Dictionary (`custom_dict`)

Corrects emerging domain terms, personal names, and acronyms in real time without server restarts or model retraining:

```json
{
  "대화형AI": "대화형 에이아이",
  "LLM": "엘엘엠",
  "AICC": "에이아이씨씨"
}
```
Atomic pointer swapping inside the C++ native engine applies dictionary updates dynamically within 1 millisecond.

---

## 6. Conclusion and Future Roadmap

**SNAP Korean v2.0** provides a high-performance speech pre-processing engine combining **strict standard phonology compliance**, **real-time execution speeds**, and **context-aware semantic disambiguation** in a single native system.

Key architectural pillars:
1. **Deterministic-Neural Hybrid Architecture**: Combines neural semantic classification with deterministic C++ phonetic transformations.
2. **Non-Autoregressive Context Probing**: Replaces autoregressive generation with an encoder forward pass.
3. **On-Demand Decoupled Sub-Heads**: Eliminates inter-class interference and redundant computations via conditional execution.
4. **Native Optimization & Integration Interfaces**: Delivers sub-3ms CPU latency alongside flexible REST APIs, Docker images, and C ABI bindings.

Future roadmap initiatives include end-to-end multilingual pipeline consolidation across Japanese and English alongside on-device NPU acceleration backends.

---

### 📚 Related Documents & Resources
* 📖 **[SNAP Korean v2.0 Functional Specification](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md)**: Comprehensive 30-article rule breakdown and unit normalization catalog
* 📋 **[SNAP v2.0 REST API Manual](SNAP_REST_API_MANUAL.md)**: Endpoint parameters, response schemas, and code recipes
* 💻 **[SNAP Native C/C++ SDK Manual](SNAP_SDK_API_MANUAL.md)**: C-API headers and linking guide
* 🎮 **[SNAP v2.0 Live Interactive Demo](https://huggingface.co/spaces/softguy777/snap_voice_demo4)**: Web evaluation sandbox for Korean v2.0
