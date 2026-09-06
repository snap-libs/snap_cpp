# 📖 SNAP Korean v2.0 Functional Specification

> **Semantic Normalization via Attached Probes (SNAP) for Korean TTS Frontend & Text Normalization**  
> Complete functional specification for the SNAP Korean v2.0 engine.  
> SNAP is not an acoustic synthesis model that outputs audio directly; rather, it is a **TTS Frontend** engine that produces phonetically normalized text, G2P phoneme sequences, and prosodic pause annotations to ensure downstream TTS engines articulate speech accurately.

[English](SNAP_KO_v2.0_FUNCTIONAL_SPEC_EN.md) | [한국어](SNAP_KO_v2.0_FUNCTIONAL_SPEC.md)

---

## 📑 Table of Contents
1. [Korean G2P & 30 NIKL Standard Pronunciation Rules](#1-korean-g2p--30-nikl-standard-pronunciation-rules)
   - 1.1. Full Support System for 30 NIKL Articles (Appendix Integration)
   - 1.2. 8 Major Context & POS-Driven Phonological Rules
   - 1.3. Phonological Vowel Length SSML Prosody Control (`vowel_length`)
2. [Context-Aware Disambiguation](#2-context-aware-disambiguation)
   - 2.1. Heteronym Contextual Disambiguation (POS-Based & Neural Head)
   - 2.2. Numeral and Counter Normalization (Counter Probing Head: 8 Major Classes & 50+ Units)
   - 2.3. Semiotic Context Normalization & Colloquialization (Semiotic Probing Head)
3. [English & Loanword Normalization](#3-english--loanword-normalization)
   - 3.1. General English Vocabulary & Brand Dictionary Pronunciation
   - 3.2. Everyday Korean Conversational Loanword Adaptation
     - 3.2.1. Syllabification of Out-of-Vocabulary (OOV) English Compounds
     - 3.2.2. Mixed Alphanumeric Product Model Names
     - 3.2.3. Popular Conversational Tensification Idioms
4. [Text Normalization (TN) for Numerals, Formatting, and Units](#4-text-normalization-tn-for-numerals-formatting-and-units)
   - 4.1. Currency, Financial Amounts, and Complex Multipliers
   - 4.2. Physical, Engineering, and IT Units (100+ Types with 3 `unit_style` Modes)
   - 4.3. Dates, Time, Quarters, and Fractions
   - 4.4. Phone Numbers, IP Addresses, and Version Numbering
   - 4.5. Mathematical Operators and Symbol Compounds
   - 4.6. Non-Verbal Decorative Symbols and Parenthetical Filtering
5. [Styling & Prosody Controls](#5-styling--prosody-controls)
   - 5.1. 3 Korean Pronunciation Style Modes (`pronunciation_style`)
   - 5.2. Speech Style & Sentence-Ending Mutation (`speech_style`)
   - 5.3. Prosodic Phrasing & Sentence Boundary Detection (SBD)
   - 5.4. Dynamic User Custom Dictionary (`custom_dict`)
   - 5.5. Text Normalization Only Mode (`tn_only`)
   - 5.6. International Phonetic Alphabet (IPA) Transcription (`return_ipa` / `to_ipa`)
- [Appendix: Exhaustive Mapping Table of 30 NIKL Standard Pronunciation Articles](#appendix-exhaustive-mapping-table-of-30-nikl-standard-pronunciation-articles)

---

## 1. Korean G2P & 30 NIKL Standard Pronunciation Rules

### 1.1. Full Support System for 30 NIKL Articles (Appendix Integration)
SNAP v2.0 supports all 30 articles across 7 chapters defined in the official **Standard Pronunciation Rules of the Republic of Korea** (Ministry of Culture, Sports and Tourism Notice No. 2017-13).
* Full deterministic coverage from Chapter 1 (General Principles, Article 1) to Chapter 7 (Sai-sori Tensification, Article 30).
* **Article 22 (Verbal Vowel Assimilation):** Strictly adheres to standard prescriptive pronunciations such as `[되어]`, `[피어]`, `[기어]`, `[떼어]` (permissible variants like `[되여]`, `[피여]` are excluded by design).
* For article-by-article regulatory details, canonical examples, and implementation policies, please refer to the Appendix below.
  > 🔗 **[Appendix: Exhaustive Mapping Table of 30 NIKL Standard Pronunciation Articles](#appendix-exhaustive-mapping-table-of-30-nikl-standard-pronunciation-articles)**

---

### 1.2. 8 Major Context & POS-Driven Phonological Rules
Among the 30 articles, SNAP reliably handles the 8 major phonological mutations whose pronunciation branches depend strictly on surrounding syntactic context and morphological POS categories.

| Phonological Rule Category | Mutation Context | Default / Exception Context | Morphological & Syntactic Criteria |
|:---|:---|:---|:---|
| **Sino-Korean 'ㄹ' Tensification (Art. 26)** | `"갈등"` $\rightarrow$ **`[갈뜽]`**, `"결정"` $\rightarrow$ **`[결쩡]`** | `"발달"` $\rightarrow$ **`[발달]`**, `"살다"` $\rightarrow$ **`[살다]`** | Sino-Korean root (tensification) vs. dictionary exceptions (`발달` lax) & native Korean verb stems (`VV` lax) |
| **Lexical Morpheme Neutralized Liaison (Art. 15)** | `"겉옷"` $\rightarrow$ **`[거톧]`**, `"맛없다"` $\rightarrow$ **`[마섭따]`** | `"옷이"` $\rightarrow$ **`[오시]`**, `"꽃을"` $\rightarrow$ **`[꼬츨]`** | Lexical morphemes (coda neutralization before liaison) vs. Grammatical morphemes (immediate liaison) |
| **Passive/Causative Suffix Exception (Art. 24 Note)** | `"신고"` $\rightarrow$ **`[신꼬]`**, `"감다"` $\rightarrow$ **`[감따]`** | `"안기다"` $\rightarrow$ **`[안기다]`**, `"감기다"` $\rightarrow$ **`[감기다]`** | Verb stem tensification before endings vs. Passive/causative suffix (`-기-` maintains lax phoneme) |
| **Adnominal '-(으)ㄹ' Tensification (Art. 27)** | `"할 수 있다"` $\rightarrow$ **`[할 쑤 읻따]`**, `"갈 데가"` $\rightarrow$ **`[갈 떼가]`** | `"먹을 밥"` $\rightarrow$ **`[머글 밥]`**, `"잘 사람"` $\rightarrow$ **`[잘 사람]`** | Bound nouns after adnominal endings (`수, 것, 줄, 데, 때` whitespace scan) vs. General nouns |
| **Vowel '의' Multi-Way Branching (Art. 5.4)** | `"우리의"` $\rightarrow$ **`[우리에]`**, `"주의"` $\rightarrow$ **`[주이]`** | `"의사"` $\rightarrow$ **`[의사]`**, `"의의"` $\rightarrow$ **`[의에]`** | Genitive particle (`JKG` $\rightarrow$ `[에]`), non-initial root (`[이]`, `[에]`) vs. initial root without onset (`[의]`) |
| **Hangul Letter Name Liaison (Art. 16)** | `"디귿이"` $\rightarrow$ **`[디그지]`**, `"지읒이"` $\rightarrow$ **`[지으지]`** | `"기역이"` $\rightarrow$ **`[기여기]`**, `"니은이"` $\rightarrow$ **`[니으니]`** | Systematic palatalization & liaison pipeline when letter names meet vowel-initial case particles |
| **Verb Stem 'ㄺ' Exception (Art. 11.1)** | `"맑게"` $\rightarrow$ **`[말께]`**, `"묽고"` $\rightarrow$ **`[물꼬]`** | `"닭과"` $\rightarrow$ **`[닥꽈]`**, `"흙과"` $\rightarrow$ **`[흑꽈]`** | Verb stem 'ㄺ' + ending onset 'ㄱ' (`[ㄹ]` exception) vs. Nominal noun ('ㄱ' representative coda) |
| **2-Syllable Sino-Korean Liquid Exception (Art. 20 Note)** | `"생산량"` $\rightarrow$ **`[생산냥]`**, `"결단력"` $\rightarrow$ **`[결딴녁]`** | `"신라"` $\rightarrow$ **`[실라]`**, `"칼날"` $\rightarrow$ **`[칼랄]`** | Independent 2-syllable Sino compound suffix (`[ㄴ]` preserved) vs. General liquidization (`[ㄹ]` assimilation) |

---

### 1.3. Phonological Vowel Length SSML Prosody Control (`vowel_length`)
In accordance with Chapter 3 (Vowel Length) of the NIKL rules, SNAP identifies long-vowel initial syllables in Sino-Korean roots using a dictionary-based engine and injects prosody tags for speech synthesis.

* **SSML Tagged Mode (`vowel_length: true` & SSML enabled):**  
  Wraps long syllables with the W3C SSML `<prosody rate="85%">syllable</prosody>` tag to allow acoustic synthesis engines to articulate the extended duration naturally.
  * `"수학 문제를 풀었다"` $\rightarrow$ `<speak><prosody rate="85%">수</prosody>학 문제를 푸럳따.</speak>`
  * `"기운이 넘친다"` $\rightarrow$ `<speak><prosody rate="85%">기</prosody>우니 넘친다.</speak>`
  * `"가격표를 보았다"` $\rightarrow$ `<speak><prosody rate="85%">가</prosody>격표를 보앋따.</speak>`
  * `"고통을 참았다"` $\rightarrow$ `<speak><prosody rate="85%">고</prosody>통을 차맏따.</speak>`

* **Dictionary-Based Evaluation:**  
  Identifies hundreds of long-vowel Sino-Korean roots (`수학`, `기운`, `가격`, `성곽`, `고통`, `병원`, `사방`, etc.). When plain text output is requested (`to_ssml: false`), clean standard Korean phonetic characters are returned without SSML tags.

---

## 2. Context-Aware Disambiguation

Identical characters, numerals, and punctuation symbols convey completely different phonetic readings depending on syntactic context. Mechanical conversion based solely on surface text causes severe mispronunciations.

* **Example 1: Word Semantics (`대가`)**
  * `"희생의 대가를 치르다"` (pay the price) $\rightarrow$ **`[대까]`**
  * `"서예의 대가를 만나다"` (meet the master) $\rightarrow$ **`[대가]`**
* **Example 2: Numeral Context (`3번`)**
  * `"같은 동작을 3번 반복했다"` (repeat 3 times) $\rightarrow$ **`[세번]`** (Action frequency)
  * `"지하철 3번 출구로 나오세요"` (Exit No. 3) $\rightarrow$ **`[삼번]`** (Identifier number)
* **Example 3: Symbol Formatting (`10:12`)**
  * `"현재 시각은 10:12입니다"` (current time) $\rightarrow$ **`[열 시 십이 분]`** (Time)
  * `"경기 결과 10:12로 끝났다"` (match score) $\rightarrow$ **`[십 대 십이]`** (Score ratio)

SNAP combines syntactic analysis with neural probing heads to disambiguate identical surface representations into their contextually correct pronunciations.

### 2.1. Heteronym Contextual Disambiguation

#### 1) Part-of-Speech (POS) Tag-Based Disambiguation
Words distinguished systematically during morphological analysis due to divergent POS boundaries.

| Target Word | Tensified / Variant Context | Default / Plain Context | POS & Morphological Criteria |
|:---|:---|:---|:---|
| **신고** | `"신발을 신고"` $\rightarrow$ **`[신꼬]`** | `"경찰에 신고하다"` $\rightarrow$ **`[신고]`** | Verb stem (`신-`[VV] + `-고`[EC]) vs. Noun (`신고`[NNG]) |
| **문과** | `"인문사회 문과"` $\rightarrow$ **`[문꽈]`** | `"방문과 창문"` $\rightarrow$ **`[문과]`** | Compound noun (`문과`[NNG]) vs. Noun + Particle (`문`[NNG] + `과`[JC]) |
| **본과** | `"의과대학 본과"` $\rightarrow$ **`[본꽈]`** | `"일본과 한국"` $\rightarrow$ **`[본과]`** | Compound noun (`본과`[NNG]) vs. Proper Noun + Particle (`일본`[NNP] + `과`[JC]) |
| **이과** | `"자연계열 이과"` $\rightarrow$ **`[이꽈]`** | `"교과서 제2과"` $\rightarrow$ **`[이과]`** | Compound noun (`이과`[NNG]) vs. Numeral + Counter (`2`[SN] + `과`[NNBC]) |
| **맛** | `"맛없다"` $\rightarrow$ **`[마섭따]`** | `"맛이 좋다"` $\rightarrow$ **`[마시]`** | Noun + Adjective (`맛`[NNG] + `없-`[VA]) vs. Noun + Case Particle (`맛`[NNG] + `이`[JKS]) |
| **못** | `"못 이겨"` $\rightarrow$ **`[몯 이겨]`** | `"연못이 깊다"` $\rightarrow$ **`[연모시]`** | Negative Adverb (`못`[MAG]) vs. Noun + Particle (`연못`[NNG] + `이`[JKS]) |

#### 2) Homograph Neural Disambiguation (Heteronym Probing Head: 9 Core Words)
For homographs sharing identical POS categories (`NNG-NNG`), the Heteronym Probing Head classifies surrounding sentence semantics to determine the correct pronunciation.

| Target Word | Tensified Context (`TENS`) | Plain Context (`NONE`) | Contextual Semantic Criteria |
|:---|:---|:---|:---|
| **대가** | `"희생의 대가를 치르다"` $\rightarrow$ **`[대까]`** | `"서예의 대가를 만나다"` $\rightarrow$ **`[대가]`** | Cost/Sacrifice vs. Master/Virtuoso (Both NNG) |
| **시가** | `"부동산 시가 총액"` $\rightarrow$ **`[시까]`** | `"조선 시대 시가 문학"` $\rightarrow$ **`[시가]`** | Market price vs. Poetry/Song (Both NNG) |
| **성적** | `"성적 수치심을 느끼다"` $\rightarrow$ **`[성쩍]`** | `"기말고사 시험 성적"` $\rightarrow$ **`[성적]`** | Sexual vs. Academic grade (Both NNG) |
| **잠자리** | `"잠자리에 들 시간"` $\rightarrow$ **`[잠짜리]`** | `"하늘을 나는 고추잠자리"` $\rightarrow$ **`[잠자리]`** | Bed/Sleeping place vs. Dragonfly (Both NNG) |
| **열병** | `"유행성 열병"` $\rightarrow$ **`[열뼝]`** | `"국군의 날 부대 열병식"` $\rightarrow$ **`[열병]`** | Febrile disease vs. Military inspection (Both NNG) |
| **송장** | `"택배 배송 송장 번호"` $\rightarrow$ **`[송짱]`** | `"차가운 물에 뜬 송장"` $\rightarrow$ **`[송장]`** | Invoice/Tracking vs. Corpse (Both NNG) |
| **지적** | `"학문적 지적 호기심"` $\rightarrow$ **`[지쩍]`** | `"오류에 대한 지적"` $\rightarrow$ **`[지적]`** | Intellectual vs. Pointing out flaws (Both NNG) |
| **감기** | `"실을 팽팽하게 감기"` $\rightarrow$ **`[감끼]`** | `"독감 및 몸살 감기"` $\rightarrow$ **`[감기]`** | Winding action (Nominalized verb) vs. Cold/Flu |
| **안다** | `"아이를 품에 안다"` $\rightarrow$ **`[안따]`** | `"그 사람을 안다"` $\rightarrow$ **`[안다]`** | Embrace/Hug vs. Know/Understand |

---

### 2.2. Numeral and Counter Normalization (Counter Probing Head)
Disambiguates identical Arabic numerals into **Sino-Korean numerals (`일, 이, 삼...`)** or **Native Korean numerals (`하나, 둘, 셋...`)** depending on the bound counter meaning.

#### 1) 8 Major Numeral Counter Contexts

| Target Counter | Sino-Korean Reading (`SINO`) | Native Korean Reading (`NATIVE`) | Contextual Criteria |
|:---|:---|:---|:---|
| **번** | `3번 버스` $\rightarrow$ **`[삼번 뻐스]`** | `3번 반복했다` $\rightarrow$ **`[세번 반보캗따]`** | Route/ID number vs. Action count |
| **대** | `20대 청년` $\rightarrow$ **`[이십대]`** | `차량 2대` $\rightarrow$ **`[두대]`** | Age decade/generation vs. Vehicle/Machine unit |
| **동** | `101동` $\rightarrow$ **`[백일동]`** | `하우스 2동` $\rightarrow$ **`[두동]`** | Building block ID vs. Standalone structure unit |
| **장** | `제3장` $\rightarrow$ **`[제삼장]`** | `종이 3장` $\rightarrow$ **`[세장]`** | Book/Statute chapter vs. Paper sheet count |
| **점** | `평점 4.5점` $\rightarrow$ **`[사쩜오점]`** | `출품작 3점을 전시했다` $\rightarrow$ **`[세점]`** | Exam score/grade vs. Artwork/exhibit count |
| **단** | `태권도 4단` $\rightarrow$ **`[사단]`** | `시금치 2단` $\rightarrow$ **`[두단]`** | Martial arts rank, shelf tier vs. Vegetable/firewood bundle |
| **기** | `제5기` $\rightarrow$ **`[제오기]`** | `에어컨 2기` $\rightarrow$ **`[두기]`** | Term cohort, reactor unit vs. Machine/generator count |
| **세트** | `세트 1` $\rightarrow$ **`[세트 일]`** | `선물 2세트` $\rightarrow$ **`[두세트]`** | Game set score vs. Product package bundle |

#### 2) Automatic Support for 50+ Common Native Korean Counters
* `개` (items), `명` (people), `살` (age), `마리` (animals), `잔` (cups), `병` (bottles), `채` (houses), `권` (books), `캔` (cans), `팩` (packs), `켤레` (pairs), `그루` (trees), `송이` (flowers), `줄` (lines), `통` (containers), `조각` (pieces), `숟가락` (spoons), etc.

---

### 2.3. Semiotic Context Normalization & Colloquialization (Semiotic Probing Head)
Converts identical punctuation marks and symbols into appropriate spoken Korean words depending on context.

| Symbol | Representative Context | Spoken Korean Reading | Contextual Rule |
|:---:|:---|:---|:---|
| **`-`**<br>(Hyphen/Dash) | `010-1234-5678`<br>`-10℃`<br>`10-20개` | **`공일공 일이삼사 오육칠팔`**<br>**`영하 십도`**<br>**`십에서 이십개`** | Phone number spacing<br>Below zero / negative temperature (`영하`)<br>Numerical range (`에서`) |
| **`:`**<br>(Colon) | `14:30`<br>`3:1` | **`십사 시 삼십 분`**<br>**`삼 대 일`** | Clock time separator (`시/분`)<br>Match score / ratio (`대`) |
| **`/`**<br>(Slash) | `1/2`<br>`120km/h`<br>`2026/8/25` | **`이분의 일`**<br>**`백이십킬로미터`**<br>**`이천이십육년 팔월 이십오일`** | Fraction notation (`분의`)<br>Unit denominator<br>Date separator (`년/월/일`) |
| **`~`**<br>(Tilde) | `10~20m`<br>`3~5점을 얻었다` | **`십에서 이십미터`**<br>**`삼에서 오점을 얻었다`** | Quantity/measure range (`에서`) |
| **`.`**<br>(Period/Dot) | `3.14`<br>`2026.8.25`<br>`192.168.0.1`<br>`v2.0` | **`삼쩜일사`**<br>**`이천이십육년 팔월 이십오일`**<br>**`일구이점 일육팔점 공점 일`**<br>**`버전 이쩜영`** | Decimal point (`쩜`)<br>Date separator (`년/월/일`)<br>IP address separator (`점`)<br>Software version separator (`쩜`) |

---

## 3. English & Loanword Normalization

Converts English alphabet words into natural Korean pronunciation tailored to native speaker conventions, handling dictionaries, OOV syllabification, alphanumeric models, and popular loanword idioms.

### 3.1. General English Vocabulary & Brand Dictionary Pronunciation
* **Large-Scale CMU-Based English-Korean Dictionary:** Tens of thousands of common English words transcribed directly into standard Korean:
  * `coffee` $\rightarrow$ `커피`, `camera` $\rightarrow$ `카메라`, `music` $\rightarrow$ `뮤직`, `hotel` $\rightarrow$ `호텔`
* **IT & Global Brand Lexicon:** Correct distinction between spelling acronyms and unified word readings:
  * `ChatGPT` $\rightarrow$ `챗지피티`, `AWS` $\rightarrow$ `에이더블유에스`, `Google` $\rightarrow$ `구글`, `Apple` $\rightarrow$ `애플`, `CEO` $\rightarrow$ `씨이오`, `B2B` $\rightarrow$ `비투비`
* **Hierarchical Longest Match:** `custom_dict` $\rightarrow$ `brand lexicon` $\rightarrow$ `general CMU lexicon`.

---

### 3.2. Everyday Korean Conversational Loanword Adaptation

#### 3.2.1. Syllabification of Out-of-Vocabulary (OOV) English Compounds
For unlisted technical terms and open-source packages, an algorithmic syllabifier (`EngWordReader`) generates natural Korean readings:
* **CamelCase Decomposition:** `FastAPI` $\rightarrow$ **`패스트에이피아이`**, `DeepLearning` $\rightarrow$ **`딥러닝`**
* **Sliding Window Subword Splitting:** `glassdoor` $\rightarrow$ **`글라스도어`**, `dataset` $\rightarrow$ **`데이터셋`**
* **CV-Pattern G2P-Lite Inference:** `Stripe` $\rightarrow$ **`스트라이프`**, `Docker` $\rightarrow$ **`도커`**, `Kubernetes` $\rightarrow$ **`쿠버네티스`**

#### 3.2.2. Mixed Alphanumeric Product Model Names
* **Multi-Tier Compound Patterns:**
  * `iPhone 16 Pro Max` $\rightarrow$ `아이폰 십육 프로 맥스` (G2P: **`[아이폰 심뉵 프로 맥쓰]`**)
  * `Galaxy S24 Ultra` $\rightarrow$ `갤럭시 에스이십사 울트라` (G2P: **`[갤럭씨 에스이십싸 울트라]`**)
* **Hardware & IT Models:**
  * `RTX 4090` $\rightarrow$ **`알티엑스 사공구공`** (Digit-by-digit serial conversion)
  * `PS5` $\rightarrow$ **`피에스파이브`**, `3M` $\rightarrow$ **`쓰리엠`**, `5G` $\rightarrow$ **`파이브지`**, `100W` $\rightarrow$ **`백와트`**

#### 3.2.3. Popular Conversational Tensification Idioms
Supports colloquial tensification widely used in modern everyday Korean (`modern_standard` and `colloquial` modes):
* `버스` (bus) $\rightarrow$ **`[뻐스]`**, `서비스` (service) $\rightarrow$ **`[써비스]`**, `카페` (cafe) $\rightarrow$ **`[까페]`**, `게임` (game) $\rightarrow$ **`[껨]`**

---

## 4. Text Normalization (TN) for Numerals, Formatting, and Units

Converts non-verbal symbols, currency figures, timestamps, and measurements into grammatically fluent Korean speech.

### 4.1. Currency, Financial Amounts, and Complex Multipliers
* **Currency Symbols:** `₩10,000` $\rightarrow$ `만원`, `$100` $\rightarrow$ `백달러`, `€50` $\rightarrow$ `오십유로`
* **Large Amount Multipliers (K/M/B/T):**
  * `$10K` $\rightarrow$ `십 케이 달러`, `$2.5M` $\rightarrow$ `이쩜오 밀리언 달러`, `$2.5T` $\rightarrow$ `이쩜오 트릴리언 달러`
* **Automatic Postposition (Josa) Correction:** Automatically shifts following particles (`은/는`, `이/가`, `을/를`, `과/와`) based on the final coda of the spoken currency unit:
  * `$2.5M은` $\rightarrow$ `이쩜오 밀리언 달러는` (Corrected to `는` after vowel-final '달러')
  * `₩100K는` $\rightarrow$ `백 케이 원은` (Corrected to `은` after consonant-final '원')

---

### 4.2. Physical, Engineering, and IT Units (100+ Types)

| Input | `standard` (Default) | `full` (Full Name) | `short` (Colloquial) |
|:---|:---|:---|:---|
| `120km/h` | 백이십킬로미터 | 백이십킬로미터퍼아워 | 백이십키로 |
| `70kg` | 칠십킬로그램 | 칠십킬로그램 | 칠십키로 |
| `180cm` | 백팔십센티미터 | 백팔십센티미터 | 백팔십센티 |
| `100%` | 백퍼센트 | 백퍼센트 | 백프로 |
| `16GB` | 십육기가바이트 | 십육기가바이트 | 십육기가 |

Supported categories cover 100+ unit variations: Length (`m`, `km`, `mm`), Area (`m²`, `ha`), Volume (`L`, `mL`, `km/L`), Mass/Density (`kg`, `mg`, `㎍/㎥`), IT (`GB`, `TB`, `Mbps`, `fps`), Frequency (`Hz`, `kHz`, `rpm`), Energy/Temperature (`W`, `kWh`, `℃`, `°F`, `hPa`), and Currency (`$`, `€`, `₩`, `¥`).

---

### 4.3. Dates, Time, Quarters, and Fractions
* **Dates:** `2026.08.25` / `2026/8/25` $\rightarrow$ `이천이십육년 팔월 이십오일`
* **Time:** `14:30` $\rightarrow$ `십사 시 삼십 분`, `09:00:15` $\rightarrow$ `아홉 시 영 분 십오 초`
* **Fractions:** `1/2` $\rightarrow$ `이분의 일`, `3:1` $\rightarrow$ `삼 대 일`

---

### 4.4. Phone Numbers, IP Addresses, and Version Numbering
* **Phone Numbers:** `010-9876-5432` $\rightarrow$ `공일공 구팔칠육 오사삼이`, `1588-0000` $\rightarrow$ `일오팔팔 공공공공`
* **IP Addresses:** `192.168.0.1` $\rightarrow$ `일구이점 일육팔점 공점 일`
* **Version/Decimals:** `v2.0` $\rightarrow$ `버전 이쩜영`, `3.14` $\rightarrow$ `삼쩜일사`

---

### 4.5. Mathematical Operators and Symbol Compounds
* **Arithmetic:** `1 + 1 = 2` $\rightarrow$ `일 플러스 일은 이`, `10 × 20` $\rightarrow$ `십 곱하기 이십`
* **Range:** `10~20m` $\rightarrow$ `십에서 이십미터`
* **Symbol Ligatures:** `A&B` $\rightarrow$ `에이앤비`

---

### 4.6. Non-Verbal Decorative Symbols and Parenthetical Filtering
* **Decorative Symbols:** Automatically strips bullets (`•`, `▶`, `◆`), emojis (`😀`, `🚀`), and dividers (`===`, `---`).
* **Hanja/Translation Parentheses:** `대한민국(大韓民國)` $\rightarrow$ `대한민국` (Strips redundant Hanja annotations).
* **Parenthetical Descriptions:** `부가세(VAT) 포함` $\rightarrow$ `부가세 포함` (Strips redundant acronyms).

---

## 5. Styling & Prosody Controls

### 5.1. 3 Korean Pronunciation Style Modes (`pronunciation_style`)
* **`"modern_standard"` (Default for C++ SDK):** Balanced standard pronunciation incorporating natural modern Korean conventions (Sai-sori tensification, loanwords, particle `의` articulated as `[에]`).
* **`"strict_standard"`:** Strictly adheres to prescriptive NIKL standard rules without colloquial tensification. Ideal for news broadcasts and formal documentaries.
* **`"colloquial"` (Default for REST API):** Generates natural conversational Korean with vowel monophthongization (`시계 [시게]`), non-initial 'ㅎ' reduction (`전화 [저놔]`, `일하다 [이라다]`), and consonant nasalization assimilation (`신문 [심문]`).

---

### 5.2. Speech Style & Sentence-Ending Mutation (`speech_style`)
Transforms written/declarative endings in raw text into conversational styles for conversational agents:
* **`"original"` (Default):** Preserves raw endings (`도착했다`, `가십니까?`).
* **`"haeyo"`:** Polite informal ending (`도착했어요`, `가요?`, `그건`).
* **`"banmal"`:** Casual plain ending (`도착했어`, `가?`, `그건`, `나는`).
* **`"hapsio"`:** Formal deferential ending (`도착했습니다`, `가십니까?`).

---

### 5.3. Prosodic Phrasing & Sentence Boundary Detection (SBD)
* **3-Tier Pause Model:**
  * **Short Pause (`P1`, ~150ms):** Post-subject / case particle break $\rightarrow$ `<break strength="weak"/>`
  * **Medium Pause (`P2`, ~300ms):** Conjunctive ending / clause boundary break $\rightarrow$ `<break strength="medium"/>`
  * **Long Pause (`P3`, ~500ms):** Sentence boundary terminal break $\rightarrow$ `<break strength="strong"/>`
* **Automatic Punctuation-Free Boundary Detection:** Accurately detects sentence boundaries even in unpunctuated ASR/STT transcripts based on verbal final endings (`EF`).

---

### 5.4. Dynamic User Custom Dictionary (`custom_dict`)
* **Longest Match First:** Priority given to multi-word phrases over single tokens.
* **Top Precedence:** Overrides system dictionaries and neural predictions unconditionally.
* **Transaction Isolation:** In-memory scope per request without persistent contamination.

---

### 5.5. Text Normalization Only Mode (`tn_only`)
Setting `tn_only: true` converts non-standard tokens (digits, symbols, units, English) into plain Korean text without applying G2P phonetic mutations (e.g., `2026.8.25에 3번 버스 탐` $\rightarrow$ `이천이십육년 팔월 이십오일에 삼번 버스 탐`).

---

### 5.6. International Phonetic Alphabet (IPA) Transcription (`return_ipa` / `to_ipa`)
Setting `return_ipa: true` returns the normalized and phonologically transformed Korean speech as standard IPA symbols (e.g., `이천이십육년` $\rightarrow$ `[itɕʰʌnisimnjuŋnjʌn]`).

---

## Appendix: Exhaustive Mapping Table of 30 NIKL Standard Pronunciation Articles

Mapping of SNAP Korean v2.0 coverage against all 30 articles of the National Institute of Korean Language (NIKL) Standard Pronunciation Rules, directly implemented in [`snap_cpp/src/phonology_ko.cpp`](../snap_cpp/src/phonology_ko.cpp).

| Chapter | Article | Regulatory Specification | Representative Examples | SNAP v2.0 Status |
|:---|:---:|:---|:---|:---|
| **Ch. 1 General** | **Art. 1** | Standard Pronunciation Principles | Standard Korean grammar compliance | Full Support |
| **Ch. 2 Consonants & Vowels** | **Art. 2** | 19 Standard Consonants | ㄱ, ㄲ, ㄴ, ㄷ, ㄸ ... | Full Support |
| | **Art. 3** | 10 Monophthongs & Dipthong allowance | 참외 `[차뫼/차메]` | Full Support |
| | **Art. 4** | 11 Diphthongs | ㅑ, ㅒ, ㅕ, ㅖ, ㅘ ... | Full Support |
| | **Art. 5.1** | Monophthongization of '져, 쪄, 쳐' | 가져 `[가저]`, 쳐 `[처]` | Full Support |
| | **Art. 5.2** | 'ㅖ' pronounced as [ㅔ] except '예, 례' | 혜택 `[혜택/헤택]`, 시계 `[시계/시게]` | Style-Linked (`colloquial`) |
| | **Art. 5.3** | Post-consonantal 'ㅢ' pronounced as [ㅣ] | 희망 `[히망]`, 띄어쓰기 `[띠어쓰기]` | Full Support |
| | **Art. 5.4** | Non-initial '의' as [이], Genitive '의' as [에] | 주의 `[주의/주이]`, 우리의 `[우리에]` | Full Support |
| **Ch. 3 Vowel Length** | **Art. 6** | Phonological Vowel Length Marking | 수학 `[수ː학]`, 기운 `[기ː운]` | Optional SSML `<prosody>` |
| | **Art. 7** | Shortening from Second Syllable in Compounds | Shortened when placed second | Lexicon-Assisted |
| **Ch. 4 Coda Pronunciation** | **Art. 8** | 7 Representative Coda Rule (ㄱ, ㄴ, ㄷ, ㄹ, ㅁ, ㅂ, ㅇ) | Neutralization principles | Full Support |
| | **Art. 9** | Single & Double Coda Neutralization | 꺾다 `[꺽따]`, 옷 `[옫]`, 꽃 `[꼳]` | Full Support |
| | **Art. 10** | Cluster Reduction (ㄳ, ㄵ, ㄼ, ㄽ, ㄾ, ㅄ) | 몫 `[목]`, 앉다 `[안따]`, 값 `[갑]` | Full Support |
| | **Art. 10.1** | Cluster Exceptions: '밟-' [밥], '넓죽-' [넙] | 밟다 `[밥ː따]`, 넓죽하다 `[넙쭈카다]` | Full Support |
| | **Art. 11** | Cluster Reduction (ㄺ, ㄻ, ㄿ) | 닭 `[닥]`, 흙 `[흑]`, 삶 `[삼ː]` | Full Support |
| | **Art. 11.1** | Verb Stem 'ㄺ' + Ending 'ㄱ' Exception ([ㄹ]) | 맑게 `[말께]`, 묽고 `[물꼬]` | Full Support |
| | **Art. 12.1** | Coda 'ㅎ(ㄶ, ㅀ)' + 'ㄱ, ㄷ, ㅈ' Aspiration | 놓고 `[노코]`, 좋다 `[조타]` | Full Support |
| | **Art. 12.2** | Coda 'ㄱ, ㄷ, ㅂ, ㅈ' + Onset 'ㅎ' Aspiration | 각하 `[가카]`, 맏형 `[마텽]`, 좁히다 `[조피다]` | Full Support |
| | **Art. 12.3** | Coda 'ㅎ(ㄶ, ㅀ)' + Onset 'ㅅ' $\rightarrow$ [ㅆ] | 닿소 `[다쏘]`, 많소 `[만ː쏘]` | Full Support |
| | **Art. 12.4** | Coda 'ㅎ' + Onset 'ㄴ' $\rightarrow$ [ㄴ] Nasalization | 놓는 `[논는]`, 쌓네 `[싼네]` | Full Support |
| | **Art. 12.5** | Coda 'ㅎ' Deletion before Vowel Endings | 낳은 `[나은]`, 쌓아 `[싸아]`, 많아 `[마ː나]` | Full Support |
| | **Art. 13** | Liaison with Vowel Grammatical Morphemes | 깎아 `[까까]`, 옷이 `[오시]` | Full Support |
| | **Art. 14** | Cluster Liaison with Vowel Grammatical Morphemes | 닭을 `[달글]`, 앉아 `[안자]`, 값을 `[갑쓸]` | Full Support |
| | **Art. 15** | Neutralized Liaison before Lexical Morphemes | 겉옷 `[거톧]`, 맛없다 `[마섭따]`, 밭 아래 `[바다래]` | Full Support |
| | **Art. 16** | Letter Names + Vowel Particle Liaison | 디귿이 `[디그지]`, 키읔이 `[키으기]` | Full Support |
| **Ch. 5 Assimilation** | **Art. 17** | Palatalization ('ㄷ, ㅌ' + 'ㅣ' $\rightarrow$ [ㅈ, ㅊ]) | 굳이 `[구지]`, 같이 `[가치]`, 붙이다 `[부치다]` | Full Support |
| | **Art. 18** | Obstruent Nasalization ('ㄱ, ㄷ, ㅂ' + 'ㄴ, ㅁ') | 국물 `[궁물]`, 닫는 `[단는]`, 밥물 `[밤물]` | Full Support |
| | **Art. 19** | Liquid Nasalization ('ㅁ, ㅇ' + 'ㄹ' $\rightarrow$ [ㄴ]) | 종로 `[종노]`, 남루 `[남누]`, 협력 `[협녁→혐녁]` | Full Support |
| | **Art. 20** | Liquidization ('ㄴ' + 'ㄹ' Mutual Assimilation) | 신라 `[실라]`, 난로 `[날로]`, 칼날 `[칼랄]` | Full Support |
| | **Art. 20 Note** | 2-Syllable Sino-Korean Liquid Exception | 생산량 `[생산냥]`, 결단력 `[결딴녁]`, 입원료 `[이붠뇨]` | Full Support |
| | **Art. 21** | Assimilation Direction Rules (Progressive/Regressive) | Systematic mutation pipeline | Full Support |
| | **Art. 22** | Verbal Vowel Assimilation (Standard Forms Only) | 되어 `[되어]`, 피어 `[피어]` | Standard Forms Only |
| **Ch. 6 Tensification** | **Art. 23** | Post-Obstruent Tensification | 국밥 `[국빱]`, 깎다 `[깍따]`, 닭장 `[닥짱]` | Full Support |
| | **Art. 24** | Verb Stem Coda 'ㄴ(ㄵ), ㅁ(ㄻ)' Tensification | 신고 `[신꼬]`, 안다 `[안따]`, 삼고 `[삼꼬]` | Full Support |
| | **Art. 24 Note** | Passive/Causative Suffix '-기-' Tensification Exemption | 안기다 `[안기다]`, 감기다 `[감기다]`, 남기다 `[남기다]` | Full Support |
| | **Art. 25** | Verb Stem Coda 'ㄼ, ㄾ' Tensification | 넓게 `[널께]`, 핥다 `[할따]` | Full Support |
| | **Art. 26** | Sino-Korean 'ㄹ' Coda Tensification | 갈등 `[갈뜽]`, 발전 `[발쩐]`, 실수 `[실쑤]` | Full Support |
| | **Art. 26 Note** | Reduplicated Sino-Korean Root Exemption | 절절하다 `[절절하다]`, 괄괄하다 `[괄괄하다]` | Full Support |
| | **Art. 27** | Adnominal '-(으)ㄹ' Tensification | 할 것을 `[할 꺼슬]`, 갈 데가 `[갈 떼가]` | Full Support |
| | **Art. 28** | Compound Tensification without Sai-siot | 문고리 `[문꼬리]`, 눈동자 `[눈똥자]`, 길가 `[길까]` | Full Support |
| **Ch. 7 Sound Addition** | **Art. 29** | 'ㄴ' Insertion in Compounds/Derivatives | 솜이불 `[솜니불]`, 막일 `[망닐]`, 십육 `[심뉵]` | Full Support |
| | **Art. 30** | Sai-siot Compound Pronunciation | 냇가 `[내ː까]`, 빗물 `[빈물]`, 깻잎 `[깬닙]` | Full Support |

---
*(End of Document)*
