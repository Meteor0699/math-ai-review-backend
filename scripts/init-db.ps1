$ErrorActionPreference = "Stop"

param(
    [string]$User = "root",
    [Parameter(Mandatory = $true)]
    [string]$Password,
    [string]$HostName = "127.0.0.1",
    [int]$Port = 3306
)

$projectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$schema = Join-Path $projectRoot "database\schema.sql"
$mariadb = "D:\msys64\ucrt64\bin\mariadb.exe"

if (-not (Test-Path $mariadb)) {
    throw "MariaDB client not found: $mariadb"
}

if (-not (Test-Path $schema)) {
    throw "schema.sql not found: $schema"
}

Get-Content -Raw -Encoding UTF8 $schema |
    & $mariadb --host=$HostName --port=$Port --user=$User --password=$Password --ssl=0 --default-character-set=utf8mb4
