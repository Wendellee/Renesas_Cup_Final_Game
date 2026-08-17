@echo off
setlocal
pushd "%~dp0web"

where py >nul 2>&1
if %errorlevel% equ 0 (
  set "REPORT_PYTHON=py"
) else (
  where python >nul 2>&1
  if errorlevel 1 (
    echo Python was not found. Install Python or start any static HTTP server in report\web.
    pause
    exit /b 1
  )
  set "REPORT_PYTHON=python"
)

start "RA8P1 Report Server" cmd /k "%REPORT_PYTHON% -m http.server 8124 --bind 127.0.0.1"
timeout /t 2 /nobreak >nul
start "" "http://127.0.0.1:8124/"
popd
endlocal
