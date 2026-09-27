@echo off
setlocal
title Project Astra Cosmos
cd /d "%~dp0"

if not exist "astra.exe" (
  echo.
  echo [Astra Cosmos] astra.exe not found next to .start.bat.
  echo [Astra Cosmos] The Phase 0 build output must be in this folder:
  echo [Astra Cosmos]   astra.exe
  echo [Astra Cosmos]   shaders\triangle.vert.spv
  echo [Astra Cosmos]   shaders\triangle.frag.spv
  echo [Astra Cosmos] See README.md (section "Build") for how to produce it.
  echo.
  pause
  exit /b 1
)

echo [Astra Cosmos] launching astra.exe --cpu-priority=high ...
astra.exe --cpu-priority=high

endlocal
