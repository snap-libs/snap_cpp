import sys, ctypes, os, json
sys.stdout.reconfigure(encoding='utf-8')
dll_dir = 'c:/work/snap/snap_cpp/build/Release'
os.add_dll_directory(dll_dir)
snap = ctypes.CDLL(dll_dir + '/snap_cpp.dll')
snap.snap_create.argtypes   = [ctypes.c_char_p, ctypes.c_char_p]
snap.snap_create.restype    = ctypes.c_void_p
snap.snap_process.argtypes  = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_process.restype   = ctypes.c_void_p
snap.snap_normalize.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
snap.snap_normalize.restype  = ctypes.c_void_p
snap.snap_free.argtypes     = [ctypes.c_void_p]
snap.snap_free.restype      = None
snap.snap_destroy.argtypes  = [ctypes.c_void_p]
snap.snap_destroy.restype   = None

engine = snap.snap_create(b'c:/work/snap/snap_py/weights', b'ko')
print('[OK] C++ 초기화 완료\n')

tests = [
    '가속(CUDA)을 지원한다.',
    '인공지능(AI) 기술이다.',
    '쿠다(cuda 를 어쩌구',
    'CUDA를 사용한다.',
]

for text in tests:
    tb = text.encode('utf-8')

    ptr = snap.snap_normalize(engine, tb)
    norm = ctypes.string_at(ptr).decode('utf-8') if ptr else '(null)'
    if ptr: snap.snap_free(ptr)

    ptr = snap.snap_process(engine, tb)
    if ptr:
        raw = json.loads(ctypes.string_at(ptr).decode('utf-8'))
        if isinstance(raw, list):
            phonology = raw[0].get('phonology', '(null)')
        else:
            phonology = raw.get('phonology', '(null)')
        snap.snap_free(ptr)
    else:
        phonology = '(null)'

    print(f'원문    : {text}')
    print(f'normalize: {norm}')
    print(f'phonology: {phonology}')
    print()

snap.snap_destroy(engine)
