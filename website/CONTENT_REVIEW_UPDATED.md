# SNAP 웹사이트 콘텐츠 검토 보고서 (최신 PDF 기반)

> **작성일**: 2026-07-27 (업데이트)  
> **검토 대상**: website/index.html 콘텐츠  
> **참고 문서**: snap_paper_en.pdf (최신 논문)

---

## 📊 검토 결과 요약 (재평가)

| 항목 | 상태 | 심각도 | 설명 |
|------|------|--------|------|
| 기본 개념 이해도 | ✅ 정확 | 낮 | 프로브와 양자화 설명 대부분 정확 |
| 핵심 가치 전달 | ⚠️ 누락 | 중 | "얕은 의미 문제"라는 핵심 통찰 여전히 미포함 |
| 기술 수치 | ⚠️ 일부 부정확 | 중 | 일부 수치는 정확하나 용도 불명 수치 존재 |
| 아키텍처 설명 | ⚠️ 누락 | 중 | Head 개수와 구조 명시 부족 |
| MeCab 표현 | ⚠️ 부분 과장 | 낮 | "완전 제거" 표현 정확화 필요 |
| SSML 표준 | ✅ 정확 | 낮 | 올바르게 서술됨 |
| 배포 정보 | ✅ 정확 | 낮 | ~120MB 언급 정확함 |

---

## 🟢 수정된 사항 (PDF 확인)

### 1. **일본어 정확도는 CORRECT** ✅

#### 웹사이트:
```
"한자 다의어 읽기 92.43% 정확도 달성"
```

#### PDF (Section 3.3.2):
```
"Final validation accuracy reached 92.43%"
```

**결론**: 웹사이트 정확함! (TECHNICAL_REPORT.md의 87.43%는 구버전)

### 2. **레이턴시 표현 부분 정확** ✅

#### 웹사이트:
```
"Latency: Real-time Sub-ms (CPU 10ms / GPU 0.23ms)"
```

#### PDF (Section 3.8):
```
Net Additional Overhead: ~0.03ms (Running 3 probe heads concurrently)
```

**상황**: 
- 추가 비용 0.03ms는 정확함
- `CPU 10ms / GPU 0.23ms`는 논문에 없음 (출처 불명)
- 하지만 Standalone 배포 시 다른 수치가 있을 수 있음 (Section 4.1에서 ~44ms/sentence)

**평가**: 과장은 아니나, 출처 설명이 필요

### 3. **배포 크기 정확** ✅

#### 웹사이트 암시:
```
"경량 신경망 프로브(Probes) 조합"
```

#### PDF (Section 4.3):
```
Total Footprint: ~120MB (4x reduction from 449MB FP32)
```

**결론**: 많은 데이터가 정확함

---

## 🔴 지정학적 오류 및 부정확성

### 1. **음자 정규화의 본질 왜곡 (여전함)**

#### 현재 웹사이트:
```
"문장의 문맥과 의미를 정밀하게 반영하면서도"
```

#### PDF의 핵심 발견 (Section 5.1):
> **"context-dependent semantic ambiguities — which conventional rule- or n-gram-based morphological analyzers cannot resolve — can be effectively resolved simply by recycling frozen BERT hidden states"**

더 정확하게는, 문맥 의존 _구조적_ 모호성이지, 깊은 의미 이해가 아님.

**개선안**:
```
"SNAP은 frozen BERT의 맥락 표현을 재활용하여,
전통적 n-gram 기반 분석기로는 해결 불가능한 
구조적 모호성을 활용하는 방식입니다."
```

---

### 2. **MeCab 제거 표현 정확화 필요**

#### 현재:
```
"기존 MeCab/Fugashi 형태소 분석기 의존성을 전수 제거"
```

#### PDF (Section 2.2):
```
"Fully replaces MeCab by predicting morphological boundaries and POS tags using BERT"
```

**핵심**: 
- MeCab 완전 제거 맞음 (Korean의 경우)
- 다만 Japanese는 뉘앙스가 다름 (Section 2.1.2):
  ```
  "perform segmentation internally without external packages like Fugashi or MeCab"
  ```

