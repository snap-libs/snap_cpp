# SNAP 웹사이트 메시징 명확성 평가

> **작성일**: 2026-07-28  
> **평가 목표**: 방문자가 "SNAP = 3개국어 정규화/역정규화 프로젝트"를 명확히 인식하는가?  
> **평가 기준**: 메시징 명확성, 가시성, 이해도, 기억성

---

## 📊 **종합 평가: 7.5/10** — 양호하나 개선 필요

| 항목 | 평가 | 점수 | 설명 |
|------|------|------|------|
| **핵심 메시지 명확성** | ⭐⭐⭐⭐ | 8/10 | 명확하나 강조 부족 |
| **3개국어 지원 가시성** | ⭐⭐⭐ | 7/10 | 언어 토글 있으나 눈에 띄지 않음 |
| **정규화/역정규화 구분** | ⭐⭐⭐ | 6/10 | 설명 있으나 시각적 구분 약함 |
| **첫 인상 (Hero Section)** | ⭐⭐⭐⭐ | 8/10 | 제목과 부제 명확 |
| **정보 계층** | ⭐⭐⭐⭐ | 8/10 | 논리적 흐름 우수 |
| **시각적 강조** | ⭐⭐⭐ | 6/10 | 핵심 정보 강조 약함 |
| **기억성** | ⭐⭐⭐ | 7/10 | 기술 용어 많아 이해 어려움 |
| **콜 투 액션** | ⭐⭐⭐ | 6/10 | 다음 단계 명확하지 않음 |

**종합 점수: 7.5/10** — 기술적으로 정확하나 메시징 강도 약함

---

## 🟢 **잘된 점**

### 1. **첫 인상 (Hero Section) (8/10)** ⭐⭐⭐⭐

#### 강점:
```
✅ 제목 명확
   - "SNAP" (큰 글씨, 눈에 띔)
   - 부제: "Semantic Normalization via Attached Probes"
   - 한국어: "고정된 사전학습 BERT 위에 경량 신경망 프로브..."

✅ 설명 구체적
   - 3개국어 지원 명시
   - 문맥 의존적 발음 모호성 해결 명시
   - 실시간 처리 강조

✅ 레이아웃
   - 왼쪽 고정 헤더에 배치
   - 스크롤해도 항상 보임
   - 시각적 우선순위 높음
```

---

### 2. **정보 계층 (8/10)** ⭐⭐⭐⭐

#### 강점:
```
✅ 논리적 흐름
   01. 텍스트 정규화 (TN) 개요
   02. Frozen BERT Probing 해결책
   03. 실시간 인퍼런스 & 레이턴시
   04. SNAP 기반 기술 (Head 아키텍처)
   05. 역정규화 (ITN) 개요
   06. ITN 예시
   07. 응용 및 데모
   08. 문서 및 저장소

✅ 각 섹션 명확
   - 문제 → 해결책 → 성능 → 기술 → 응용
   - 자연스러운 흐름
```

---

### 3. **언어 지원 표시 (7/10)** ⭐⭐⭐

#### 강점:
```
✅ 3언어 토글 명시
   - EN / KO / JA 버튼
   - 왼쪽 상단에 배치
   - 활성 상태 강조색으로 표시

✅ 모든 콘텐츠 3언어 지원
   - 제목, 설명, 예시 모두 번역됨
   - 일관성 있음
```

---

## 🟡 **개선 필요 사항**

### 1. **3개국어 지원 가시성 부족 (7/10)** ⚠️

#### 문제점:
```
❌ 언어 토글 버튼이 너무 작음
   - 폰트 크기: 0.875rem (14px)
   - 색상: slate-500 (회색, 눈에 띄지 않음)
   - 위치: 왼쪽 상단 (스크롤 필요)

❌ "3개국어 지원"이 명시적이지 않음
   - 부제에 언어 명시 없음
   - 첫 인상에서 다국어 강조 약함

❌ 모바일에서 언어 토글 접근성 낮음
   - 버튼 크기 작음
   - 위치 불편함
```

