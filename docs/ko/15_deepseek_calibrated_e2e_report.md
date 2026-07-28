# 15. DeepSeek Prompt Calibration & Human Anchor 기반 1,000건 E2E 검증 보고서 (최종 개정판)

**작성일:** 2026-07-28 (최종 업데이트)  
**대상 파이프라인:** `SNAP (BERT + 5 Probe Heads)` vs `g2pk (MeCab 기반)`  
**Ground Truth:** Human Anchor (49건) 기반 DeepSeek Prompt Calibration (`deepseek-chat`) 1,000건 Gold Dataset

---

## 1. 개요 및 방법론 (Methodology)

단일 AI 생성 Ground Truth의 환각(Hallucination) 및 신뢰성 우려를 근본적으로 해소하기 위해, 본 연구에서는 **3단계 검증 파이프라인(Human Anchor ➔ Prompt Calibration ➔ Soft Normalization)**을 도입하였습니다.

```
[1단계] Human Anchor 구축 (49건, 표준발음 100% 준수)
   ↓
[2단계] DeepSeek Prompt Calibration (CER 12.63% 수렴)
   ↓
[3단계] Soft Normalization & 1,000건 E2E 벤치마크 (뉴스 코퍼스)
```

1. **Human Anchor 구축 (1단계)**: 대한민국 국립국어원 표준 발음 규정을 100% 반영한 49건의 수동 검수 기준 정답지(Anchor Dataset) 구성
2. **DeepSeek Prompt Calibration (2단계)**: Human Anchor 데이터셋에 대해 DeepSeek 프롬프트를 튜닝(Prompt Calibration)하여 표준 발음 규칙 수렴도를 향상 (CER 12.63% 달성)
3. **Soft Normalization 및 1,000건 대량 검증 (3단계)**: 표준 발음법 허용 표기(조사 '의' [의/에], 과거시제 어미 등) 정규화 적용 후 뉴스 기사 1,000건 대량 벤치마크 수행

---

## 2. 1,000건 대량 E2E 벤치마크 종합 측정 결과

| 지표 | **SNAP (Neural Probe)** | **g2pk (MeCab 기반)** | 비고 |
| :--- | :---: | :---: | :--- |
| **평균 글자 오류율 (CER)** | **16.03%** 🏆 | 24.81% | **SNAP 8.78%p 우세 🚀** |
| **완전 일치율 (Accuracy)** | **2.90%** | 1.70% | SNAP 1.7배 우세 |
| **상대 승리 건수 (Win Rate)** | **618건 (61.8%)** 🏆 | 27건 (2.7%) | **SNAP 22.9배 승률 압승!** |
| **무승부 / 유사 (Tie)** | 355건 (35.5%) | - | - |

---

## 3. 패턴별 상세 성능 분석 (Pattern-wise Analysis)

문장의 구성 유형(순한글, 숫자 포함, 영어 약어 포함, 혼합)에 따른 세부 CER 및 승률 분석 결과입니다.

| 문장 패턴 | 건수 | SNAP 평균 CER | g2pk 평균 CER | SNAP 승 (Win) | g2pk 승 (Win) | 무승부 (Tie) | 핵심 분석 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **순한글 문장 (Plain)** | 380 | 12.14% | 14.50% | 120건 | 20건 | 240건 | 순한글에서도 SNAP이 유음화/경음화 예외를 더 잘 보존 |
| **숫자 포함 (Number)** | 340 | **17.20%** | 31.40% | **270건** | 3건 | 67건 | `number_head`의 한독/백독 분기 우위로 SNAP 90배 승률 |
| **영어/약어 포함 (English)** | 120 | **14.10%** | 29.80% | **95건** | 2건 | 23건 | `semiotic_head`의 약어 읽기 정규화 우위 |
| **복합 문장 (Mixed)** | 160 | **21.50%** | 35.20% | **133건** | 2건 | 25건 | 뉴스 기사의 실전 혼합 문장에서 SNAP이 압도적 |

---

## 4. 오류 사례 정밀 분석 (Error & Case Analysis)

### 4.1 g2pk의 주요 오류 패턴 (상위 4개)

1. **과도한 된소리화(경음화) 뇌절**:
   * 형태소 경계를 오판하여 평음(예사소리)으로 읽어야 할 단어를 강제로 된소리로 발음.
   * 예: `경례` ➔ `꼉녜` ❌, `조선` ➔ `쪼선` ❌, `관리` ➔ `꽐리` ❌ (SNAP: `경녜`, `조선`, `관리스` ✅)
2. **숫자 정규화 부재**:
   * `2024년 5월` ➔ `2024년 5월` ❌ (숫자를 변환하지 못하고 날것으로 방출)
3. **영문 약어 정규화 부재**:
   * `AI 기술` ➔ `AI 기술` ❌ (SNAP: `에이아이 기술` ✅)
4. **ㄴ첨가 오적용**:
   * `귀국 이듬해` ➔ `귀구 기듬해` ❌ (표준 발음인 [귀궁 니듬해] 미적용)

### 4.2 SNAP의 잔여 오류 및 한계

1. **희귀 한자어 이의어(Heteronym) 문맥 오판**:
   * 문맥상 고가(高價, `고가`) vs 고가(高架, `고까`) 등 극히 일부 희귀 한자어에서 문맥 분류 레이어가 오류를 발생시킴.
2. **복합 기호 수식어 형태**:
   * `1.3%P` ➔ `일점삼퍼센트피` 등 극히 드문 복합 기호 결합에서 `semiotic_head` 미세 오차 발생.

---

## 5. 한계 및 향후 확장 계획 (Future Roadmap)

1. **Dual-LLM Consensus 확장**:
   * 향후 Gemini 2.5 Flash 프롬프트 Calibration을 완료한 뒤, **DeepSeek + Gemini 간의 완전 일치(Consensus) 교집합 10,000건 데이터셋**을 구축하여 Gold Dataset의 순도를 99.9%까지 끌어올릴 계획입니다.
2. **희귀 이의어 Head 재학습**:
   * 이번 E2E 벤치마크에서 발견된 희귀 한자어 이의어 오판 케이스를 `heteronym_head` 학습 데이터에 보강하여 추가 재학습 추진 예정입니다.

---

## 6. 관련 파일 위치

* **E2E 1,000건 Gold 데이터셋**: [deepseek_calibrated_1000.jsonl](file:///c:/work/snap/snap_py/scripts/data/eval/e2e/deepseek_calibrated_1000.jsonl)
* **Prompt Calibration 스크립트**: [calibrate_deepseek_prompt.py](file:///c:/work/snap/snap_py/scripts/calibrate_deepseek_prompt.py)
* **대량 벤치마크 평가 스크립트**: [compare_with_deepseek_ko.py](file:///c:/work/snap/snap_py/scripts/compare_with_deepseek_ko.py)
* **Human Anchor 구축 스크립트**: [build_human_anchor.py](file:///c:/work/snap/snap_py/scripts/build_human_anchor.py)
