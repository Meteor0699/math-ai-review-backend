-- V004: agent 模式会话持久化（见 docs/AGENT_MODE.md 第 2 节）
-- 幂等：CREATE TABLE 前查 information_schema，表已存在时跳过；
-- 不写死 USE，库名由连接参数 / 调用方上下文决定（与 V002 的 DATABASE() 用法一致）。
-- 该文件通过 scripts/migrate-db.sh 的 `mysql < file`（batch）方式执行，
-- 因此使用无 DELIMITER 的命令式建表方案，避免客户端 DELIMITER 差异。

SET @conversation_table_exists = (
  SELECT COUNT(*) FROM information_schema.TABLES
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_conversation'
);
SET @migration_sql = IF(
  @conversation_table_exists = 0,
  'CREATE TABLE ai_conversation (
     id BIGINT NOT NULL AUTO_INCREMENT,
     user_id BIGINT NOT NULL,
     question_id BIGINT NOT NULL,
     message_count INT NOT NULL DEFAULT 0,
     created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
     updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
     PRIMARY KEY (id),
     UNIQUE KEY uk_ai_conversation_user_question (user_id, question_id),
     KEY idx_ai_conversation_user_updated (user_id, updated_at),
     CONSTRAINT fk_ai_conversation_user FOREIGN KEY (user_id) REFERENCES user (id) ON DELETE CASCADE ON UPDATE CASCADE,
     CONSTRAINT fk_ai_conversation_question FOREIGN KEY (question_id) REFERENCES question (id) ON DELETE CASCADE ON UPDATE CASCADE
   ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @message_table_exists = (
  SELECT COUNT(*) FROM information_schema.TABLES
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'ai_message'
);
SET @migration_sql = IF(
  @message_table_exists = 0,
  'CREATE TABLE ai_message (
     id BIGINT NOT NULL AUTO_INCREMENT,
     conversation_id BIGINT NOT NULL,
     role ENUM(''user'',''assistant'',''tool'') NOT NULL,
     content MEDIUMTEXT DEFAULT NULL,
     tool_name VARCHAR(64) DEFAULT NULL,
     tool_args TEXT DEFAULT NULL,
     tool_result MEDIUMTEXT DEFAULT NULL,
     created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
     PRIMARY KEY (id),
     KEY idx_ai_message_conversation (conversation_id, id),
     CONSTRAINT fk_ai_message_conversation FOREIGN KEY (conversation_id) REFERENCES ai_conversation (id) ON DELETE CASCADE ON UPDATE CASCADE
   ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;