#### 개선안:
```html
<!-- 1. 언어 토글 강조 -->
<div class="lang-toggle mb-8">
  <span class="text-xs font-bold uppercase tracking-widest text-slate-400 mr-3">
    Language:
  </span>
  <button id="btn-en" class="px-3 py-1 rounded hover:bg-slate-800 
                             active:bg-accent active:text-onyx">
    EN
  </button>
  <span>/</span>
  <button id="btn-ko" class="px-3 py-1 rounded hover:bg-slate-800 
                             active:bg-accent active:text-onyx">
    KO
  </button>
  <span>/</span>
  <button id="btn-ja" class="px-3 py-1 rounded hover:bg-slate-800 
                             active:bg-accent active:text-onyx">
    JA
  </button>
</div>

<!-- 2. 부제에 언어 명시 -->
<p class="mt-4 max-w-xs leading-normal text-sm">
  <span data-en="Multilingual framework (Korean, English, Japanese) attaching lightweight neural probes..."
        data-ko="다국어 프레임워크(한국어·영어·일본어) 지원..."
        data-ja="多言語フレームワーク（韓国語・英語・日本語）対応...">
  </span>
</p>

<!-- 3. 언어별 배지 추가 -->
<div class="flex gap-2 mt-4">
  <span class="px-2 py-1 text-xs rounded-full bg-slate-900/50 border border-slate-800">
    🇰🇷 Korean
  </span>
  <span class="px-2 py-1 text-xs rounded-full bg-slate-900/50 border border-slate-800">
    🇺🇸 English
  </span>
  <span class="px-2 py-1 text-xs rounded-full bg-slate-900/50 border border-slate-800">
    🇯🇵 Japanese
  </span>
</div>
```

---

### 2. **정규화/역정규화 구분 약함 (6/10)** ⚠️

#### 문제점:
```
❌ TN과 ITN이 분리되어 있음
   - 섹션 01-04: TN (정규화)
   - 섹션 05-06: ITN (역정규화)
   - 관계성 불명확

❌ 시각적 구분 없음
   - 같은 스타일로 표현
   - 두 개념의 차이 강조 부족

❌ "정규화/역정규화"라는 개념 설명 부족
   - 기술 용어만 사용
   - 일반인 이해도 낮음
```

#### 개선안:
```html
<!-- 1. 개념 설명 추가 -->
<div class="mt-8 p-4 rounded-lg border border-slate-800 bg-slate-900/30">
  <h3 class="text-sm font-bold text-slate-200 mb-2">
    정규화 (Normalization) vs 역정규화 (Inverse Normalization)
  </h3>
  <div class="grid grid-cols-2 gap-4 text-xs text-slate-400">
    <div>
      <p class="font-semibold text-slate-300 mb-1">정규화 (TN)</p>
      <p>텍스트 → 발음 기호</p>
      <p class="text-slate-500 mt-1">예: "3번" → "삼 번" 또는 "세 번"</p>
    </div>
    <div>
      <p class="font-semibold text-slate-300 mb-1">역정규화 (ITN)</p>
      <p>발음 기호 → 텍스트</p>
      <p class="text-slate-500 mt-1">예: "삼 번" → "3번"</p>
    </div>
  </div>
</div>

<!-- 2. 섹션 헤더 강조 -->
<section class="mt-16 pt-8 border-t-2 border-accent">
  <h2 class="text-lg font-bold text-accent mb-4">
    Part 2: 역정규화 (Inverse Normalization)
  </h2>
  <!-- 콘텐츠 -->
</section>
```

---

### 3. **시각적 강조 부족 (6/10)** ⚠️

#### 문제점:
```
❌ 핵심 메시지 강조 약함
   - "3개국어" 강조 없음
   - "정규화/역정규화" 강조 없음
   - "실시간" 강조 없음

❌ 아이콘/배지 부재
   - 텍스트만 사용
   - 시각적 다양성 부족

❌ 색상 활용 미흡
   - 모든 텍스트가 회색
   - 강조색 (파란색) 거의 사용 안 함
```

