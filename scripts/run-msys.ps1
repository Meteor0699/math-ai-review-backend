$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$exe = Join-Path $projectRoot "build-msys\math-ai-review-backend.exe"

if (-not (Test-Path $exe)) {
    throw "Backend executable not found. Run scripts\build-msys.ps1 first."
}

$env:Path = "D:\msys64\ucrt64\bin;D:\msys64\usr\bin;$env:Path"
Set-Location $projectRoot
if (Test-Path ".env") {
    Get-Content ".env" | ForEach-Object {
        $line = $_.Trim()
        if ($line -and -not $line.StartsWith("#")) {
            $pair = $line.Split("=", 2)
            if ($pair.Length -eq 2) {
                [Environment]::SetEnvironmentVariable($pair[0], $pair[1], "Process")
            }
        }
    }
}
& $exe
