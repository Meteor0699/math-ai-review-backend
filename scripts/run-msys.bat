@echo off
setlocal
set PATH=D:\msys64\ucrt64\bin;D:\msys64\usr\bin;%PATH%
cd /d "D:\solo proj\math-ai-review-backend"
if not exist build-msys\tmp mkdir build-msys\tmp
set TMPDIR=D:\solo proj\math-ai-review-backend\build-msys\tmp
set TEMP=D:\solo proj\math-ai-review-backend\build-msys\tmp
set TMP=D:\solo proj\math-ai-review-backend\build-msys\tmp
build-msys\math-ai-review-backend.exe
endlocal
