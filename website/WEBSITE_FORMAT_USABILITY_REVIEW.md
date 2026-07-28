# SNAP 웹사이트 형식·배치·사용성·적합도 종합 평가

> **작성일**: 2026-07-28  
> **평가 범위**: 형식, 배치, 사용성, 적합도 (콘텐츠 제외)  
> **평가 기준**: 기술 포트폴리오 사이트 표준

---

## 📊 **종합 평가: 8.2/10** — 우수한 기술 포트폴리오 사이트

| 항목 | 평가 | 점수 | 설명 |
|------|------|------|------|
| **레이아웃 구조** | ⭐⭐⭐⭐⭐ | 9/10 | 2단 고정/스크롤 패턴 우수 |
| **시각 계층** | ⭐⭐⭐⭐ | 8/10 | 색상 대비 좋음, 섹션 구분 개선 필요 |
| **네비게이션** | ⭐⭐⭐⭐ | 8/10 | 명확하고 직관적 |
| **반응형 설계** | ⭐⭐⭐⭐ | 8/10 | 모바일 양호, 태블릿 개선 필요 |
| **타이포그래피** | ⭐⭐⭐⭐ | 8/10 | 폰트 선택 적절, 계층 명확 |
| **인터랙션** | ⭐⭐⭐ | 7/10 | 커서 이펙트 있음, 피드백 부족 |
| **접근성** | ⭐⭐⭐ | 7/10 | 색상 대비 양호, 포커스 상태 미흡 |
| **성능** | ⭐⭐⭐⭐ | 8/10 | 가벼운 구조, 로딩 빠름 |
| **브랜드 일관성** | ⭐⭐⭐⭐ | 8/10 | 기술 제품 이미지 잘 표현 |
| **정보 구조** | ⭐⭐⭐⭐ | 8/10 | 논리적 흐름, 섹션 연결 명확 |

---

## 🟢 **잘된 점**

### 1. **레이아웃 구조 (9/10)** ⭐⭐⭐⭐⭐

#### 강점:
```
✅ 2단 고정/스크롤 패턴 (Profile 사이트 스타일)
   - 왼쪽: 고정 헤더 (sticky)
   - 오른쪽: 스크롤 콘텐츠
   - 데스크톱에서 매우 효과적

✅ 시각적 균형
   - 50:50 분할로 대칭성 우수
   - 왼쪽 고정으로 네비게이션 항상 접근 가능
   - 오른쪽 콘텐츠에 집중 가능

✅ 모바일 적응
   - 자동 스택 레이아웃
   - 터치 친화적 구조
```

---

### 2. **네비게이션 (8/10)** ⭐⭐⭐⭐

#### 강점:
```
✅ 명확한 구조
   - 7개 주요 섹션 (적절한 수)
   - 계층적 표현 (01, 02, 03...)
   - 3언어 지원

✅ ScrollSpy 기능
   - 현재 위치 강조
   - 부드러운 스크롤
   - 활성 상태 시각화

✅ 접근성
   - 키보드 네비게이션 가능
   - aria-label 포함
```

---

### 3. **색상 팔레트 (9/10)** ⭐⭐⭐⭐⭐

#### 강점:
```
✅ 다크 테마 (Onyx #0a0a0a)
   - 기술 제품에 적합
   - 눈 피로 적음
   - 프리미엄 느낌

✅ 강조색 (Slate Blue #93c5fd)
   - 활성 상태 명확
   - WCAG AA 대비 충분
   - 세련된 선택

✅ 계층적 회색
   - 텍스트 가독성 우수
   - 시각적 깊이 표현
```

---

### 4. **타이포그래피 (8/10)** ⭐⭐⭐⭐

#### 강점:
```
✅ 폰트 선택
   - Inter: 본문 (현대적, 가독성 우수)
   - Noto Sans JP: 일본어 (명확함)
   - Monospace: 기술 용어 (구분 명확)

✅ 크기 계층
   - H1: 4xl (36px) → 5xl (48px) 반응형
   - H2: base (16px)
   - Body: sm (14px)
   - 명확한 계층 구조

✅ 자간 활용
   - tracking-tight: 제목 (-0.025em)
   - tracking-widest: 라벨 (0.1em)
   - 시각적 강조 효과
```

---

### 5. **반응형 설계 (8/10)** ⭐⭐⭐⭐

