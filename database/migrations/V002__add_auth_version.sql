SET @column_exists = (
  SELECT COUNT(*) FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'user' AND COLUMN_NAME = 'auth_version'
);
SET @migration_sql = IF(
  @column_exists = 0,
  'ALTER TABLE user ADD COLUMN auth_version INT NOT NULL DEFAULT 1 AFTER status',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;
