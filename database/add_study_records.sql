CREATE TABLE IF NOT EXISTS question_attempt (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '做题记录ID',
  user_id BIGINT NOT NULL COMMENT '用户ID',
  question_id BIGINT NOT NULL COMMENT '题目ID',
  status ENUM('viewed', 'mastered', 'wrong') NOT NULL DEFAULT 'viewed' COMMENT '学习状态：viewed已查看，mastered已掌握，wrong标记错误',
  note TEXT DEFAULT NULL COMMENT '学生备注',
  attempt_count INT NOT NULL DEFAULT 1 COMMENT '记录次数',
  first_viewed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '首次查看时间',
  last_attempt_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '最近记录时间',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  UNIQUE KEY uk_question_attempt_user_question (user_id, question_id),
  KEY idx_question_attempt_user_time (user_id, last_attempt_at),
  KEY idx_question_attempt_question (question_id),
  CONSTRAINT fk_question_attempt_user
    FOREIGN KEY (user_id) REFERENCES user (id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_question_attempt_question
    FOREIGN KEY (question_id) REFERENCES question (id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='做题记录表';

CREATE TABLE IF NOT EXISTS wrong_question (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '错题本ID',
  user_id BIGINT NOT NULL COMMENT '用户ID',
  question_id BIGINT NOT NULL COMMENT '题目ID',
  note TEXT DEFAULT NULL COMMENT '错题备注',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '状态：0已移除，1有效',
  added_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '加入错题本时间',
  removed_at DATETIME DEFAULT NULL COMMENT '移出错题本时间',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  UNIQUE KEY uk_wrong_question_user_question (user_id, question_id),
  KEY idx_wrong_question_user_status_time (user_id, status, added_at),
  KEY idx_wrong_question_question (question_id),
  CONSTRAINT fk_wrong_question_user
    FOREIGN KEY (user_id) REFERENCES user (id)
    ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT fk_wrong_question_question
    FOREIGN KEY (question_id) REFERENCES question (id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='错题本表';
