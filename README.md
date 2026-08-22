# 🚀 SNAP Project

> **Semantic Normalization via Attached Probes**  
> *Real-Time, Context-Aware Multilingual TTS Pre-Processing Engine*

[🌐 Official Website](https://snap-libs.github.io/snap/) | [🤗 HF Live Demo](https://huggingface.co/spaces/softguy777/snap-demo) | [📦 C++ SDK Setup Guide](https://github.com/snap-libs/snap_cpp/blob/main/SNAP_SDK_INSTALL.md) | [📘 C++ API Manual](https://github.com/snap-libs/snap_cpp/blob/main/SNAP_API_MANUAL.md) | [🎛️ TUI Option Tuner](https://github.com/snap-libs/snap_cpp/blob/main/SNAP_OPTION_TUNER.md) | [✉️ Contact](mailto:snap.leejh@gmail.com)

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
  Offers an engine-agnostic C++ interface supporting Korean, Japanese, and English front-end pipelines.

---

## 3. BERT Utilization in SNAP

SNAP attaches lightweight, task-specific neural probing heads on top of a frozen BERT backbone. By leveraging deep bidirectional contextual understanding, SNAP enables downstream speech synthesis engines to achieve **human-like articulation, natural breathing rhythms, and context-accurate pronunciation** across five core linguistic tasks:

* **1. Prosodic Pause & Phrasing Prediction (운율 쉼 및 문장 분절)**
  - **Abstract Pause Tiering**: Synthesizes natural sentence phrasing by abstracting breathing intensity into 3 distinct levels (`P1`: Micro-pause for breath control [70ms], `P2`: Clause boundary break [200ms], `P3`: Sentence-final termination [500ms]), decoupling linguistic structure from engine-specific punctuation quirks.
  - **Standard SSML Output**: Attaches standard W3C `<break time="..."/>` tags directly onto G2P phonetic transcriptions for downstream TTS engines.
  - **ASR Sentence Segmentation**: Restores missing sentence boundaries and periods from unpunctuated STT transcripts by accurately identifying neural `EF` (P3) endings.

* **2. Numeral & Counter Disambiguation (수사·단위 판별)**
  - Resolves Sino-Korean vs. Native Korean numerals based on surrounding context.
  - *Examples*: `"3번 버스"` (Sino: *삼번*) vs. `"3번 반복"` (Native: *세번*), `"20대 청년"` (Sino: *이십대*) vs. `"차 20대"` (Native: *스무대*).

* **3. Heteronym & Polyphone Disambiguation (동철이음이의어 판별)**
  - **Korean**: Context-dependent tensification (e.g., `"물가 상승"` [*물까*] vs. `"강가의 물가"` [*물가*]).
  - **English**: Part-of-speech pronunciation shifts (e.g., verb `live` [/lɪv/] vs. adjective `live` [/laɪv/], past-tense `read` [/rɛd/] vs. present `read` [/riːd/]).
  - **Japanese**: Contextual Kanji On/Kun reading selection (e.g., `"1日"` [*ついたち*] vs. [*いちにち*]).

* **4. Character-level Morphological POS Probing (문자 단위 형태소 품사 태깅)**
  - Predicts fine-grained grammatical tags (`EF` sentence-final, `EC` conjunctive, `ETM` adnominal, `JKB`/`JX` particles) without heavy external POS taggers, providing robust zero-OOV generalization for out-of-vocabulary terms and neologisms.

* **5. Acoustic Model Hidden-State Export (음향 모델 특징 재사용)**
  - Exports pre-computed BERT hidden states (`[seq_len, 768]`) and `word2ph` alignments directly to BERT-embedded TTS models (MeloTTS, BERT-VITS2), eliminating redundant BERT computations and minimizing inference latency.

---

## 4. Disambiguation Improvements by SNAP

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

### 🇰🇷 Korean: Prosodic Phrasing & Punctuation Restoration (Unpunctuated Stream)
* **Sentence**: *"정부는 오늘 오전 국무회의를 열고 내년도 예산안 편성 지침을 최종 확정했습니다 날씨가 참 맑네요"*  
  *(Raw, continuous speech transcript without any commas or periods)*
* ❌ **Legacy G2P / Rules**:  
  `"정부는 오늘 오전 궁무회이를 열고 내년도 예사난 편성 지치믈 최종 확쩡핻씀니다 날씨가 참 망네요"`  
  *(Monolithic unpunctuated stream causing TTS models to rush without breathing intervals or sentence breaks)*
* **✓ SNAP (2-Pass Phrasing & Tiered Pause)**:  
  `"정부는[P1] 오늘 오전 궁무회이를 열고[P2] 내년도 예사난 편성 지치믈[P1] 최종 확쩡핻씀니다.[P3] 날씨가 참 망네요.[P3]"`  
  *(Automatically extracts micro-pauses [P1], clause breaks [P2], and restores missing sentence boundaries [P3] from neural sentence-final endings `EF`)*

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

## 5. TTS-Specific Option Tuning (`snap-setup`)

SNAP includes an interactive Terminal User Interface (TUI) management tool, **`snap-setup`**, allowing developers to tune language-specific text normalization and phonological options according to target TTS specifications.

![SNAP Setup Screenshot](assets/snap_setup_screen.png)

### Language-Specific Options
* **Korean**: Target TTS Adapter preset (`< custom >` / `< melotts >` / `< f5tts >` / `< ssml >` / `< raw >`), Vowel length marking (`:`), IPA phonetic symbol conversion, SSML tag output
* **Japanese**: Writing script selection (`< katakana >` / `< hiragana >` / `< romaji >`), Pitch accent contour marking, IPA phonetic symbol conversion, SSML tag output
* **English**: IPA symbol output, SSML tag output

### Environment & Asset Management
Configures `SNAP_HOME` environment variables, manages Hugging Face model downloads and verification, and syncs settings (`model_index.json`) seamlessly with C++ and Python engines.

---

## 6. Configuration & Global Options (`snap_config.json`)

The SNAP engine controls language-specific phonological rules, G2P modes, and output formats through `models/snap_config.json` (or dynamic runtime overrides).

### Configuration Schema
```json
{
    "device": "auto",
    "num_threads": 0,
    "ko": {
        "vowel_length": false,
        "to_ipa": false,
        "to_ssml": false
    },
    "ja": {
        "script": "katakana",
        "pitch_accent": false,
        "to_ipa": false,
        "to_ssml": false
    },
    "en": {
        "to_ipa": false,
        "to_ssml": false
    }
}
```

### Key Option Definitions
* **`device`** (`string`, default: `"auto"`): Target hardware Execution Provider (`"auto"`, `"cuda"`, `"directml"`, `"coreml"`, `"openvino"`, or `"cpu"`). In `"auto"` mode, SNAP automatically detects NVIDIA GPU (CUDA), Apple Silicon (CoreML), or Windows DirectX (DirectML) with zero inference latency overhead and gracefully falls back to CPU if unavailable.
* **`num_threads`** (`int`, default: `0`): Number of intra-op CPU threads (`0` = automatic core detection).
* **`to_ssml`** (`bool`, default: `false`): Enables SSML output tags (`<break strength="..."/>` for Korean pauses, `<phoneme alphabet="ipa" ph="rɛd">read</phoneme>` for English). When set to `false`, SNAP outputs direct phonetic text.
* **`to_ipa`** (`bool`, default: `false`): Enables International Phonetic Alphabet (IPA) output string transcription (e.g. `[aɪ rɛd ðə bʊk...]`).
* **`vowel_length`** (`bool`, default: `false`): Korean vowel length notation (e.g., `밤:` vs `밤`).
* **`script`** (`string`, default: `"katakana"`): Japanese writing script output (`"katakana"`, `"hiragana"`, or `"romaji"`).
* **`pitch_accent`** (`bool`, default: `false`): Japanese pitch accent contour marking (`^` high pitch rise, `]` pitch drop).

---

## 7. Ecosystem & Future Roadmap

### 🎙️ TTS Integrations & Video Dubbing
The SNAP C++ SDK and Python modules continually expand integration examples across open-source and commercial TTS models, alongside automated video dubbing and multi-lingual subtitling workflows.

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

## 8. Usage Code Examples

After installing the SDK, SNAP can be easily executed using the following code examples:

### C++ Native API
```cpp
#include <iostream>
#include "snap/snap_api.h"

int main() {
    // 1. Initialize SNAP engine (uses SNAP_HOME environment variable if NULL)
    void* handle = snap_create(NULL, "ko");
    if (!handle) return 1;

    // 2. Run inference with SSML break tags ("to_json": false returns plain text)
    const char* text = "정부는 오늘 오전 국무회의를 열고 예산안을 확정했습니다.";
    const char* result = snap_process_ext(handle, text, "{\"to_ssml\": true, \"to_json\": false}");

    if (result) {
        std::cout << "Phonology: " << result << std::endl;
        // Output: "<speak>정부는 <break strength=\"weak\"/> 오늘 오전 궁무회이를 열고 <break strength=\"medium\"/> 내년도 예사난 편성 지치믈 <break strength=\"weak\"/> 최종 확쩡핻씀니다. <break strength=\"strong\"/></speak>"
        snap_free((void*)result);
    }

    // 3. Destroy engine handle
    snap_destroy(handle);
    return 0;
}
```

### Python API
```python
from snap import ContextClassifier

# 1. Initialize SNAP Engine for Korean
clf = ContextClassifier("ko", weights_dir="models")

# 2. Process text with SSML break tags
text = "정부는 오늘 오전 국무회의를 열고 예산안을 확정했습니다."
result = clf.process(text, to_ssml=True)

print("Normalized:", result["normalized_text"])
print("Phonology :", result["phonology"])
# 👉 "<speak>정부는 <break strength=\"weak\"/> 오늘 오전 궁무회이를 열고 <break strength=\"medium\"/> 내년도 예사난 편성 지치믈 <break strength=\"weak\"/> 최종 확쩡핻씀니다. <break strength=\"strong\"/></speak>"
```

---

## 9. Documentation & Resources

For detailed installation, setup, and interactive demos, please refer to the official resources below:

* 🌐 **Official Website & Interactive Demo**: [https://snap-libs.github.io/snap/](https://snap-libs.github.io/snap/)
* 📦 **C++ SDK Installation Guide**: [https://github.com/snap-libs/snap_cpp/blob/main/SNAP_SDK_INSTALL.md](https://github.com/snap-libs/snap_cpp/blob/main/SNAP_SDK_INSTALL.md) — Comprehensive CMake build and platform setup instructions.
* 📘 **C++ API Reference Manual**: [https://github.com/snap-libs/snap_cpp/blob/main/SNAP_API_MANUAL.md](https://github.com/snap-libs/snap_cpp/blob/main/SNAP_API_MANUAL.md) — Opaque handle lifecycle, function specifications, and memory ownership rules.
* 🎛️ **TUI Option Tuner Guide (Optional)**: [https://github.com/snap-libs/snap_cpp/blob/main/SNAP_OPTION_TUNER.md](https://github.com/snap-libs/snap_cpp/blob/main/SNAP_OPTION_TUNER.md) — Interactive TUI option configuration and asset management.

---

## 10. Community & Contact

We welcome questions, feedback, bug reports, and collaboration inquiries!

* ✉️ **Official Email**: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)
* 🐛 **Bug Reports & Issues**: [https://github.com/snap-libs/snap_cpp/issues](https://github.com/snap-libs/snap_cpp/issues)
* 💬 **GitHub Discussions**: [https://github.com/snap-libs/snap_cpp/discussions](https://github.com/snap-libs/snap_cpp/discussions)
* 🤗 **Hugging Face Hub**: [https://huggingface.co/softguy777/snap-weights](https://huggingface.co/softguy777/snap-weights)

---

## 11. License & Dual Licensing

SNAP C++ SDK is licensed under a **Dual License** model:

1. **Open Source & Research (GNU AGPLv3)**:
   - Free for non-commercial, open-source projects, academic use, and research.
   - Any software, SaaS platform, cloud API, or service built with or embedding SNAP must make its complete source code available under the **GNU Affero General Public License v3 (AGPL-3.0)**.
2. **Commercial License**:
   - For closed-source, proprietary commercial products, on-premise enterprise deployments, or cloud TTS services that do not wish to disclose source code.
   - For commercial licensing inquiries and custom terms, please contact: [snap.leejh@gmail.com](mailto:snap.leejh@gmail.com)
