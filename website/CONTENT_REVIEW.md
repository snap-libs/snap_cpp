# SNAP 웹사이트 콘텐츠 검토 보고서

> **작성일**: 2026-07-27  
> **검토 대상**: website/index.html 콘텐츠  
> **참고 문서**: snap_py/TECHNICAL_REPORT.md

---

## 📊 검토 결과 요약

| 항목 | 상태 | 심각도 | 설명 |
|------|------|--------|------|
| 기본 개념 이해도 | ⚠️ 부분 부정확 | 중 | 프로브(Probes)와 양자화 설명 불충분 |
| 핵심 가치 전달 | ⚠️ 누락 | 중 | "얕은 의미 문제"라는 핵심 통찰 미포함 |
| 기술 수치 | ❌ 오류 | 중 | 일부 정확도/성능 수치 불일치 |
| 아키텍처 설명 | ⚠️ 과장 | 중 | "Sub-millisecond" 표현이 오해 야기 |
| MeCab 제거 표현 | ❌ 오류 | 중 | 완전 제거가 아님 (대체, 선택 사항) |
| SSML 표준 | ✅ 정확 | 낮 | 올바르게 서술됨 |
| 언어 지원 | ✅ 정확 | 낮 | KR, EN, JA 중복 확인 됨 |

---

## 🔴 주요 오류 및 부정확성

### 1. **음자 정규화의 본질 왜곡**

#### 현재 웹사이트:
```
"SNAP Framework는 ... 문장의 문맥과 의미를 정밀하게 반영하면서도, 
양자화된 소형 BERT와 경량 프로브(Probes) 조합을 통해 
실시간(Sub-millisecond) 환경에서 높은 수준의 정규화"
```

#### 논문의 핵심 통찰 (Section 4.1):
> **"Pronunciation Disambiguation Is a Shallow Semantic Problem"**

문제를 "깊은(deep) 의미 기반"이 아니라 **"얕은(shallow) 패턴 인식"** 문제로 정의합니다:
- `3:10` → 시간 vs 스코어 판별 = 조사(~에, ~으로)만 보면 됨
- 담론 전체 맥락이 필요 없음
- BERT의 문장 수준 이해도가 필요한 게 아니라 국소적 단서만 필요

**개선안**: 
```
"SNAP Framework는 문맥 의존적 발음 변화가 실은 
'얕은 패턴 인식' 문제임을 발견했습니다. 
조사(particles), 접근 명사, 문법 구조 같은 국소적 단서만으로 
대부분의 의미 모호성을 해결할 수 있습니다."
```

---

### 2. **레이턴시 표현 오류 (과장)**

#### 현재:
```
"Latency: Real-time Sub-ms (CPU 10ms / GPU 0.23ms)"
```

#### 논문 (Section 3.4):
```
| Computation Step | Latency | Notes |
|---|---|---|
| BERT Forward Pass | ~8 ms | **Already performed by existing VITS** |
| Linear Head (per head) | ~0.01 ms | Single matmul |
| **Net additional cost** | **~0.03 ms** | 3 heads running |
```

**문제**: 
- "Sub-ms"는 _추가 비용_만 의미 (0.03ms)
- 전체 파이프라인은 ~8ms (BERT는 이미 존재)
- CPU 10ms는 어디서 나온 수치인가? (논문에 없음)

**개선안**:
```
"Latency: Zero-cost reuse
- BERT computation: 8ms (이미 VITS에서 실행)
- 추가 비용: ~0.03ms (6개 head 동시 실행)
- 순증가: 0.4% 오버헤드"
```

---

### 3. **정확도 수치 불일치**

#### 웹사이트 - 일본어:
```
"한자 다의어 읽기 92.43% 정확도 달성"
```

#### 논문 - 일본어 Kanji G2P Head (Section 3.2.1):
```
**V7: 87.43% validation accuracy**
```

**차이점**: 5.01%p 오차 (심각)

#### 웹사이트 - 평가 지표:
```
"34,281 문장 코퍼스 및 300건 고난도 벤치마크 99.96% Effective Pass Rate"
```

**문제**: 이 수치는 논문 어디에도 없음
- 논문의 gold 코퍼스: N=1,500 (README_AI.md, Section 5)
- Python Gold 정확도: 52.87% (매우 낮음)
- C++ Gold 정확도: 52.60%