#### 개선안:
```html
<!-- 1. 핵심 메시지 강조 -->
<div class="mt-6 space-y-3">
  <div class="flex items-start gap-3">
    <span class="text-accent font-bold">✓</span>
    <p class="text-sm text-slate-300">
      <span class="text-accent font-semibold">3개국어 지원</span>
      (한국어, 영어, 일본어)
    </p>
  </div>
  <div class="flex items-start gap-3">
    <span class="text-accent font-bold">✓</span>
    <p class="text-sm text-slate-300">
      <span class="text-accent font-semibold">정규화 & 역정규화</span>
      (양방향 변환)
    </p>
  </div>
  <div class="flex items-start gap-3">
    <span class="text-accent font-bold">✓</span>
    <p class="text-sm text-slate-300">
      <span class="text-accent font-semibold">실시간 처리</span>
      (10-44ms, 문장 길이에 따라 변동)
    </p>
  </div>
</div>

<!-- 2. 배지 추가 -->
<div class="flex flex-wrap gap-2 mt-6">
  <span class="px-3 py-1 text-xs font-semibold rounded-full 
               bg-accent/10 border border-accent text-accent">
    Multilingual
  </span>
  <span class="px-3 py-1 text-xs font-semibold rounded-full 
               bg-accent/10 border border-accent text-accent">
    Real-time
  </span>
  <span class="px-3 py-1 text-xs font-semibold rounded-full 
               bg-accent/10 border border-accent text-accent">
    Context-Aware
  </span>
</div>
```

---

### 4. **기억성 부족 (7/10)** ⚠️

#### 문제점:
```
❌ 기술 용어 많음
   - "Frozen BERT Probing"
   - "Semantic Normalization via Attached Probes"
   - "Context-dependent phonetic ambiguities"
   - 일반인 이해 어려움

❌ 핵심 메시지 반복 없음
   - 한 번만 설명
   - 기억에 남지 않음

❌ 시각적 요약 없음
   - 다이어그램 부재
   - 인포그래픽 부재
```

#### 개선안:
```html
<!-- 1. 간단한 설명 추가 -->
<div class="mt-8 p-4 rounded-lg bg-slate-900/50 border border-slate-800">
  <h3 class="text-sm font-bold text-slate-200 mb-3">
    SNAP이란?
  </h3>
  <p class="text-sm text-slate-400 leading-relaxed">
    SNAP은 <span class="text-accent font-semibold">3개국어(한국어, 영어, 일본어)의 
    텍스트를 정확한 발음으로 변환</span>하는 AI 프레임워크입니다. 
    <span class="text-accent font-semibold">정규화(텍스트→발음)</span>와 
    <span class="text-accent font-semibold">역정규화(발음→텍스트)</span> 
    양방향을 모두 지원하며, 
    <span class="text-accent font-semibold">실시간 처리</span>가 가능합니다.
  </p>
</div>

<!-- 2. 핵심 메시지 반복 -->
<section class="mt-16 p-6 rounded-lg border border-accent/30 bg-accent/5">
  <h2 class="text-base font-bold text-accent mb-4">
    핵심 요약
  </h2>
  <ul class="space-y-2 text-sm text-slate-300">
    <li>✓ <span class="font-semibold">3개국어</span> 동시 지원</li>
    <li>✓ <span class="font-semibold">정규화 & 역정규화</span> 양방향</li>
    <li>✓ <span class="font-semibold">실시간 처리</span> (10-44ms)</li>
    <li>✓ <span class="font-semibold">높은 정확도</span> (87-99%)</li>
  </ul>
</section>
```

---

### 5. **콜 투 액션 부족 (6/10)** ⚠️

