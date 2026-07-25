import sys, ctypes, os
sys.stdout.reconfigure(encoding='utf-8')

dll_dir = 'c:/work/snap/snap_cpp/build/Release'
os.add_dll_directory(dll_dir)

snap = ctypes.CDLL('c:/work/snap/snap_cpp/build/Release/snap_cpp.dll')
snap.snap_create.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
snap.snap_create.restype  = ctypes.c_void_p
snap.snap_process.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_process.restype  = ctypes.c_void_p
snap.snap_free.argtypes   = [ctypes.c_void_p]
snap.snap_free.restype    = None
snap.snap_destroy.argtypes = [ctypes.c_void_p]
snap.snap_destroy.restype  = None

engine = snap.snap_create(b'c:/work/snap/snap_py/weights', b'ko')
if not engine:
    print('[FAIL] 엔진 초기화 실패')
    sys.exit(1)
print('[OK] C++ 엔진 초기화 완료')

tests = [
    'nvidia cuda rtx 3090',
    '안과에 갔다.',
    '커피 3잔을 마셨다.',
    '15층에 살고 있으며 24년 1월 1일이다.',
]
for text in tests:
    ptr = snap.snap_process(engine, text.encode('utf-8'))
    if ptr:
        result = ctypes.string_at(ptr).decode('utf-8')
        snap.snap_free(ptr)
        print(f'  [{text}] -> [{result}]')
    else:
        print(f'  [{text}] -> [처리 실패]')

snap.snap_destroy(engine)
