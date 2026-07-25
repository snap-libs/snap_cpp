import sys
sys.path.insert(0, 'c:/work/snap/snap_py')
sys.stdout.reconfigure(encoding='utf-8')

from snap.bert_session import BertSessionManager
from snap.classifier import ContextClassifier
from snap.text_normalize_kr import scan, apply_spans
from snap.phonology_kr import apply_rules

MODELS = 'models'
bm = BertSessionManager()
bm.load('KR', f'{MODELS}/ko/KR_model_bert_int8.onnx', f'{MODELS}/ko')
clf = ContextClassifier('ko', MODELS, bert_manager=bm)

tests = [
    # GPU 관련
    'nvidia cuda rtx 3090',
    'NVIDIA GeForce RTX 4090',
    # 모델번호 패턴
    'iPhone 16 Pro',
    'M4 칩을 탑재했다',
    'i7 13700K 프로세서',
    # 기존 정상 케이스 회귀 테스트
    '커피 3잔을 마셨다.',
    '15층에 살고 있다.',
    '24년 1월 1일이다.',
    '010-1234-5678로 연락 바랍니다.',
]

print('=' * 60)
for text in tests:
    r = clf.process(text)
    spans = scan(text, r.get('numbers', []))
    txt = apply_spans(text, spans)
    result = apply_rules(txt, r.get('annotations', []), r.get('morphs', []))
    print(f'원문: {text}')
    print(f'결과: {result}')
    print()
