# Morphological Analysis Necessity by Language — TTS Frontend Perspective

> Created: 2026-05-28  
> Context: SNAP TTS Frontend — evaluating which languages require morphological analysis and at what depth

---

## 1. What "Morphological Analysis" Means for TTS

In TTS preprocessing, morphological analysis serves three distinct roles:

| Role | Example | Who needs it |
|------|---------|-------------|
| **Word segmentation** | `株価が` → `株価/が` | Languages with no spaces |
| **POS tagging** | `은` = subject marker vs `은` = silver | All inflected languages |
| **Reading/pronunciation assignment** | `上` → `うえ` or `じょう` | Logographic writing systems |

---

## 2. Language Classification

### 🔴 Critical — All Three Roles Needed

#### Japanese (日本語)
- **No word boundaries**: text has no spaces → word segmentation is the baseline requirement
- **Logographic writing (Kanji)**: same character has multiple readings depending on context
- **Phonological rules depend on morpheme type**: rendaku (連濁) affects compound pronunciation
- **Pitch accent**: compound vs. phrase distinction affects tone pattern
- **Traditional solution**: MeCab + IPAdic/UniDic
- **SNAP approach**: morph_head (word segmentation + POS) + kanji reading head (disambiguation)

#### Arabic (العربية)
- **Unvocalized text**: vowels are omitted in writing; morphological analysis is the only way to determine pronunciation
  - `كتب` = kataba (he wrote) / kutiba (it was written) / kutub (books) — same letters, different vowels
- **Root-pattern morphology**: consonantal root + pattern determines word class and pronunciation
- **Clitics**: prepositions/articles attach to words without spaces
- **Assessment**: Possibly the hardest G2P problem of any major language

#### Hebrew (עברית)
- Same structure as Arabic: consonantal writing, vowels omitted
- Relatively simpler morphology than Arabic but same fundamental challenge

---

### 🟡 Important — POS Tagging + Boundary Detection

#### Korean (한국어)
- **Has spaces** (eojeol units), but morpheme boundaries are *within* eojeols
- **Phonological rules are morpheme-sensitive**: tensification (경음화), liaison (연음), nasalization all require morpheme type and boundary
  - `학교` → 학꾜 (ㄱ+ㄱ tensification within morpheme boundary)
  - `코스닥 지수` → 코스닥 지수 (NOT 찌수 — space = eojeol boundary, no tensification)
- **Traditional solution**: MeCab-ko, Komoran, Okt
- **SNAP solution**: morph_head (BIO-POS, 76 classes, 121,972 training examples) — **fully implemented**

#### Finnish (Suomi)
- Highly agglutinative: a single word can carry what other languages express in a full phrase
  - `talossanikin` = "in my house too" (one word)
- Morpheme boundary determines vowel harmony and consonant gradation
- Compound words extremely long; stress is always on first syllable (simple rule) but pronunciation of consonants changes at boundaries

#### Turkish (Türkçe)
- Agglutinative: suffixes change pronunciation via vowel harmony
  - `ev` (house) + `ler` → `evler`, but `kol` (arm) + `ler` → `kollar`
- Suffix attachment changes the phoneme inventory of the stem
- Morpheme segmentation required to apply harmony rules

#### Hungarian (Magyar)
- Similar to Turkish/Finnish in structure
- Vowel harmony + consonant assimilation at morpheme boundaries

---

### 🟠 Moderate — Compound Detection + Stress

#### German (Deutsch)
- **Compound nouns are written as one word**: `Bundesverfassungsgericht` = Federal Constitutional Court
- Compound splitting is essential for correct stress placement
  - `Bundesland` → `Bundes|land` → stress on `Bun`, not `land`
- Without splitting, TTS stress is systematically wrong for compound nouns
- Inflectional morphology is simpler and mostly phonologically transparent

#### Swedish / Norwegian / Danish
- Compound words (like German)
- **Pitch accent (Swedish/Norwegian)**: word tone depends on morphological form
  - Swedish has two tones: `anden` (the duck, tone 1) vs `anden` (the spirit, tone 2)
  - Compound vs. derived word affects which tone applies

#### Russian (Русский)
- **Stress is mobile**: position shifts with inflectional form
  - `рука́` (hand, nom.) → `ру́ки` (gen.) → `руки́` (pl.)
- Without morphological analysis, stress prediction for OOV words is unreliable
- For known words: pronunciation dictionary covers most cases
- For OOV: morphological analysis of suffix helps predict stress

---

### 🟢 Minimal — POS Tagging Level Sufficient

