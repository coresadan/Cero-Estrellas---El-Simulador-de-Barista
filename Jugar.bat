@echo off
setlocal
if exist "%~dp0Release\Windows\CeroEstrellas.exe" (
    start "" "%~dp0Release\Windows\CeroEstrellas.exe" -windowed -ResX=1280 -ResY=720
) else (
    start "" "F:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0CeroEstrellas.uproject" -game -windowed -ResX=1280 -ResY=720 -NoSplash
)
