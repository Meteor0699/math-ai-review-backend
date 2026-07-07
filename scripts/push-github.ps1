param(
    [string]$Message = "",
    [string]$Remote = "origin",
    [string]$Branch = "",
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

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $projectRoot

Invoke-Git rev-parse --is-inside-work-tree | Out-Null

if (-not $Branch) {
    $Branch = (& git branch --show-current).Trim()
    if (-not $Branch) {
        throw "Cannot detect current branch. Please pass -Branch explicitly."
    }
}

$trackedEnv = (& git ls-files ".env").Trim()
if ($trackedEnv) {
    throw ".env is tracked by git. Remove it from git before pushing secrets."
}

$status = & git status --porcelain
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
Invoke-Git push $Remote $Branch

Write-Host "Done. Latest commit:"
Invoke-Git log -1 --oneline
