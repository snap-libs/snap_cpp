@echo off
chcp 65001 >nul
set EXE=build\Release\test_e2e.exe
set WEIGHTS=c:\work\snap\snap

echo === C++ 동기화 검증 ===
echo.

for %%t in (
    "抗議書でこう書いています"
    "脱会者である面々が"
    "風俗店をお探しの方"
    "技工士さんが色や形を"
    "通常時でもひび割れが"
    "財務相は9日に"
    "増幅音による優れたサウンド"
    "論議を交わすことができるのでは"
    "急きょ取りやめた"
    "気にした方がよいかな"
    "部屋へ入るとドライヤー"
    "収束させ強力なエネルギー波で敵を蒸散させる"
) do (
    echo Input: %%t
    %EXE% %WEIGHTS% ja %%t 2>&1 | Select-String "phonology" | ForEach-Object { $_ -replace '.*"phonology":"([^"]+)".*', 'Result: $1' }
    echo.
)
