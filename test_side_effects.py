import sys
sys.path.insert(0, 'c:/work/snap/snap_py')
sys.stdout.reconfigure(encoding='utf-8')

from snap.bert_session import BertSessionManager
from snap.classifier import ContextClassifier
from snap.text_normalize_kr import scan, apply_spans
from snap.phonology_kr import apply_rules

WEIGHTS = 'c:/work/snap/snap_py/weights'
bm = BertSessionManager()
bm.load('KR', f'{WEIGHTS}/ko/model_bert_int8.onnx', f'{WEIGHTS}/ko')
clf = ContextClassifier('ko', WEIGHTS, bert_manager=bm)

def g2p(text):
    r = clf.process(text)
    spans = scan(text, r.get('numbers', []))
    txt = apply_spans(text, spans)
    return apply_rules(txt, r.get('annotations', []), r.get('morphs', []))

cases = [
    # 규칙 1: 영문 뒤 1~10 → 영어 발음
    ("M4 칩",            "엠포 칩"),
    ("i9 프로세서",       "아이나인 프로세서"),
    ("window8",          None),           # 윈도에잇 (window사전, 8=에잇)
    ("Windows 10",       "윈도우즈 텐"),

    # 규칙 2: 영문 뒤 11~99 → 한자음
    ("Windows 11",       None),           # 윈도우즈 십일 (음운 후)
    ("iPhone 16 Pro",    None),           # 아이폰 십육 프로 (음운 후)

    # 규칙 3: 영문 뒤 4자리 이상, 100의 배수 → 한자음
    ("a1000",            "에이천"),
    ("a5000",            "에이오천"),
    ("a2000",            "에이이천"),
    ("a10000",           "에이만"),

    # 규칙 4: 영문 뒤 4자리 이상, 100의 배수 아님 → 자릿수
    ("RTX 3090",         None),           # 알티엑스 삼공구공
    ("GTX 1080",         None),           # 지티엑스 일공팔공
    ("a1549",            "에이 일오사구"),
    ("a1200",            "에이천이백"),    # 100의 배수 → 한자음

    # 회귀 테스트
    ("커피 3잔을 마셨다.", "커피 세자늘 마셛따."),
    ("15층에 살고 있다.",  "시보층에 살고 읻따."),
    ("AI 3개를 만들었다.", None),          # AI 뒤지만 단위(개) 있으므로 세개
    ("BaaS를 100달러에",  "바스를 백딸러에"),
]

print(f"{'원문':<22} {'결과':<28} {'판정'}")
print("-" * 65)
for text, expected in cases:
    result = g2p(text)
    if expected is None:
        mark = "  "
        exp_str = "(확인)"
    else:
        mark = "✅" if result == expected else "❌"
        exp_str = expected
    print(f"{mark} {text:<20} → {result:<28}  예상: {exp_str}")
