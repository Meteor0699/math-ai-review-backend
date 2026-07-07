@echo off
setlocal
set PATH=D:\msys64\ucrt64\bin;D:\msys64\usr\bin;%PATH%
D:\msys64\usr\bin\bash.exe -lc "cd '/d/solo proj/math-ai-review-backend' && mkdir -p build-msys/tmp && export HOME=\"$PWD/build-msys/tmp\" TMPDIR=\"$PWD/build-msys/tmp\" TEMP=\"$PWD/build-msys/tmp\" TMP=\"$PWD/build-msys/tmp\" && export PATH=/ucrt64/bin:/usr/bin:$PATH && cmake -S . -B build-msys -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMYSQL_LIBRARIES=/ucrt64/lib/libmariadb.dll.a && cmake --build build-msys -j1"
endlocal