#### English
- **Analytic language**: little inflectional morphology
- `-ed`, `-s`, `-ing` endings are handled by simple phonological rules (no morpheme analysis needed)
- **Main problem is heteronyms**: same spelling, different pronunciation
  - `read` [riːd] / [rɛd], `lead` [liːd] / [lɛd], `live` [lɪv] / [laɪv]
  - `wound`, `tear`, `bow`, `row`, `close`, `minute` (~50 words)
- **Solution**: POS tagging (or SNAP heteronym head) — equivalent to Korean heteronym head
- Compound stress (`BLACKbird` vs `black BIRD`) benefits from compound detection
- Pronunciation dictionary (CMU dict, ~130K entries) handles most cases

#### Spanish / Italian / Portuguese
- Largely phonetically transparent: spelling ≈ pronunciation
- Exception: Spanish `ll`/`y` merger, silent `h`
- Stress is mostly predictable from written accent marks
- **Assessment**: G2P rules + accent mark detection sufficient; morphological analysis not needed

#### French
- **Liaison** (linking): `les_enfants` → /le.z‿ɑ̃.fɑ̃/
  - Word boundary detection (not full morphological analysis) sufficient
- Silent letters: `h` aspiré vs. `h` muet affects liaison
- **Assessment**: Word-boundary liaison rules + pronunciation dictionary; morph analysis not needed

#### Mandarin Chinese (普通话)
- **Characters are monosyllabic**: each character = one syllable
- G2P reduces to: character → pinyin lookup (mostly unambiguous)
- Exceptions: `了`, `的`, `地`, `得` — function word disambiguation (POS level)
- **Word segmentation** is needed (no spaces), but simpler than Japanese (no kanji readings)
- **Assessment**: Word segmenter (jieba-level) sufficient; full morphological analysis overkill

#### Thai (ภาษาไทย)
- No spaces: word segmentation needed (like Japanese/Chinese)
- But Thai script is phonetic (unlike Kanji) — once segmented, reading is clear
- **Assessment**: Word segmenter + syllable rules; morphological analysis not needed

---

### Dravidian Languages (Tamil, Telugu, Kannada, Malayalam)

- Agglutinative like Korean/Finnish
- Morpheme boundaries affect pronunciation (vowel elision, consonant sandhi)
- **Assessment**: Morphological analysis needed, similar depth to Korean

---

## 3. Summary Matrix

| Language | Word Seg | POS Tag | Reading Assign | Compound Split | Assessment |
|----------|:--------:|:-------:|:--------------:|:--------------:|-----------|
| Japanese | ✅ Critical | ✅ | ✅ Critical | ✅ | **Hardest** |
| Arabic | ✅ | ✅ | ✅ Critical | — | **Hardest** |
| Hebrew | ✅ | ✅ | ✅ Critical | — | Very Hard |
| Korean | △ (spaces help) | ✅ Critical | — | — | Hard (solved) |
| Finnish | ✅ | ✅ | — | — | Hard |
| Turkish | ✅ | ✅ | — | — | Hard |
| German | — | △ | — | ✅ Critical | Moderate |
| Swedish | — | △ | — | ✅ | Moderate |
| Russian | — | ✅ | — | — | Moderate |
| English | — | △ (heteronym) | — | △ | Light |
| Spanish | — | — | — | — | Minimal |
| French | — | △ (liaison) | — | — | Minimal |
| Mandarin | △ (seg only) | △ | — | — | Minimal |
| Thai | ✅ (seg only) | — | — | — | Minimal |

---

## 4. SNAP Architecture Applicability

The SNAP architecture (BERT hidden states → shared MLP heads) is most valuable for:

1. **Languages where context determines pronunciation** (not rule-based)
   - Japanese kanji reading disambiguation
   - Arabic vowel insertion
   - English/Korean heteronyms

2. **Languages where morpheme boundaries affect phonological rules**
   - Korean (경음화, 연음)
   - Japanese (rendaku, pitch accent)
   - Finnish/Turkish (vowel harmony)

3. **Languages where a single dictionary lookup suffices**
   - Spanish, Italian: SNAP overhead not justified
   - French: rule-based liaison + dictionary is sufficient

### Priority for SNAP expansion

| Priority | Language | Rationale |
|:--------:|----------|-----------|
| 1 | Japanese | Architecture ready; morph_head is the missing piece |
| 2 | English | Heteronym head only; low effort |
| 3 | German | Compound splitter + pronunciation dict |
| 4 | Arabic | High impact; requires significant new investment |
| 5 | Turkish/Finnish | Agglutinative morph_head similar to Korean |