#### 강점:
```
✅ Tailwind 기반 체계적 구조
   - sm: 640px (모바일)
   - md: 768px (태블릿)
   - lg: 1024px (데스크톱)

✅ 모바일 우선 설계
   - 기본: 모바일 레이아웃
   - lg: 데스크톱 레이아웃
   - 자연스러운 확장

✅ 터치 친화성
   - 버튼 크기 충분 (py-2, py-3)
   - 탭 간격 적절
```

---

### 6. **인터랙션 (7/10)** ⭐⭐⭐

#### 강점:
```
✅ 커서 Glow 이펙트
   - 마우스 따라다니는 라디얼 그래디언트
   - 세련되고 현대적
   - 성능 최적화됨 (CSS 변수)

✅ 네비게이션 호버
   - nav-indicator 확장 (2rem → 4rem)
   - 텍스트 색상 변화
   - 부드러운 전환 (0.2s)

✅ 탭 인터페이스
   - 언어별 예시 탭
   - 부드러운 전환
```

---

## 🟡 **개선 필요 사항**

### 1. **시각 계층 약화 (8/10)** ⚠️

#### 문제점:
```
❌ 섹션 구분 불명확
   - 섹션 간 경계 약함
   - 배경색 변화 없음
   - 구분선(divider) 부재

❌ 카드 구분 미흡
   - 모든 콘텐츠가 평면적
   - 깊이감 부족
   - 그림자(shadow) 최소화

❌ 강조 요소 부족
   - 중요 정보 시각적 강조 약함
   - 배경색 변화 없음
```

#### 개선안:
```html
<!-- 섹션 구분선 추가 -->
<section class="pb-12 mb-12 border-b border-slate-800/50">
  <!-- 콘텐츠 -->
</section>

<!-- 카드 깊이감 추가 -->
<div class="p-4 rounded-lg border border-slate-800 
            bg-gradient-to-br from-slate-900/50 to-slate-950/50
            shadow-md hover:shadow-lg transition-shadow">
  <!-- 콘텐츠 -->
</div>
```

---

### 2. **인터랙션 피드백 부족 (7/10)** ⚠️

#### 문제점:
```
❌ 버튼 피드백 미흡
   - 언어 토글 버튼: 호버 상태만 있음
   - 클릭 피드백 없음
   - 활성 상태 시각화 약함

❌ 스크롤 인디케이터 부재
   - 현재 위치 파악 어려움
   - ScrollSpy 있으나 시각적 표현 약함

❌ 로딩 상태 없음
   - 데모 입력 시 피드백 없음
   - 사용자 혼동 가능
```

#### 개선안:
```html
<!-- 버튼 활성 상태 명확화 -->
<button class="active:scale-95 active:opacity-80 
               transition-all duration-150">
  EN
</button>

<!-- 스크롤 진행 표시 -->
<div class="fixed top-0 left-0 h-1 bg-accent"
     style="width: var(--scroll-progress)"></div>
```

---

### 3. **섹션 흐름 및 구조 (8/10)** ⚠️

#### 문제점:
```
❌ 섹션 제목 위치 혼동
   - 모바일: sticky 헤더 (보임)
   - 데스크톱: sr-only (숨김)
   - 일관성 부족

❌ 네비게이션 항목 과다
   - 8개 항목 (스크롤 필요)
   - 모바일에서 접근성 저하

❌ 콘텐츠 밀도 불균형
   - 섹션별 길이 차이 큼
   - 시각적 리듬 약함
```

#### 개선안:
```html
<!-- 섹션 제목 일관성 -->
<section id="about">
  <h2 class="text-lg font-bold mb-6 text-slate-200">
    정규화 개요 및 예시
  </h2>
  <!-- 콘텐츠 -->
</section>

<!-- 네비게이션 축약 -->
<nav class="space-y-1 max-h-96 overflow-y-auto">
  <!-- 주요 항목만 표시 -->
</nav>
```

---

### 4. **접근성 (7/10)** ⚠️

#### 문제점:
```
❌ 포커스 상태 미흡
   - :focus-visible 스타일 없음
   - 키보드 네비게이션 어려움

❌ 색상 대비 (일부)
   - slate-400 텍스트: 대비 충분
   - 하지만 일부 배경에서 약할 수 있음

❌ 스크린 리더 지원 부족
   - aria-label 최소화
   - 구조적 마크업 개선 필요
```

