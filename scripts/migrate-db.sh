#!/usr/bin/env bash
set -euo pipefail

for name in DB_HOST DB_PORT DB_NAME DB_USER DB_PASSWORD; do
  if [[ -z "${!name:-}" ]]; then
    echo "Missing required database setting: ${name}" >&2
    exit 1
  fi
done

export MYSQL_PWD="${DB_PASSWORD}"
mysql_args=(--protocol=TCP --host="${DB_HOST}" --port="${DB_PORT}" --user="${DB_USER}" "${DB_NAME}")
if [[ "${DB_SSL_MODE:-DISABLED}" == "REQUIRED" ]]; then
  mysql_args+=(--ssl)
fi

mysql "${mysql_args[@]}" --batch --skip-column-names <<'SQL'
CREATE TABLE IF NOT EXISTS schema_migrations (
  version VARCHAR(128) NOT NULL,
  checksum CHAR(64) NOT NULL,
  applied_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (version)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
SQL

command_file="$(mktemp)"
trap 'rm -f "${command_file}"' EXIT
printf "SELECT GET_LOCK('math_ai_schema_migration', 60);\n" > "${command_file}"

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
migrations_dir="${MIGRATIONS_DIR:-${script_dir}/../database/migrations}"
for migration in "${migrations_dir}"/V*.sql; do
  version="$(basename "${migration}" .sql)"
  checksum="$(sha256sum "${migration}" | awk '{print $1}')"
  applied="$(mysql "${mysql_args[@]}" --batch --skip-column-names \
    --execute="SELECT checksum FROM schema_migrations WHERE version = '${version}' LIMIT 1")"
  if [[ -n "${applied}" && "${applied}" != "${checksum}" ]]; then
    echo "Migration checksum mismatch: ${version}" >&2
    exit 1
  fi
  [[ -n "${applied}" ]] && continue
  printf "SOURCE %s;\n" "${migration}" >> "${command_file}"
  printf "INSERT INTO schema_migrations (version, checksum) VALUES ('%s', '%s') ON DUPLICATE KEY UPDATE checksum = VALUES(checksum);\n" \
    "${version}" "${checksum}" >> "${command_file}"
done

printf "SELECT RELEASE_LOCK('math_ai_schema_migration');\n" >> "${command_file}"
mysql "${mysql_args[@]}" < "${command_file}"
