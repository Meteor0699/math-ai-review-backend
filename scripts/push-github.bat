@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0push-github.ps1" %*
