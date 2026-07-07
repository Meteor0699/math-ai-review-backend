param(
    [string]$Message = "",
    [string]$Remote = "origin",
    [string]$Branch = "",
    [string]$Username = "",
    [switch]$UseTokenPrompt,
    [switch]$NoVerify,
    [switch]$AllowEmpty
)

$ErrorActionPreference = "Stop"

function Invoke-Git {
    param([Parameter(ValueFromRemainingArguments = $true)][string[]]$Args)
    & git @Args
    if ($LASTEXITCODE -ne 0) {
        throw "git $($Args -join ' ') failed with exit code $LASTEXITCODE"
    }
}

function ConvertFrom-SecureStringToPlainText {
    param([System.Security.SecureString]$SecureValue)

    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($SecureValue)
    try {
        return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    }
    finally {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
    }
}

function Invoke-GitPush {
    param(
        [string]$RemoteName,
        [string]$BranchName,
        [string]$GitHubUsername,
        [switch]$PromptForToken
    )

    if (-not $PromptForToken) {
        Invoke-Git push $RemoteName $BranchName
        return
    }

    if (-not $GitHubUsername) {
        $GitHubUsername = Read-Host "GitHub username"
    }
    if (-not $GitHubUsername) {
        throw "GitHub username is required."
    }

    $secureToken = Read-Host "GitHub Personal Access Token" -AsSecureString
    $token = ConvertFrom-SecureStringToPlainText $secureToken
    if (-not $token) {
        throw "GitHub token is required."
    }

    $tempDir = Join-Path ([System.IO.Path]::GetTempPath()) ("math-ai-review-git-" + [Guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempDir | Out-Null
    $askpassPs1 = Join-Path $tempDir "askpass.ps1"
    $askpassCmd = Join-Path $tempDir "askpass.cmd"

    Set-Content -Path $askpassPs1 -Encoding ASCII -Value @'
param([string]$Prompt)
if ($Prompt -match "Username") {
    [Console]::Out.Write([Environment]::GetEnvironmentVariable("GIT_PUSH_USERNAME", "Process"))
}
else {
    [Console]::Out.Write([Environment]::GetEnvironmentVariable("GIT_PUSH_TOKEN", "Process"))
}
'@
    Set-Content -Path $askpassCmd -Encoding ASCII -Value '@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0askpass.ps1" %*
'

    $oldAskPass = $env:GIT_ASKPASS
    $oldTerminalPrompt = $env:GIT_TERMINAL_PROMPT
    $oldUsername = $env:GIT_PUSH_USERNAME
    $oldToken = $env:GIT_PUSH_TOKEN

    try {
        $env:GIT_ASKPASS = $askpassCmd
        $env:GIT_TERMINAL_PROMPT = "0"
        $env:GIT_PUSH_USERNAME = $GitHubUsername
        $env:GIT_PUSH_TOKEN = $token
        Invoke-Git push $RemoteName $BranchName
    }
    finally {
        $env:GIT_ASKPASS = $oldAskPass
        $env:GIT_TERMINAL_PROMPT = $oldTerminalPrompt
        $env:GIT_PUSH_USERNAME = $oldUsername
        $env:GIT_PUSH_TOKEN = $oldToken
        Remove-Item -LiteralPath $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    }
}

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $projectRoot

Invoke-Git rev-parse --is-inside-work-tree | Out-Null

if (-not $Branch) {
    $branchOutput = & git branch --show-current
    $Branch = if ($null -eq $branchOutput) { "" } else { $branchOutput.ToString().Trim() }
    if (-not $Branch) {
        throw "Cannot detect current branch. Please pass -Branch explicitly."
    }
}

$trackedEnvOutput = & git ls-files ".env"
$trackedEnv = if ($null -eq $trackedEnvOutput) { "" } else { $trackedEnvOutput.ToString().Trim() }
if ($trackedEnv) {
    throw ".env is tracked by git. Remove it from git before pushing secrets."
}

$status = @(& git status --porcelain)
$hasChanges = $status.Count -gt 0

if ($hasChanges) {
    $blocked = $status | Where-Object {
        $_ -match '^\?\?\s+\.env$' -or
        $_ -match '^\?\?\s+.*\.env$' -or
        $_ -match '^[ MADRCU?!]{2}\s+\.env$' -or
        $_ -match '^[ MADRCU?!]{2}\s+.*\.env$'
    }
    if ($blocked) {
        throw "Refusing to add .env files. Check .gitignore and remove secrets from the working tree."
    }

    Write-Host "Changed files:"
    & git status --short

    if (-not $Message) {
        $Message = Read-Host "Commit message"
    }
    if (-not $Message) {
        throw "Commit message is required when there are local changes."
    }

    Invoke-Git add --all

    $stagedEnv = (& git diff --cached --name-only) | Where-Object {
        $_ -eq ".env" -or $_ -like "*.env"
    }
    if ($stagedEnv) {
        Invoke-Git reset HEAD -- $stagedEnv
        throw "Refusing to commit env file(s): $($stagedEnv -join ', ')"
    }

    $commitArgs = @("commit", "-m", $Message)
    if ($NoVerify) {
        $commitArgs += "--no-verify"
    }
    Invoke-Git @commitArgs
}
elseif ($AllowEmpty) {
    if (-not $Message) {
        $Message = Read-Host "Empty commit message"
    }
    if (-not $Message) {
        throw "Commit message is required for an empty commit."
    }
    Invoke-Git commit --allow-empty -m $Message
}
else {
    Write-Host "No local file changes to commit."
}

Write-Host "Pushing $Branch to $Remote..."
Invoke-GitPush -RemoteName $Remote -BranchName $Branch -GitHubUsername $Username -PromptForToken:$UseTokenPrompt

Write-Host "Done. Latest commit:"
Invoke-Git log -1 --oneline
