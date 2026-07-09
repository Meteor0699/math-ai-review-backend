-- 学生个人试卷库迁移脚本。
-- 使用方式：mysql -u 用户名 -p math_ai_review < database/add_student_paper_library.sql

USE math_ai_review;

ALTER TABLE exam_paper
  ADD COLUMN owner_user_id BIGINT DEFAULT NULL COMMENT '上传用户ID，管理员导入或系统内置试卷可为空' AFTER file_type,
  ADD COLUMN visibility ENUM('public', 'private') NOT NULL DEFAULT 'public' COMMENT '可见性：public公开，private仅上传者可见' AFTER owner_user_id,
  ADD COLUMN ai_review_status ENUM('not_required', 'approved') NOT NULL DEFAULT 'not_required' COMMENT 'AI审核状态：not_required无需审核，approved审核通过' AFTER visibility,
  ADD COLUMN ai_review_comment TEXT DEFAULT NULL COMMENT 'AI审核说明' AFTER ai_review_status,
  ADD KEY idx_exam_paper_owner_visibility (owner_user_id, visibility),
  ADD CONSTRAINT fk_exam_paper_owner FOREIGN KEY (owner_user_id) REFERENCES user (id)
    ON DELETE RESTRICT ON UPDATE CASCADE;
