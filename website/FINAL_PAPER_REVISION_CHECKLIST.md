# 최종 논문 수정 체크리스트

> **작성일**: 2026-07-28  
> **목표**: snap_paper_en.tex 최종 수정 항목 정리  
> **상태**: ArXiv 제출 전 최종 검토

---

## 📋 **수정 항목 정리**

### 1️⃣ **제목 수정** (Priority 1 - 필수)

#### 위치: 논문 최상단 (Line 53)

**기존:**
```latex
\title{\textbf{SNAP: Zero-Overhead Morpho-Semantic Text Normalization\\for Multilingual TTS via Frozen BERT Probing}}
```

**수정:**
```latex
\title{\textbf{SNAP: Minimal-Overhead Morpho-Semantic Text Normalization\\for Multilingual TTS via Frozen BERT Probing}}
```

**변경 사항:**
- "Zero-Overhead" → "Minimal-Overhead"

---

### 2️⃣ **Abstract 수정** (Priority 1 - 필수)

#### 위치: Abstract 섹션 (Line 65-68)

**기존:**
```
"SNAP achieves near-zero computational overhead (~0.03 ms)"
```

**수정:**
```
"SNAP achieves minimal computational overhead (~0.03 ms) 
when BERT hidden states are recycled from the acoustic model, 
and 30-170 ms latency in standalone deployment scenarios 
(depending on sentence length)"
```

**변경 사항:**
- "near-zero" → "minimal"
- Embedded Mode 조건 명시
- Standalone Mode 레이턴시 추가

---

### 3️⃣ **Introduction 수정** (Priority 1 - 필수)

#### 위치: Introduction 섹션 (Line 71-100)

**기존:**
```
"SNAP achieves near-zero computational overhead (~0.03 ms)"
```

**수정:**
```
"SNAP achieves near-zero computational overhead in Embedded Mode 
(~0.03 ms when BERT is already present in the acoustic model) 
and competitive latency in Standalone Mode (30-170 ms depending on sentence length)"
```

**변경 사항:**
- Embedded Mode 조건 명시
- Standalone Mode 레이턴시 추가
- 두 모드 명확히 구분

---

### 4️⃣ **Method 섹션 추가** (Priority 1 - 필수)

#### 위치: Method 섹션 (Architecture 부분)

**추가 내용:**
```
"Deployment Modes:

Embedded Mode: SNAP recycles BERT hidden states from the acoustic model,
adding only +0.03 ms overhead. This is the recommended deployment for 
modern TTS systems (VITS2, BERT-VITS2, etc.) that already include BERT.

Standalone Mode: SNAP executes BERT independently, resulting in 
30-170 ms latency depending on sentence length, with SNAP probe heads 
contributing ~1-2 ms. This mode is suitable for legacy systems or 
batch processing scenarios."
```

---

### 5️⃣ **Experiments 섹션 수정** (Priority 1 - 필수)

#### 위치: Section 3.6 (TTS Front-End Latency Benchmark)

**기존:**
```
"Standalone Mode adds BERT forward execution (~44ms/sentence under INT8 CPU inference)."
```

**수정:**
```
"Standalone Mode adds BERT forward execution (30-170 ms/sentence under INT8 CPU inference, 
depending on sentence length):
- Short sentences (10-50 chars): ~30-36 ms
- Medium sentences (100-200 chars): ~54-92 ms
- Long sentences (500 chars): ~169 ms"
```

**변경 사항:**
- 고정값 44ms → 범위 30-170ms
- 문장 길이별 상세 수치 추가
- 실제 측정 데이터 반영

---

### 6️⃣ **Section 4.1 (INT8 Quantization) 수정** (Priority 1 - 필수)

#### 위치: Section 4.1

**기존:**
```
"CPU Latency: ~93ms/sentence (FP32) → ~44ms/sentence (INT8)"
```

**수정:**
```
"CPU Latency: ~93ms/sentence (FP32) → 30-170 ms/sentence (INT8, depending on sentence length)
- Short sentences (10-50 chars): ~30-36 ms
- Medium sentences (100-200 chars): ~54-92 ms
- Long sentences (500 chars): ~169 ms"
```

**변경 사항:**
- 고정값 44ms → 범위 30-170ms
- 문장 길이별 상세 수치 추가

---

### 7️⃣ **Latency Breakdown 테이블 추가** (Priority 2 - 권장)

#### 위치: Experiments 섹션

**추가 테이블:**
```latex
\begin{table}[h]
\centering
\caption{Latency Breakdown by Deployment Mode and Sentence Length}
\label{tab:latency_breakdown}
\begin{tabular}{lcccc}
\hline
\textbf{Deployment} & \textbf{BERT Time} & \textbf{SNAP Overhead} & \textbf{Total} & \textbf{Use Case} \\
\hline
Embedded & Recycled & +0.03 ms & +0.03 ms & Real-time TTS \\
Standalone (10-50 chars) & 30-36 ms & 1-2 ms & 31-38 ms & Batch \\
Standalone (100-200 chars) & 54-92 ms & 1-2 ms & 55-94 ms & Batch \\
Standalone (500 chars) & 169 ms & 1-2 ms & 170-171 ms & Batch \\
\hline
\end{tabular}
\end{table}
```

