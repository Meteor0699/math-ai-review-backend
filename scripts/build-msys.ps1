$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$bash = "D:\msys64\usr\bin\bash.exe"

if (-not (Test-Path $bash)) {
    throw "MSYS2 bash not found: $bash"
}

$buildDir = Join-Path $projectRoot "build-msys"

# Copy MariaDB DLL from MSYS2 to build directory for runtime
$mariadbDll = "D:\msys64\ucrt64\bin\libmariadb.dll"
if (Test-Path $mariadbDll) {
    Copy-Item $mariadbDll $buildDir -Force
    Write-Host "Copied libmariadb.dll to build directory"
}

& $bash -lc "cd '/d/solo proj/math-ai-review-backend' && mkdir -p build-msys/tmp && export HOME=`"`$PWD/build-msys/tmp`" TMPDIR=`"`$PWD/build-msys/tmp`" TEMP=`"`$PWD/build-msys/tmp`" TMP=`"`$PWD/build-msys/tmp`" && export PATH=/ucrt64/bin:/usr/bin:`$PATH && cmake -S . -B build-msys -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMYSQL_LIBRARIES='/ucrt64/lib/libmariadb.dll.a' && cmake --build build-msys -j1"

Write-Host "Build complete: $projectRoot\build-msys\math-ai-review-backend.exe"