**평가**: 한국어는 정확함. 일본어 설명은 괜찮음.

---

### 3. **Head 개수 미명시 (누락)**

#### 웹사이트:
```
언급 없음 - 단순히 "프로브(Probes)"만 표현
```

#### PDF (Section 2.1):
```
KoreanHigh     Morph Head + 3 Semantic + 3 Rules = 4 Neural + 3 Rule
JapaneseMedium Morph + Yomi + Semiotic = 3 heads
EnglishLow     Heteronym + Semiotic = 2 heads
```

**개선안**: 섹션에 Head 개수 명시
```html
<li>"6개의 언어 특화 Neural Head + 규칙 모듈
  • 한국어: Morph Head + 3 Semantic Heads
  • 일본어: Morph Head + Yomi + Semiotic Head
  • 영어: Heteronym + Semiotic Head"</li>
```

---

### 4. **"양자화" 용어 모호성** ⚠️

#### 현재:
```
"양자화된 소형 BERT와 프로브"
```

#### PDF (Section 4.1):
```
"Dynamic INT8 Weight Quantization via ONNX Runtime"
- BERT 자체가 아닌 배포 시 INT8 quantization 적용
- 훈련 시에는 frozen BERT (양자화 아님)
```

**진실**:
- BERT: Pre-trained 모델, frozen (양자화 아님)
- 배포: INT8 quantization 옵션 제공

**개선안**:
```
"고정된(Frozen) 사전학습 BERT +
경량 신경망 프로브 조합
(배포 시 INT8 양자화 옵션 지원)"
```

---

## 🟡 누락된 핵심 내용

### 1. **핵심 통찰: 구조적 모호성의 단순성**

#### PDF의 발견 (Section 5.1, 5.2):
```
"SNAP decomposes complex multilingual text normalization 
into independent lightweight probes"

"multi-head execution adds virtually zero cost.
Moreover, probe independence allows individual heads to be added, 
updated, or removed without retraining remaining modules."
```

#### 웹사이트: 
- ❌ 이 설명 부재
- ❌ 왜 프로브 아키텍처가 특별한지 미설명

**개선안**:
```
"SNAP은 복잡한 다국어 정규화를 
독립적이고 가벼운 프로브들로 분해합니다.
모든 프로브가 같은 BERT 표현을 공유하므로,
새로운 Head 추가, 업데이트, 제거가
다른 모듈에 영향을 주지 않습니다."
```

---

### 2. **배포 옵션 미강조**

#### PDF (Section 4):
- Embedded Mode (VITS 내부): ~0.03ms overhead
- Standalone Mode (INT8 C++): ~120MB + 44ms/sentence
- 100% C++/Python Equivalence 검증됨

#### 웹사이트:
- ❌ Standalone 배포 옵션 언급 부재
- ❌ C++ Native 지원 미명시

---

### 3. **성능 개선 비교 부재**

#### PDF 주요 수치:
```
Korean:
- Morph: +9.3%p vs MeCab (93.5%)
- E2E CER: -5.23%p vs g2pk (11.54% vs 16.77%)
- Character Accuracy: 88.46%

Japanese:
- Morph BIO-POS: 97.84%
- Yomi: 92.43%
- Semiotic: +38.1%p vs regex (98.3%)

English:
- Heteronym: 98.79%
- Semiotic: +64.8%p vs regex (99.80%)
```

#### 웹사이트:
- ✅ 수치는 대부분 있음
- ❌ 기준점(baseline) 명확하지 않음
- ❌ "vs regex +64.8%p" 같은 context 부족

---

## ✏️ 우선순위 수정안

### Priority 1: 개념 명확화 (중대)

#### A) 음자 모호성의 본질 설명 개선
```
추가할 내용:
"SNAP은 문맥 의존적 음자 변화를 단순한 구조적 패턴 인식으로 해결합니다.
예를 들어, 3:10이 시간인지 스코어인지는 뒤따르는 조사(~에 vs ~으로)만 
구별해도 대부분 판별됩니다. 복잡한 LLM의 담론 수준 이해가 필요 없습니다."
```

