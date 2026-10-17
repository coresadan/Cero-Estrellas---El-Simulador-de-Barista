@echo off
setlocal
if exist "%~dp0Tools\LocalAI\Python\Scripts\python.exe" if exist "%~dp0Tools\LocalAI\Ollama\ollama.exe" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\start_ai.ps1"
)
if exist "%~dp0Release\Windows\CeroEstrellas.exe" (
    start "" "%~dp0Release\Windows\CeroEstrellas.exe" -dx11 -windowed -ResX=1920 -ResY=1080
) else (
    start "" "F:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0CeroEstrellas.uproject" -game -dx11 -windowed -ResX=1920 -ResY=1080 -NoSplash
)
