# Security Policy

## Reporting a vulnerability

Do not disclose a suspected vulnerability in a public issue. Contact the repository owner privately with:

- the affected endpoint or file;
- reproduction steps;
- the expected and observed impact;
- any proposed mitigation.

Do not include production credentials, API keys, access tokens, database exports, or private student files in a report.

## Supported version

Security fixes are applied to the current `main` branch. Deployments should track a reviewed commit and rotate any credential that may have been exposed.

## Response process

The maintainer should acknowledge the report, reproduce it in a non-production environment, prioritize a fix by impact, add a regression test, and disclose the issue only after affected deployments have been updated.
