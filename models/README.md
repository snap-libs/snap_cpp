---
language:
- ko
- ja
- en
license: apache-2.0
tags:
- text-normalization
- g2p
- tts
- bert
- zero-overhead
- int8
- onnx
metrics:
- cer
- accuracy
---

# SNAP Multilingual G2P & Text Normalization Models

High-performance, zero-dependency C/C++ & Python inference engine models for Multilingual Text Normalization (ITN/TN) and G2P (Grapheme-to-Phoneme) conversion.

## 📦 Repository Layout

```
snap-models/
├── manifest.json                             # Root version & variant controller
├── README.md                                 # Model card documentation
│
├── ko/                                       # Korean Models & Lexicons
│   ├── dictionaries/v1.0.0/                  # Independent Lexicon Versioning
│   └── model_variants/kcbert-base-int8/v1.0.0/ # Backbone Model & Probe Heads
│
├── ja/                                       # Japanese Models & Lexicons
│   ├── dictionaries/v1.0.0/
│   └── model_variants/ja-kanji-bert-int8/v1.0.0/
│
└── en/                                       # English Models & Lexicons
    ├── dictionaries/v1.0.0/
    └── model_variants/en-bert-base-int8/v1.0.0/
```

## 🚀 Quick Usage (Python)

```python
from snap import PhonologyKR

# Engine automatically parses manifest.json and loads active_version
frontend = PhonologyKR(models_dir="./models")
result = frontend.normalize("2024년 5월 28일 오후 3시에 만납시다.")
print(result["phonology"])
# Output: "이천이십사년 오월 이십팔일 오후 세시에 만납씨다."
```

## 📜 License

Apache-2.0 License.
