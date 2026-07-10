# Contributing

## Development workflow

1. Create a focused branch from the latest `main`.
2. Keep changes limited to one behavior or engineering concern.
3. Never commit `.env` files, credentials, uploaded papers, database exports, or build output.
4. Add or update tests when behavior changes.
5. Run the checks below before opening a pull request.

```powershell
cmake --build build -j2
ctest --test-dir build --output-on-failure
cd frontend
npm.cmd test
npm.cmd run build
```

Describe the user-visible behavior, database impact, security impact, and verification performed in the pull request.