#### 문제점:
```
❌ 다음 단계 명확하지 않음
   - "어디서 시작해야 하나?"
   - "어떻게 사용하나?"
   - "더 알아보려면?"

❌ 버튼/링크 부족
   - 문서 링크 있으나 눈에 띄지 않음
   - 데모 링크 명확하지 않음

❌ 행동 유도 약함
   - 방문자가 다음 단계 모름
```

#### 개선안:
```html
<!-- 1. 명확한 CTA 버튼 -->
<div class="mt-8 flex flex-wrap gap-3">
  <a href="#applications" class="px-4 py-2 rounded-lg 
                                bg-accent text-onyx font-semibold
                                hover:bg-accent/90 transition-colors">
    데모 보기
  </a>
  <a href="#documentation" class="px-4 py-2 rounded-lg 
                                 border border-accent text-accent
                                 hover:bg-accent/10 transition-colors">
    문서 읽기
  </a>
  <a href="https://github.com/..." class="px-4 py-2 rounded-lg 
                                         border border-slate-700 text-slate-300
                                         hover:border-slate-600 transition-colors">
    GitHub 보기
  </a>
</div>

<!-- 2. 섹션별 CTA -->
<section class="mt-12 p-6 rounded-lg bg-slate-900/50 border border-slate-800">
  <h3 class="text-sm font-bold text-slate-200 mb-3">
    다음 단계
  </h3>
  <ul class="space-y-2 text-sm text-slate-400">
    <li>→ <a href="#applications" class="text-accent hover:underline">
      응용 예시 및 데모 보기
    </a></li>
    <li>→ <a href="#documentation" class="text-accent hover:underline">
      기술 문서 읽기
    </a></li>
    <li>→ <a href="https://github.com/..." class="text-accent hover:underline">
      GitHub 저장소 방문
    </a></li>
  </ul>
</section>
```

---

## 📋 **메시징 명확성 개선 체크리스트**

### 핵심 메시지:
- [ ] "3개국어 지원" 명시적 강조
- [ ] "정규화/역정규화" 개념 설명
- [ ] "실시간 처리" 강조
- [ ] 핵심 메시지 반복 (최소 2회)

### 시각적 강조:
- [ ] 언어 토글 버튼 크기 확대
- [ ] 배지/아이콘 추가
- [ ] 강조색 활용 증가
- [ ] 섹션 구분 명확화

### 기억성:
- [ ] 간단한 설명 추가 ("SNAP이란?")
- [ ] 핵심 요약 섹션 추가
- [ ] 인포그래픽/다이어그램 추가
- [ ] 기술 용어 단순화

### 행동 유도:
- [ ] 명확한 CTA 버튼 추가
- [ ] 다음 단계 명시
- [ ] 문서/데모 링크 강조
- [ ] 모바일 CTA 최적화

---

## 🎯 **최종 평가**

### 현재 상태:
- ✅ 기술적으로 정확함
- ✅ 정보 계층 논리적
- ⚠️ 메시징 강도 약함
- ⚠️ 3개국어 가시성 부족
- ⚠️ 핵심 개념 강조 부족

### 개선 후 예상:
- 방문자가 "SNAP = 3개국어 정규화/역정규화"를 명확히 인식
- 더 높은 기억성
- 더 강한 행동 유도
- 더 나은 사용자 경험

### 우선순위:
1. **3개국어 가시성 강화** — 필수
2. **핵심 메시지 강조** — 필수
3. **정규화/역정규화 구분** — 권장
4. **CTA 추가** — 권장

---

## ✨ **결론**

SNAP 웹사이트는 **기술적으로 정확하고 정보 계층이 논리적**이지만, **메시징 강도가 약합니다**.

**강점**:
- 첫 인상 명확
- 정보 계층 우수
- 3언어 지원 구현됨

**약점**:
- 3개국어 가시성 부족
- 핵심 메시지 강조 약함
- 정규화/역정규화 구분 불명확
- 기억성 낮음

**권장 조치**:
위의 개선안을 적용하면 방문자가 "SNAP = 3개국어 정규화/역정규화 프로젝트"를 명확히 인식할 수 있을 것입니다.