-- List query indexes.
--
-- Every list endpoint filters on `status = 1`, optionally narrows by
-- course/chapter/owner, and orders by an explicit column plus `id`. Only
-- (status, sort_order) existed on `course`, so the remaining tables fell back
-- to a full scan plus filesort for their default listing. These indexes are
-- deliberately additive: nothing is dropped, so the migration cannot break an
-- existing foreign key's supporting index. The narrower indexes that the new
-- ones fully cover (idx_question_course, idx_question_chapter,
-- idx_exam_paper_status, idx_knowledge_course, idx_question_attempt_user_time,
-- idx_wrong_question_user_status_time) can be dropped later, after confirming
-- with EXPLAIN that the wider index is chosen.

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'question' AND INDEX_NAME = 'idx_question_status_sort'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE question ADD INDEX idx_question_status_sort (status, sort_order, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'question' AND INDEX_NAME = 'idx_question_course_status_sort'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE question ADD INDEX idx_question_course_status_sort (course_id, status, sort_order, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'question' AND INDEX_NAME = 'idx_question_chapter_status_sort'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE question ADD INDEX idx_question_chapter_status_sort (chapter_id, status, sort_order, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'exam_paper' AND INDEX_NAME = 'idx_exam_paper_status_visibility_year'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE exam_paper ADD INDEX idx_exam_paper_status_visibility_year (status, visibility, exam_year, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'exam_paper' AND INDEX_NAME = 'idx_exam_paper_owner_status_year'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE exam_paper ADD INDEX idx_exam_paper_owner_status_year (owner_user_id, status, exam_year, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'knowledge_point' AND INDEX_NAME = 'idx_knowledge_status_sort'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE knowledge_point ADD INDEX idx_knowledge_status_sort (status, sort_order, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'knowledge_point' AND INDEX_NAME = 'idx_knowledge_course_status_sort'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE knowledge_point ADD INDEX idx_knowledge_course_status_sort (course_id, status, sort_order, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'chapter' AND INDEX_NAME = 'idx_chapter_status_sort'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE chapter ADD INDEX idx_chapter_status_sort (status, sort_order, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'question_attempt' AND INDEX_NAME = 'idx_question_attempt_user_time_id'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE question_attempt ADD INDEX idx_question_attempt_user_time_id (user_id, last_attempt_at, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;

SET @index_exists = (
  SELECT COUNT(*) FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'wrong_question' AND INDEX_NAME = 'idx_wrong_question_user_status_added_id'
);
SET @migration_sql = IF(
  @index_exists = 0,
  'ALTER TABLE wrong_question ADD INDEX idx_wrong_question_user_status_added_id (user_id, status, added_at, id)',
  'SELECT 1'
);
PREPARE migration_statement FROM @migration_sql;
EXECUTE migration_statement;
DEALLOCATE PREPARE migration_statement;