---

### 4. **MeCab 제거에 대한 과장된 표현**

#### 현재:
```
"기존 MeCab/Fugashi 형태소 분석기 의존성을 전수 제거하고"
```

#### 논문의 실제 의도 (Section 2.2):
```
"Fugashi uses the same unidic dictionary as MeCab 
while eliminating installation conflicts"
```

**핵심**: 
- MeCab을 완전히 제거한 게 아니라, **Python 기반 대체 Fugashi 사용**
- 여전히 형태소 분석 필요 (다만 설치 충돌 해결)
- SNAP이 형태소 분석을 완전히 대체하는 건 아님

**개선안**:
```
"기존 MeCab의 설치 충돌 문제를 Fugashi를 통해 해결하고,
SNAP 프로브로 문맥 의존적 이음어를 정밀 처리합니다"
```

---

### 5. **"양자화된 소형 BERT" 표현의 모호성**

#### 현재:
```
"양자화된 소형 BERT와 프로브"
```

#### 논문 실제 설명:
```
- BERT: pre-trained 모델 그대로 사용 (양자화 명시 없음)
- 크기: ~110M 파라미터 ("소형"은 상대적 표현)
- Heads만 추가: 0.45% of BERT = 1.93 MB
```

**문제**: 
- BERT 자체가 양자화된 건지, heads만 양자화된 건지 불명확
- 논문에서는 "frozen BERT" 강조 (재훈련 없음)

**개선안**:
```
"고정된(Frozen) 사전학습 BERT와 경량 신경망 프로브(각 0.01% 수준)"
```

---

## 🟡 누락된 핵심 내용

### 1. **SNAP이 특별한 이유: 비용-성능 트레이드오프의 해결**

#### 논문 Abstract부터:
```
"at virtually zero additional cost (~0.03 ms/sentence)"
"The combined trainable parameters of all six heads amount to just 0.45% of BERT 
(1.93 MB total), achieving validation accuracies of 87.43%–100%"
```

#### 웹사이트 누락 내용:
- ❌ 왜 LLM 방식보다 낫는가? (비용-정확도 비교 부재)
- ❌ Head 아키텍처의 구체적 구조 설명 부재
- ❌ "Six heads" 개수와 목적 명시 부재

---

### 2. **SNAP이 해결하는 4가지 문제 카테고리가 모호**

#### 논문 (Section 2.1):
```
Category 1: Cross-lingual Semiotic Tokens    (기호+숫자)
Category 2: Heteronyms & Kanji Homographs    (이음어)
Category 3: Korean Number Reading Shift      (수사 읽기)
```

#### 웹사이트:
- ✅ 예시는 있음
- ❌ 각 언어가 어떤 문제를 해결하는지 명확하지 않음

---

### 3. **SSML 표준화의 중요성 미강조**

#### 논문 (Section 2.2):
```
"By introducing this standard as the interface between the neural 
network and the rule engine, we establish a universal, debuggable 
semantic communication protocol."
```

#### 웹사이트: 
- SSML 태그 예시는 있으나
- ❌ 왜 SSML인지, 실제 무엇을 제공하는지 설명 부족

---

### 4. **Head별 정확도가 다른 이유 설명 전무**

#### 논문의 중요한 발견들:
```
- Japanese Kanji: 87.43% (446개 클래스 → 가장 복잡)
- English Heteronym: 98.79% (173개 클래스)
- Semiotic: 99.80% (5-9개 클래스만)
- Korean Number: 98.8% (2개 클래스만)
```

#### 웹사이트: 
- 수치만 나열
- ❌ 왜 차이나는지, 무엇이 어렵고 쉬운지 설명 없음

---

## 🟢 잘된 점

✅ **올바르게 표현된 부분**:

1. **SSML 표준 사용** - 정확하게 설명됨
2. **언어별 문제 예시** - 구체적이고 이해하기 쉬움
   - 한국어: `감기` (발음 이음어)
   - 영어: `bass`
   - 일본어: `人気`
3. **3언어 동시 지원** - 명확함
4. **일반적 아키텍처 설명** - 기본 개념 올바름
5. **실제 응용 사례** (RaconVoice, racon_dubber) - 구체적