#### 개선안:
```css
/* 포커스 상태 추가 */
a:focus-visible,
button:focus-visible {
  outline: 2px solid var(--accent);
  outline-offset: 2px;
}

/* 색상 대비 검증 */
/* WCAG AA: 4.5:1 (텍스트), 3:1 (UI) */
```

---

### 5. **태블릿 최적화 (7/10)** ⚠️

#### 문제점:
```
❌ 태블릿 (768px-1024px) 레이아웃 미흡
   - 2단 레이아웃 적용 안 됨
   - 1단 모바일 레이아웃 유지
   - 화면 공간 낭비

❌ 터치 타겟 크기
   - 일부 버튼 작음 (최소 44px 권장)
   - 언어 토글 버튼 특히 작음

❌ 스크롤 성능
   - 긴 페이지에서 프레임 드롭 가능
   - 커서 이펙트 최적화 필요
```

#### 개선안:
```html
<!-- 태블릿 2단 레이아웃 추가 -->
<div class="md:flex md:justify-between md:gap-4">
  <header class="md:sticky md:w-1/2">...</header>
  <main class="md:w-1/2">...</main>
</div>

<!-- 터치 타겟 확대 -->
<button class="px-4 py-2 min-h-[44px] min-w-[44px]">
  EN
</button>
```

---

## 🔴 **주요 형식 이슈**

### 1. **카드 스타일 일관성 부족**

#### 현재:
```html
<!-- 일부 카드 -->
<div class="p-5 rounded-md border border-slate-800 
            bg-slate-900/30">
  <!-- 콘텐츠 -->
</div>

<!-- 다른 카드 -->
<div class="p-3 rounded bg-slate-950 border border-slate-800">
  <!-- 콘텐츠 -->
</div>
```

#### 문제:
- 패딩 불일치 (p-5 vs p-3)
- 배경색 다름 (bg-slate-900/30 vs bg-slate-950)
- 반경 다름 (rounded-md vs rounded)

#### 개선안:
```css
/* 카드 컴포넌트 표준화 */
.card {
  @apply p-4 rounded-lg border border-slate-800 
         bg-slate-900/50 transition-all duration-200;
}

.card:hover {
  @apply border-slate-700 bg-slate-900/70 shadow-lg;
}
```

---

### 2. **공백(Spacing) 불일치**

#### 현재:
```html
<!-- 섹션 간 -->
<section class="mb-16 md:mb-24 lg:mb-36">...</section>

<!-- 요소 간 -->
<div class="space-y-2">...</div>
<div class="space-y-3">...</div>
<div class="space-y-5">...</div>
<div class="space-y-6">...</div>
```

#### 문제:
- 공백 규칙 일관성 없음
- 8px 단위 미준수
- 시각적 리듬 약함

#### 개선안:
```css
/* 8px 기반 스케일 */
--spacing-xs: 4px;   /* 0.5 */
--spacing-sm: 8px;   /* 1 */
--spacing-md: 16px;  /* 2 */
--spacing-lg: 24px;  /* 3 */
--spacing-xl: 32px;  /* 4 */
--spacing-2xl: 48px; /* 6 */

/* 사용 */
.section { margin-bottom: var(--spacing-2xl); }
.card { padding: var(--spacing-lg); }
```

---

## ✏️ **우선순위별 개선안**

### Priority 1: 시각 계층 강화 (높음)

```html
<!-- 1. 섹션 구분선 추가 -->
<section class="pb-12 mb-12 border-b border-slate-800/50">
  <h2 class="text-lg font-bold mb-6">섹션 제목</h2>
  <!-- 콘텐츠 -->
</section>

<!-- 2. 카드 깊이감 추가 -->
<div class="p-4 rounded-lg border border-slate-800 
            bg-gradient-to-br from-slate-900/50 to-slate-950/50
            shadow-md hover:shadow-lg transition-shadow">
  <!-- 콘텐츠 -->
</div>

<!-- 3. 강조 배경 추가 -->
<div class="p-4 rounded-lg bg-slate-900/80 border-l-2 border-accent">
  <p class="text-slate-200 font-medium">중요 정보</p>
</div>
```

### Priority 2: 인터랙션 피드백 추가 (중간)

