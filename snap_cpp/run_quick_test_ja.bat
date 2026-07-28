@echo off
setlocal
chcp 65001 >nul

echo ========================================================
echo   SNAP TTS Frontend Windows Quick Test - Japanese (One-Click)
echo ========================================================
echo.

:: 1. Define paths and URLs
set "ZIP_URL=https://huggingface.co/softguy777/snap-weights/resolve/main/quickstart/win-x64-ja-int8-quickstart.zip"
set "ZIP_FILE=win-x64-ja-int8-quickstart.zip"
set "TARGET_DIR=..\snap\weights"

:: 2. Download ZIP file using PowerShell
echo [1/3] 허깅페이스에서 일본어 테스트 패키지 다운로드 중...
echo (약 85MB 크기이며 네트워크 환경에 따라 시간이 약간 소요될 수 있습니다.)
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%ZIP_URL%' -OutFile '%TARGET_DIR%\%ZIP_FILE%'"
if %errorlevel% neq 0 (
    echo [ERROR] 다운로드 중 오류가 발생했습니다. 네트워크 연결을 확인하세요.
    pause
    exit /b 1
)

:: 3. Extract ZIP file using PowerShell
echo [2/3] 다운로드된 압축 패키지 압축 해제 중...
powershell -Command "Expand-Archive -Path '%TARGET_DIR%\%ZIP_FILE%' -DestinationPath '%TARGET_DIR%' -Force"
if %errorlevel% neq 0 (
    echo [ERROR] 압축 해제 중 오류가 발생했습니다.
    pause
    exit /b 1
)
del /q "%TARGET_DIR%\%ZIP_FILE%"

:: 4. Execute test_e2e.exe
echo [3/3] 사전 빌드된 바이너리를 기동하여 G2P 파이프라인 즉시 테스트 실행:
echo --------------------------------------------------------
echo [입력 문장]: 彼女は料理が上手だ。
echo.
echo 彼女は料理が上手だ。 | "%TARGET_DIR%\test_e2e.exe" "%TARGET_DIR%" ja
echo --------------------------------------------------------
echo.
echo 테스트 실행이 완료되었습니다.
pause
