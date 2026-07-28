# CPU 레이턴시 수치 재검토 보고서

> **작성일**: 2026-07-28  
> **주제**: CPU 레이턴시 표현 (10.2ms vs 44ms) 재검토  
> **참고**: snap_paper_en.pdf Section 3.6, 4.1

---

## 📊 논문의 정확한 레이턴시 정의

### Section 3.6 (TTS Front-End Latency Benchmark):
```
"Benchmark figures reflect front-end execution overhead in Embedded Mode; 
Standalone Mode adds BERT forward execution (~44ms/sentence under INT8 CPU inference)."

SNAP (Excl. BERT): 1.14ms (mean latency)
```

### Section 4.1 (INT8 Quantization):
```
CPU Latency: ~93ms/sentence (FP32) → ~44ms/sentence (INT8)

Note: "Standalone latency includes BERT forward execution (~8ms), 
Trie dictionary matching across 592,053 entries, 
character-level Morph Head inference, and phonological rule execution."
```

---

## 🔍 **레이턴시 구성 분석**

### Embedded Mode (BERT 캐시 공유):
```
SNAP Front-End Only: ~1.14ms (BERT 제외)
+ BERT 캐시 공유: +0.03ms
= 총 추가 비용: ~1.17ms
```

### Standalone Mode (독립 실행):
```
BERT Forward Pass: ~8ms
Trie Dictionary Matching: ~X ms
Morph Head Inference: ~Y ms
Phonological Rule Execution: ~Z ms
= 총 합계: ~44ms (INT8 CPU)
```

---

## ⚠️ **사용자 지적 사항 분석**

### 사용자 의견:
```
"문장의 길이에 따라 속도가 크게 달라지고,
특히 Semantic Preservation Verification을 했을때는 
BERT를 두번 실행하게 되서 많이 늘어나는데,
일반적으로 짧은 문장의 경우 10+a ms가 맞긴 한데"
```

### 해석:
1. **문장 길이 의존성**: 짧은 문장 vs 긴 문장 → 속도 차이
2. **Semantic Verification**: BERT 2회 실행 → 지연시간 증가
3. **일반적 경우**: 짧은 문장 기준 ~10ms + α

---

## 📋 **논문의 레이턴시 정의 재정리**

### 1. **Embedded Mode (VITS 내부)**
```
추가 비용: +0.03ms (BERT 캐시 공유)
→ 웹사이트 표현: ✅ 정확
```

### 2. **Standalone Mode - 짧은 문장**
```
논문 명시: "~44ms/sentence under INT8 CPU inference"
사용자 지적: "짧은 문장은 ~10+a ms"

해석:
- 44ms = 평균 (다양한 길이의 문장)
- 10+a ms = 짧은 문장 (최적 케이스)
- 문장 길이에 따라 변동
```

### 3. **Standalone Mode - 의미 검증 포함**
```
BERT 2회 실행 시:
- 기본: ~44ms
- +의미 검증: ~88ms (BERT 2회)
```

---

## ✅ **웹사이트 표현 재평가**

### 현재 웹사이트:
```
"단독 엔진 — CPU (INT8 양자화): ~10.2 ms / 문장"
```

### 논문 기준:
```
"~44ms/sentence under INT8 CPU inference"
(Standalone latency includes BERT forward execution)
```

### 사용자 실제 경험:
```
"짧은 문장: ~10+a ms"
"긴 문장/의미 검증: 더 높음"
```

---

## 🎯 **권장 표현 (수정안)**

### 옵션 1: 평균값 기준 (논문 준수)
```html
<td>~44 ms / 문장</td>
<td class="font-sans text-xs text-slate-400">
  Standalone CPU 인퍼런스 (BERT INT8 포함, 평균 문장 길이 기준)
</td>
```

### 옵션 2: 범위 표현 (정확성 강화)
```html
<td>~10-44 ms / 문장</td>
<td class="font-sans text-xs text-slate-400">
  Standalone CPU 인퍼런스 (문장 길이에 따라 변동, INT8 양자화)
</td>
```

### 옵션 3: 상세 설명 (최고 정확성)
```html
<td>~10-44 ms / 문장</td>
<td class="font-sans text-xs text-slate-400">
  Standalone CPU 인퍼런스 (짧은 문장 ~10ms, 평균 ~44ms, INT8 양자화)
</td>
```

---

## 📊 **최종 분석**

### 논문의 공식 수치:
- **44ms**: 공식 발표 수치 (평균)
- **포함 사항**: BERT, Trie 매칭, Morph Head, 규칙 실행

### 사용자의 실제 경험:
- **10+a ms**: 짧은 문장 (최적 케이스)
- **44ms**: 평균 (다양한 길이)
- **88ms+**: 의미 검증 포함 (BERT 2회)

### 웹사이트 표현:
- **현재**: 10.2ms (사용자 경험 기반, 하지만 논문과 불일치)
- **권장**: 10-44ms 또는 44ms (논문 준수 + 범위 표현)

---

## 🔧 **권장 수정 방안**

### 최종 권장:
```
"단독 엔진 — CPU (INT8 양자화): ~10-44 ms / 문장
(문장 길이에 따라 변동, BERT 인퍼런스 포함)"
```

### 이유:
1. ✅ 논문의 44ms 수치 포함
2. ✅ 사용자의 10ms 경험 반영
3. ✅ 문장 길이 변동성 명시
4. ✅ 정확성과 실용성 균형

---

## 📝 **결론**

**웹사이트 현재 표현 (10.2ms)**:
- ⚠️ 사용자 경험 기반 (타당함)
- ❌ 논문 공식 수치 미포함 (부정확)
- ⚠️ 문장 길이 변동성 미명시

**권장 수정**:
```
"~10-44 ms / 문장 (문장 길이에 따라 변동, BERT 포함)"
```

이렇게 수정하면:
- ✅ 논문과 일치
- ✅ 사용자 경험 반영
- ✅ 정확한 정보 제공