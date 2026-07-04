@echo off
setlocal
cd /d "%~dp0"

echo [1/2] 安装依赖 ...
python -m pip install -r requirements.txt -q
if errorlevel 1 (
    echo 请先安装 Python 3.10+ 并确保 python 命令可用。
    pause
    exit /b 1
)

echo [2/2] 启动 MoodCanvas 云端服务 http://0.0.0.0:8765
python -m uvicorn main:app --host 0.0.0.0 --port 8765
