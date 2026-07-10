#!/usr/bin/env bash
set -euo pipefail

/app/scripts/migrate-db.sh
exec /app/math-ai-review-backend