```html
<!-- 1. 버튼 피드백 -->
<button class="px-3 py-1 rounded transition-all duration-150
               hover:bg-slate-800 active:scale-95 active:opacity-80
               focus-visible:outline-2 focus-visible:outline-offset-2
               focus-visible:outline-accent">
  EN
</button>

<!-- 2. 스크롤 진행 표시 -->
<div id="scroll-progress" class="fixed top-0 left-0 h-1 bg-accent"
     style="width: 0%"></div>

<!-- 3. 로딩 상태 -->
<div id="tn-output" class="p-3 rounded bg-slate-950 
                           border border-slate-800 
                           text-slate-200 text-xs font-mono
                           min-h-12 flex items-center">
  <span class="animate-pulse">처리 중...</span>
</div>
```

### Priority 3: 접근성 개선 (중간)

```html
<!-- 1. 포커스 상태 -->
<style>
  a:focus-visible,
  button:focus-visible {
    outline: 2px solid var(--accent);
    outline-offset: 2px;
  }
</style>

<!-- 2. ARIA 라벨 -->
<button id="btn-ko" aria-label="한국어로 변경" aria-pressed="true">
  KO
</button>

<!-- 3. 색상 대비 검증 -->
<!-- WCAG AA 최소 4.5:1 (텍스트) -->
```

### Priority 4: 반응형 개선 (낮음)

```html
<!-- 태블릿 2단 레이아웃 -->
<div class="md:flex md:justify-between md:gap-4">
  <header class="md:sticky md:w-1/2">...</header>
  <main class="md:w-1/2">...</main>
</div>

<!-- 터치 타겟 확대 -->
<button class="px-4 py-2 min-h-[44px] min-w-[44px]">
  EN
</button>
```

---

## 📋 **형식·배치 개선 체크리스트**

### 시각 계층:
- [ ] 섹션 구분선 추가 (border-b)
- [ ] 카드 그림자 강화 (shadow-md → shadow-lg)
- [ ] 배경 그래디언트 추가
- [ ] 강조 요소 배경색 추가

### 인터랙션:
- [ ] 버튼 active 상태 추가 (scale-95)
- [ ] 스크롤 진행 표시 추가
- [ ] 포커스 상태 스타일 추가
- [ ] 로딩 상태 애니메이션

### 타이포그래피:
- [ ] 제목 크기 계층 정리 (h2, h3, h4)
- [ ] 공백 규칙 표준화 (8px 단위)
- [ ] 라인 높이 일관성 (leading-relaxed)

### 접근성:
- [ ] ARIA 라벨 추가
- [ ] 색상 대비 검증 (WCAG AA)
- [ ] 포커스 상태 명확화
- [ ] 키보드 네비게이션 테스트

### 반응형:
- [ ] 태블릿 레이아웃 추가 (md:)
- [ ] 터치 타겟 크기 확대 (44px)
- [ ] 모바일 네비게이션 최적화
- [ ] 스크롤 성능 최적화

---

## 🎯 **최종 평가**

### 현재 상태:
- ✅ 기술 포트폴리오로서 기본 구조 우수
- ✅ 색상과 타이포그래피 세련됨
- ⚠️ 시각 계층과 인터랙션 개선 필요
- ⚠️ 접근성 강화 필요

### 개선 후 예상:
- 더 명확한 정보 계층
- 향상된 사용자 경험
- 더 나은 접근성
- 전문적인 느낌 강화

### 추천 개선 순서:
1. **시각 계층 강화** (Priority 1) — 필수
2. **인터랙션 피드백** (Priority 2) — 권장
3. **접근성 개선** (Priority 3) — 권장
4. **반응형 최적화** (Priority 4) — 선택

---

## ✨ **결론**

SNAP 웹사이트는 **기술 포트폴리오로서 우수한 기본 구조**를 갖추고 있습니다.

**강점**:
- 2단 레이아웃 패턴 효과적
- 색상 팔레트 세련됨
- 타이포그래피 명확함
- 네비게이션 직관적

**개선점**:
- 시각 계층 강화 필요
- 인터랙션 피드백 추가 필요
- 접근성 개선 필요
- 태블릿 최적화 필요

**권장 조치**:
Priority 1-2 개선사항을 적용하면 더욱 전문적이고 사용자 친화적인 웹사이트가 될 것입니다.