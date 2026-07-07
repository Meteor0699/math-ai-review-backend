@echo off
setlocal
if "%~1"=="" (
  echo Usage: scripts\init-db.bat MYSQL_PASSWORD [MYSQL_USER]
  exit /b 1
)
set MYSQL_PASSWORD=%~1
set MYSQL_USER=%~2
if "%MYSQL_USER%"=="" set MYSQL_USER=root
set SCHEMA=%~dp0..\database\schema.sql
D:\msys64\ucrt64\bin\mariadb.exe --host=127.0.0.1 --port=3306 --user=%MYSQL_USER% --password=%MYSQL_PASSWORD% --ssl=0 --default-character-set=utf8mb4 < "%SCHEMA%"
endlocal
