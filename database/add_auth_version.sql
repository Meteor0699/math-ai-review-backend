-- Add a version to user authentication state so password and permission
-- changes can invalidate previously issued JWTs.
SET @column_exists = (
  SELECT COUNT(*)
  FROM information_schema.COLUMNS
  WHERE TABLE_SCHEMA = DATABASE()
    AND TABLE_NAME = 'user'
    AND COLUMN_NAME = 'auth_version'
);
SET @migration_sql = IF(
  @column_exists = 0,
  'ALTER TABLE user ADD COLUMN auth_version INT NOT NULL DEFAULT 1 COMMENT ''鉴权版本，密码或权限变化时递增以使旧Token失效'' AFTER status',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;
