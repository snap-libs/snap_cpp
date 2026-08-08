# 🚀 SNAP Project

> **Semantic Normalization via Attached Probes**  
> *Real-Time, Context-Aware Multilingual TTS Pre-Processing Engine*

[🌐 Official Website](https://snap-libs.github.io/snap/) | [📦 C++ SDK Setup Guide](INSTALL.md) | [📘 C++ API Manual](SNAP_API_MANUAL.md) | [🎛️ TUI Setup Manager Guide](https://github.com/snap-libs/snap_core/blob/main/setup/SNAP_SETUP_MANUAL.md)

---

## 1. Overview & Key Challenges

In Text-to-Speech (TTS) systems, the accuracy of the **front-end text normalization and grapheme-to-phoneme (G2P)** pipeline directly dictates the quality of synthesized audio. Pronunciation and normalization errors occurring at this stage propagate downstream and cannot be recovered by acoustic models or vocoders.

Legacy front-end approaches suffer from distinct structural trade-offs:

* **Regex / Rule-based Approaches**:  
  While fast and lightweight, they lack context awareness and fail to resolve linguistic ambiguities—such as heteronyms, polyphones, pitch accents, and context-dependent number/symbol readings—resulting in inaccurate phonetic representations passed to the TTS engine.
* **LLM (Large Language Model) Approaches**:  
  While highly context-aware, their massive computational cost and high latency make them impractical for real-time, on-device applications.

---

## 2. The SNAP Approach

The **SNAP Project** provides a pragmatic solution that guarantees real-time streaming performance while accurately capturing sentence-level context and semantics.

* **Small ONNX BERT + Task Probing Heads**:  
  Attaches lightweight, task-specific neural probing heads on top of a small frozen BERT backbone to resolve context-dependent phonetic ambiguities.
* **Real-Time On-Device Engine**:  
  Operates within a **~120MB memory footprint** with real-time performance (CPU avg. **~44ms/sentence** [30–170ms], GPU **11–14ms**).

* **BERT Hidden Layer Reuse (Embedded Mode)**:  
  When integrated with modern BERT-embedded TTS models (e.g., BERT-VITS2, MeloTTS), SNAP reuses pre-computed BERT hidden states from the acoustic model, reducing net additional text normalization latency to **nearly zero (~0.03ms)**.
* **C++ Native SDK**:  
  Offers an engine-agnostic C++ interface supporting Korean, Japanese, and English front-end pipelines. *(Prerequisites: Windows requires [MSVC 2019/2022 Redistributable x64](https://aka.ms/vs/17/release/vc_redist.x64.exe); Linux requires GLIBC 2.27+).*

---

## 3. Disambiguation Improvements by SNAP

Rule-based regex pipelines fail to interpret surrounding syntactic context, often misreading identical surface tokens. SNAP leverages frozen BERT representations to resolve heteronyms, part-of-speech variations, and context-dependent readings.

### 🇰🇷 Korean: Numeral System Disambiguation (Bus Route vs. Count)
* **Sentence**: *"여기서 **3번** 버스를 타고 **3번** 갈아타야 갈 수 있어."*  
  *(Take bus **#3** here and transfer **3 times**.)*
* ❌ **Legacy g2pk (Rules)**:  
  `"여기서 [세번] 버스를 타고 [세번] 가라타야..."`  
  *(Misinterprets bus route #3 as a counter, using native numeral '세번')*
* **✓ SNAP (Context-Aware)**:  
  `"여기서 [삼 번] 버스를 타고 [세 번] 가라타야..."`  
  *(Correctly assigns Sino-Korean '삼 번' for route number and native '세 번' for frequency)*

### 🇯🇵 Japanese: Kanji Heteronym Disambiguation (Date vs. Period)
* **Sentence**: *"1日は休みで、1日中雨が降った。"*  
  *(The 1st was a holiday, and it rained all day long.)*
* ❌ **Legacy MeCab / G2P**:  
  `[いちにち (ichinichi) / いちにち (ichinichi)]`  
  *(Misreads calendar date '1日' as period 'ichinichi')*
* **✓ SNAP (Context-Aware)**:  
  `[ついたち (tsuitachi) / いちにち (ichinichi)]`  
  *(Distinguishes date reading 'tsuitachi' from duration 'ichinichi')*

### 🇺🇸 English: Heteronym Part-of-Speech Disambiguation (Verb vs. Adjective)
* **Sentence**: *"I **live** near a **live** concert."*
* ❌ **Legacy g2p_en**:  
  `[/laɪv/ / /laɪv/]`  
  *(Misreads verb 'live' as adjective '/laɪv/')*
* **✓ SNAP (Context-Aware)**:  
  `[/lɪv/ (Verb) / /laɪv/ (Adjective)]`  
  *(Accurately distinguishes verb `/lɪv/` from adjective `/laɪv/` based on sentence structure)*

> **Preventing Rule Explosion**:  
> Expanding regex rules endlessly to cover edge cases leads to rule explosion and engine fragility. SNAP neural heads filter contextual structures upfront, maintaining low engine complexity.

---

## 4. TTS-Specific Option Tuning (`snap-setup`)

SNAP includes an interactive Terminal User Interface (TUI) management tool, **`snap-setup`**, allowing developers to tune language-specific text normalization and phonological options according to target TTS specifications.

![SNAP Setup Screenshot](../setup/assets/snap_setup_screen.png)

### Language-Specific Options
* **Korean**: Vowel length marking (`:`), IPA phonetic symbol conversion, Text Normalization only (`TN Only`) mode
* **Japanese**: Writing script selection (`< katakana >` / `< hiragana >` / `< romaji >`), Pitch accent contour marking, IPA phonetic symbol conversion
* **English**: IPA symbol output, Text Normalization only mode

### Environment & Asset Management
Configures `SNAP_HOME` environment variables, manages Hugging Face model downloads and verification, and syncs settings (`snap_config.json`) seamlessly with C++ and Python engines.

---

## 5. Ecosystem & Future Roadmap

### 🎙️ TTS Integrations & Video Dubbing
The SNAP C++ SDK and Python modules will continually expand integration examples across open-source and commercial TTS models, alongside automated video dubbing and multi-lingual subtitling workflows.

### 🔄 SNAP-ITN (Inverse Text Normalization) — Coming Soon
Extending the Frozen BERT Probing architecture to Speech Recognition (ASR) post-processing, **SNAP-ITN** eliminates numeric hallucination and high latency seen in LLMs, while resolving rule explosion in traditional WFSTs.

* **🇰🇷 Korean ITN Example**
  - **ASR Output**: `"오전 열 시 삼십 분에 이만 원 지불했습니다"`
  - ❌ **Legacy WFST/Rules**: Missegments numerals and particles
  - **✓ SNAP-ITN**: `"오전 10:30에 20,000원 지불했습니다"`

* **🇯🇵 Japanese ITN Example**
  - **ASR Output**: `"せんきゅうひゃくはちじゅうごねん しがつ ついたちに にまんえん はらいました"`
  - ❌ **Legacy Rules**: Misinterprets verb endings `し`(4), `まる`(0) as numbers or adds redundant thousands commas (`1,985年`)
  - **✓ SNAP-ITN**: `"1985年4月1日に20,000円払いました"`  
    *(Suppresses year commas `1985年`, preserves proper nouns like `百年戦争`, `七転八起`)*

---

## 6. Usage Code Examples

After installing the SDK, SNAP can be easily executed using the following code examples:

### C++ Native API
```cpp
#include "snap_api.h"

// Initialize SNAP engine with unified configuration
snap_handle_t handle = snap_init("models/snap_config.json");

// Perform real-time context-aware text normalization
const char* result = snap_normalize(handle, "여기서 3번 버스를 타고 3번 가라타야 해.", "ko");
```

### Python API
```python
import snap

# Initialize SNAP Engine
engine = snap.Engine(config_path="models/snap_config.json")

# Normalize text with context-aware disambiguation
result = engine.normalize("I live near a live concert.", lang="en")
print(result)
```

---

## 7. Documentation & Resources

For detailed installation, setup, and interactive demos, please refer to the official resources below:

* 🌐 **Official Website & Interactive Demo**: [https://snap-libs.github.io/snap/](https://snap-libs.github.io/snap/)
* 📦 **C++ SDK Installation Guide**: [INSTALL.md](INSTALL.md) — Comprehensive CMake build and platform setup instructions.
* 📘 **C++ API Reference Manual**: [SNAP_API_MANUAL.md](SNAP_API_MANUAL.md) — Opaque handle lifecycle, function specifications, and memory ownership rules.
* 🎛️ **TUI Setup Manager Guide**: [https://github.com/snap-libs/snap_core/blob/main/setup/SNAP_SETUP_MANUAL.md](https://github.com/snap-libs/snap_core/blob/main/setup/SNAP_SETUP_MANUAL.md) — Interactive TUI option configuration and asset management.