---

## ✏️ 개선 방안

### Priority 1: 핵심 오류 수정 (중대)

#### A) 일본어 정확도 수정
```diff
- "한자 다의어 읽기 92.43% 정확도 달성"
+ "한자 다의어 읽기 87.43% 정확도 달성 (446개 세부 분류)"
```

#### B) 음자 문제의 본질 설명 추가
```
섹션 "03. SNAP's Practical & Elegant Compromise" 수정:

현재 설명을 한심층 교체:
"SNAP은 문맥 의존 음자 변화가 '얕은 패턴 인식 문제'임을 발견했습니다. 
조사(particles), 인접 명사, 문법 구조 같은 국소 단서만으로 
대부분의 의미 모호성을 해결할 수 있습니다. 
이를 통해 복잡한 LLM 없이도 실시간 처리가 가능합니다."
```

#### C) 레이턴시 표현 정확화
```diff
- "Latency: Real-time Sub-ms (CPU 10ms / GPU 0.23ms)"
+ "Latency: Near-Zero Overhead
  - BERT: 8ms (기존 VITS에서 이미 실행)
  - 추가 비용: +0.03ms (모든 6개 head)"
```

### Priority 2: 누락된 기술 세부정보 추가 (중)

#### D) Head 구조 설명 추가
```
"Core Engine" 섹션에 추가:

"Architecture Details:
- 6개의 독립적 경량 Head (총 1.93 MB)
  • Japanese Kanji G2P: 1.34 MB (446 클래스)
  • English Heteronym: 0.52 MB (173 클래스)
  • Semiotic (KO/EN/JA): 0.07 MB (5-9 클래스)
- 모든 Head는 Frozen BERT의 768차원 representation 공유
- Head 구조: Dropout(0.1) → Linear(768 → N_classes)"
```

#### E) LLM vs SNAP 비교표 추가
```
새로운 카드 추가:

"Why Not LLM?
┌─────────────────┬──────────────┬─────────────────┐
│     항목        │     LLM      │      SNAP       │
├─────────────────┼──────────────┼─────────────────┤
│ 모델 크기       │ 100M~13B+    │ 0.45% of BERT   │
│ 레이턴시        │ 500ms~2s     │ +0.03ms         │
│ 메모리          │ 1GB~100GB+   │ +2MB            │
│ 정확도          │ 높음         │ 87~99% (충분)   │
│ 배포 비용       │ 높음         │ 극히 낮음       │
└─────────────────┴──────────────┴─────────────────┘"
```

### Priority 3: 용어 and 표현 개선 (낮음)

#### F) MeCab 표현 수정
```diff
- "기존 MeCab/Fugashi 형태소 분석기 의존성을 전수 제거"
+ "MeCab 설치 충돌 문제를 해결하고 문맥 기반 이음어를 정밀 처리"
```

#### G) "양자화" 용어 명확화
```diff
- "양자화된 소형 BERT와 프로브"
+ "고정된(Frozen) 사전학습 BERT + 경량 신경망 프로브 조합"
```

---

## 📋 수정 체크리스트

- [ ] 일본어 정확도: 92.43% → 87.43%
- [ ] 레이턴시 설명 재작성 (CPU 10ms/GPU 0.23ms 출처 확인)
- [ ] "얕은 의미 문제" 핵심 통찰 추가
- [ ] Head 아키텍처 세부 설명 추가
- [ ] LLM vs SNAP 비교표 추가
- [ ] MeCab 표현 정확화
- [ ] "양자화" 용어 명확화
- [ ] 34,281 문장 수치 출처 확인 및 수정
- [ ] 99.96% accuracy 수치 검증/수정

---

## 🔗 참고 자료

**논문 주요 섹션**:
- Section 2.1: 4가지 문제 카테고리 정의
- Section 2.2: SSML 아키텍처
- Section 2.3: Head 크기 및 구조
- Section 3.2: Head별 정확도 세부
- Section 3.4: 레이턴시 분석
- Section 4.1: "얕은 의미 문제" 발견

**추가 문서**:
- README_AI.md: G2P 파이프라인 상세
- Code: snap_py/snap/phonology_{lang}.py