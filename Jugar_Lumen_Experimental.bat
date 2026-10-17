@echo off
setlocal
if exist "%~dp0Tools\LocalAI\Python\Scripts\python.exe" if exist "%~dp0Tools\LocalAI\Ollama\ollama.exe" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\start_ai.ps1"
)
start "" "%~dp0Release\Windows\CeroEstrellas.exe" -dx12 -sm6 -CeroLumen -ini:Engine:[ConsoleVariables]:r.PSOPrecache.GlobalShaders=0 -windowed -ResX=1920 -ResY=1080 -NoSplash