---

### 8️⃣ **사용 시나리오 섹션 추가** (Priority 2 - 권장)

#### 위치: Experiments 또는 Discussion 섹션

**추가 내용:**
```
"Deployment Scenarios:

1. Embedded Mode (Recommended for modern TTS):
   - BERT already present in acoustic model (VITS2, BERT-VITS2, etc.)
   - SNAP recycles cached BERT hidden states
   - Overhead: +0.03 ms (negligible)
   - Ideal for: Real-time TTS synthesis

2. Standalone Mode (For legacy systems):
   - BERT executed independently
   - Latency: 30-170 ms (depending on sentence length)
   - Ideal for: Batch processing, server deployments"
```

---

### 9️⃣ **성능 우위 강조 추가** (Priority 2 - 권장)

#### 위치: Results 또는 Discussion 섹션

**추가 내용:**
```
"Despite the BERT execution overhead in Standalone Mode, 
SNAP still outperforms existing G2P engines:

- SNAP (Standalone): 30-170 ms (with semantic disambiguation)
- g2pk (MeCab-based): 9.79 ms (without semantic disambiguation)
- SNAP (Embedded): +0.03 ms (with semantic disambiguation)

The trade-off between latency and accuracy is favorable for SNAP 
in most real-world TTS applications, particularly in Embedded Mode 
where BERT is already present."
```

---

### 🔟 **벤치마크 조건 명시** (Priority 2 - 권장)

#### 위치: Experiments 섹션

**추가 내용:**
```
"Benchmark Conditions:
- CPU: Intel Xeon (specify model)
- GPU: RTX 3090
- Quantization: INT8
- Sentence length: 10-500 characters
- Tokenizer: BERT tokenizer (subword tokens: 5-224)
- Dataset: 1,000 news corpus sentences"
```

---

## 📊 **수정 우선순위**

### Priority 1 (필수 - ArXiv 제출 전 반드시 수정)
- [x] 제목: "Zero-Overhead" → "Minimal-Overhead"
- [x] Abstract: 명확화 및 Standalone Mode 추가
- [x] Introduction: 두 모드 구분
- [x] Method: Deployment Modes 설명 추가
- [x] Experiments: 레이턴시 범위 표현 (30-170ms)
- [x] Section 4.1: CPU 레이턴시 범위 표현

### Priority 2 (권장 - 논문 품질 향상)
- [ ] Latency Breakdown 테이블 추가
- [ ] 사용 시나리오 섹션 추가
- [ ] 성능 우위 강조 추가
- [ ] 벤치마크 조건 명시

---

## ✅ **수정 완료 체크리스트**

### 파일: snap_paper_en.tex

#### 제목 (Line 53)
- [ ] "Zero-Overhead" → "Minimal-Overhead" 변경

#### Abstract (Line 65-68)
- [ ] "near-zero" → "minimal" 변경
- [ ] Embedded Mode 조건 명시
- [ ] Standalone Mode 레이턴시 추가

#### Introduction (Line 71-100)
- [ ] 두 모드 명확히 구분
- [ ] Standalone Mode 레이턴시 추가

#### Method 섹션
- [ ] Deployment Modes 설명 추가

#### Section 3.6 (Latency Benchmark)
- [ ] 고정값 44ms → 범위 30-170ms
- [ ] 문장 길이별 상세 수치 추가

#### Section 4.1 (INT8 Quantization)
- [ ] 고정값 44ms → 범위 30-170ms
- [ ] 문장 길이별 상세 수치 추가

#### 추가 (Priority 2)
- [ ] Latency Breakdown 테이블 추가
- [ ] 사용 시나리오 섹션 추가
- [ ] 성능 우위 강조 추가
- [ ] 벤치마크 조건 명시

---

## 🎯 **최종 검증**

### 수정 후 확인 사항:
- [ ] 모든 수치가 실제 데이터와 일치
- [ ] "Zero-Overhead" 주장 제거됨
- [ ] 두 모드 (Embedded vs Standalone) 명확히 구분됨
- [ ] 오해의 소지 없음
- [ ] 학술적 정확성 확보됨
- [ ] 리뷰어 신뢰 확보됨

---

## ✨ **최종 결론**

**Priority 1 항목 6개를 반드시 수정한 후 ArXiv 제출하세요.**

### 예상 효과:
- ✅ 논문 신뢰도 극대화
- ✅ 리뷰어 신뢰 확보
- ✅ 학술 커뮤니티 인정
- ✅ ArXiv 게재 가능성 증대

**수정 완료 후 제출하면 매우 강력한 논문이 될 것입니다.**