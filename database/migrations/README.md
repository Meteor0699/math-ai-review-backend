# Production database migrations

Files are applied in lexical order by `scripts/migrate-db.sh`. Applied versions and SHA-256 checksums are stored in `schema_migrations`.

- Never use `DROP DATABASE`, `DROP TABLE`, or a fixed `USE` statement.
- Make every migration safe to execute twice.
- Never edit a migration after it reaches a shared environment.
- Add a new numbered file for every change.
- Back up production data before schema changes.

`V001` adopts an existing database and fails if the initial business schema is absent. Initialize a brand-new empty database once before enabling automatic migrations.