#### B) Head 아키텍처 명시
```html
<card>
"Neural Probe Architecture:
- 한국어: 4개 Head (Morph + Heteronym + Counter + Semiotic)
         + 3개 Rule Module
- 일본어: 3개 Head (Morph + Yomi + Semiotic)
- 영어: 2개 Head (Heteronym + Semiotic)"
</card>
```

#### C) "양자화" 표현 정확화
```diff
- "양자화된 소형 BERT와 프로브"
+ "고정된 사전학습 BERT와 경량 신경망 프로브
  (배포 시 INT8 양자화 최적화)"
```

### Priority 2: 누락된 정보 추가 (중)

#### D) 배포 옵션 추가
```html
"Deployment Modes:
- Embedded: BERT-integrated TTS 내부에서 +0.03ms
- Standalone: 독립 C++ 엔진 (~120MB, 44ms/sentence)
- 100% Python/C++ 동치성 보증"
```

#### E) 모듈식 아키텍처 강조
```
"프로브 독립성:
모든 프로브는 Frozen BERT의 캐시된 표현을 공유하므로
각 Head를 독립적으로 추가, 업데이트, 제거 가능합니다.
다언어 확장 시 기존 Head에 영향 없음."
```

#### F) 규칙 엔진 평가
```
"한국어 규칙 엔진 검증:
- 표준 발음 규칙 준수: 100% (1,000 문장)
- 특수 케이스 처리: 98.74% (자음 길이), 100% (숫자)"
```

### Priority 3: 용어 개선 (낮음)

#### G) 레이턴시 설명 상세화
```diff
기존:
"Latency: Real-time Sub-ms (CPU 10ms / GPU 0.23ms)"

개선:
"Latency: Near-Zero Overhead
- 추가 비용: +0.03ms (모든 probe 동시 실행)
- 배포 모드별: Embedded 0.03ms / Standalone 44ms
- BERT는 이미 TTS에서 실행되므로 재사용"
```

---

## 📋 수정 체크리스트

### 정확성 관련:
- [x] 일본어 92.43% - 이미 정확함
- [x] 배포 크기 ~120MB - 이미 정확함
- [ ] "CPU 10ms / GPU 0.23ms" 출처 확인 필요
- [ ] 34,281 문장 수치 출처 확인 필요

### 설명 관련:
- [ ] 음자 모호성의 본질: "얕은 패턴 인식" 표현 추가
- [ ] Head 개수: 한국어 4+3, 일본어 3, 영어 2 명시
- [ ] 배포 옵션: Embedded vs Standalone 명시
- [ ] 모듈식 아키텍처: 프로브 독립성 설명
- [ ] 규칙 엔진: 한국어 표준 규칙 준수 언급

### 용어 관련:
- [ ] "양자화" → "Frozen BERT + INT8 배포 옵션"
- [ ] MeCab: 한국어는 "완전 제거", 일본어는 명확히
- [ ] 레이턴시: 0.03ms vs 44ms 구분 명확화

---

## 🔗 PDF 참고 섹션

**정확한 수치 출처**:
- Section 2.1: 언어별 Head 구성
- Section 3.3.2: 일본어 Yomi 92.43%
- Section 3.4: End-to-end Korean E2E 평가
- Section 4: Standalone 배포 (~120MB, 44ms)
- Section 5.1: 핵심 통찰 정리

**배포 관련**:
- Section 4.1: INT8 quantization (449MB → 113MB)
- Section 4.2: C++ Native (100% equivalence)
- Section 4.3: 최소 배포 패키지

---

## ✅ 최종 평가

**웹사이트 현재 상태**: 
- ✅ 대부분의 핵심 수치 정확
- ⚠️ 설명의 깊이와 명확성 개선 필요
- ⚠️ 왜 SNAP이 특별한지 더 잘 설명해야 함

**개선 후 예상**:
- 방문자가 SNAP의 차별성 명확히 이해 가능
- 기술적 디테일 충분히 제공
- 직관적 이해와 기술적 깊이 균형