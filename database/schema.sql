-- schema.sql
-- MySQL 8.0 schema for 高校数学课程 AI 辅助复习 Web 平台

CREATE DATABASE IF NOT EXISTS math_ai_review
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

USE math_ai_review;

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

DROP TABLE IF EXISTS ai_explanation;
DROP TABLE IF EXISTS question_option;
DROP TABLE IF EXISTS question;
DROP TABLE IF EXISTS exam_paper;
DROP TABLE IF EXISTS knowledge_point;
DROP TABLE IF EXISTS chapter;
DROP TABLE IF EXISTS course;
DROP TABLE IF EXISTS user;

SET FOREIGN_KEY_CHECKS = 1;

CREATE TABLE user (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '用户ID',
  username VARCHAR(64) NOT NULL COMMENT '登录用户名',
  password_hash VARCHAR(255) NOT NULL COMMENT '密码哈希值',
  real_name VARCHAR(64) DEFAULT NULL COMMENT '真实姓名',
  student_no VARCHAR(64) DEFAULT NULL COMMENT '学号，管理员可为空',
  role ENUM('student', 'admin') NOT NULL DEFAULT 'student' COMMENT '用户角色：student学生，admin管理员',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '用户状态：0禁用，1启用',
  last_login_at DATETIME DEFAULT NULL COMMENT '最后登录时间',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  UNIQUE KEY uk_user_username (username),
  UNIQUE KEY uk_user_student_no (student_no),
  KEY idx_user_role (role),
  KEY idx_user_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

CREATE TABLE course (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '课程ID',
  name VARCHAR(100) NOT NULL COMMENT '课程名称',
  code VARCHAR(50) NOT NULL COMMENT '课程编码',
  description TEXT DEFAULT NULL COMMENT '课程简介',
  cover_url VARCHAR(500) DEFAULT NULL COMMENT '课程封面地址',
  sort_order INT NOT NULL DEFAULT 0 COMMENT '排序值，越小越靠前',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '课程状态：0下架，1正常',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  UNIQUE KEY uk_course_code (code),
  KEY idx_course_status_sort (status, sort_order)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='课程表';

CREATE TABLE chapter (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '章节ID',
  course_id BIGINT NOT NULL COMMENT '所属课程ID',
  parent_id BIGINT DEFAULT NULL COMMENT '父章节ID，第一版可为空',
  title VARCHAR(150) NOT NULL COMMENT '章节标题',
  description TEXT DEFAULT NULL COMMENT '章节说明',
  sort_order INT NOT NULL DEFAULT 0 COMMENT '排序值，越小越靠前',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '章节状态：0隐藏，1正常',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  KEY idx_chapter_course_sort (course_id, sort_order),
  KEY idx_chapter_parent (parent_id),
  CONSTRAINT fk_chapter_course
    FOREIGN KEY (course_id) REFERENCES course (id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  CONSTRAINT fk_chapter_parent
    FOREIGN KEY (parent_id) REFERENCES chapter (id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='章节表';

CREATE TABLE knowledge_point (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '重点知识ID',
  course_id BIGINT NOT NULL COMMENT '所属课程ID',
  chapter_id BIGINT NOT NULL COMMENT '所属章节ID',
  title VARCHAR(150) NOT NULL COMMENT '知识点标题',
  core_content MEDIUMTEXT NOT NULL COMMENT '核心内容说明',
  formulas MEDIUMTEXT DEFAULT NULL COMMENT '重要公式、定理或结论',
  typical_questions MEDIUMTEXT DEFAULT NULL COMMENT '典型题型与常见考法',
  common_mistakes MEDIUMTEXT DEFAULT NULL COMMENT '常见错误与易混点',
  exam_frequency ENUM('low', 'medium', 'high') NOT NULL DEFAULT 'medium' COMMENT '考试频率：low低频，medium中频，high高频',
  review_advice MEDIUMTEXT DEFAULT NULL COMMENT '复习建议',
  sort_order INT NOT NULL DEFAULT 0 COMMENT '排序值，越小越靠前',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '知识点状态：0隐藏，1正常',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  KEY idx_knowledge_course (course_id),
  KEY idx_knowledge_chapter_sort (chapter_id, sort_order),
  CONSTRAINT fk_knowledge_course
    FOREIGN KEY (course_id) REFERENCES course (id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  CONSTRAINT fk_knowledge_chapter
    FOREIGN KEY (chapter_id) REFERENCES chapter (id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='重点知识表';

CREATE TABLE exam_paper (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '往年试卷ID',
  course_id BIGINT NOT NULL COMMENT '所属课程ID',
  title VARCHAR(255) NOT NULL COMMENT '试卷标题',
  exam_year YEAR DEFAULT NULL COMMENT '试卷年份',
  semester VARCHAR(50) DEFAULT NULL COMMENT '学期信息',
  original_filename VARCHAR(255) NOT NULL COMMENT '原始文件名',
  stored_filename VARCHAR(255) NOT NULL COMMENT '服务器保存文件名',
  file_path VARCHAR(500) NOT NULL COMMENT '服务器本地文件路径',
  file_size BIGINT NOT NULL DEFAULT 0 COMMENT '文件大小，单位字节',
  file_type VARCHAR(50) NOT NULL COMMENT '文件类型或扩展名',
  split_status ENUM('not_split', 'splitting', 'split_done') NOT NULL DEFAULT 'not_split' COMMENT '拆题状态：not_split未拆题，splitting拆题中，split_done已拆题',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '试卷状态：0禁用，1正常',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  KEY idx_exam_paper_course_year (course_id, exam_year),
  KEY idx_exam_paper_status (status),
  CONSTRAINT fk_exam_paper_course
    FOREIGN KEY (course_id) REFERENCES course (id)
    ON DELETE RESTRICT ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='往年试卷表';

CREATE TABLE question (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '题目ID',
  course_id BIGINT NOT NULL COMMENT '所属课程ID',
  chapter_id BIGINT DEFAULT NULL COMMENT '所属章节ID',
  knowledge_point_id BIGINT DEFAULT NULL COMMENT '关联重点知识ID',
  exam_paper_id BIGINT DEFAULT NULL COMMENT '来源往年试卷ID',
  title VARCHAR(255) DEFAULT NULL COMMENT '题目标题',
  content MEDIUMTEXT NOT NULL COMMENT '题干内容',
  question_type ENUM('single_choice', 'blank', 'calculation', 'solution', 'proof') NOT NULL COMMENT '题型：single_choice单选题，blank填空题，calculation计算题，solution解答题，proof证明题',
  difficulty ENUM('basic', 'medium', 'hard') NOT NULL DEFAULT 'medium' COMMENT '难度：basic基础，medium中等，hard较难',
  standard_answer MEDIUMTEXT NOT NULL COMMENT '标准答案',
  normal_explanation MEDIUMTEXT DEFAULT NULL COMMENT '普通解析',
  source VARCHAR(255) DEFAULT NULL COMMENT '题目来源',
  source_year YEAR DEFAULT NULL COMMENT '题目来源年份',
  from_exam_paper TINYINT NOT NULL DEFAULT 0 COMMENT '是否来自往年试题：0否，1是',
  paper_question_no VARCHAR(50) DEFAULT NULL COMMENT '在试卷中的题号',
  sort_order INT NOT NULL DEFAULT 0 COMMENT '排序值，越小越靠前',
  status TINYINT NOT NULL DEFAULT 1 COMMENT '题目状态：0隐藏，1正常',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  KEY idx_question_course (course_id),
  KEY idx_question_chapter (chapter_id),
  KEY idx_question_knowledge (knowledge_point_id),
  KEY idx_question_exam_paper (exam_paper_id),
  KEY idx_question_type_difficulty (question_type, difficulty),
  KEY idx_question_source_year (source_year),
  KEY idx_question_from_exam_paper (from_exam_paper),
  CONSTRAINT fk_question_course
    FOREIGN KEY (course_id) REFERENCES course (id)
    ON DELETE RESTRICT ON UPDATE CASCADE,
  CONSTRAINT fk_question_chapter
    FOREIGN KEY (chapter_id) REFERENCES chapter (id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_question_knowledge
    FOREIGN KEY (knowledge_point_id) REFERENCES knowledge_point (id)
    ON DELETE SET NULL ON UPDATE CASCADE,
  CONSTRAINT fk_question_exam_paper
    FOREIGN KEY (exam_paper_id) REFERENCES exam_paper (id)
    ON DELETE SET NULL ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='题目表';

CREATE TABLE question_option (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT '选择题选项ID',
  question_id BIGINT NOT NULL COMMENT '所属题目ID',
  option_label VARCHAR(10) NOT NULL COMMENT '选项标识，如A、B、C、D',
  option_content MEDIUMTEXT NOT NULL COMMENT '选项内容',
  is_correct TINYINT NOT NULL DEFAULT 0 COMMENT '是否正确选项：0否，1是',
  sort_order INT NOT NULL DEFAULT 0 COMMENT '排序值，越小越靠前',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  UNIQUE KEY uk_question_option_label (question_id, option_label),
  KEY idx_question_option_question_sort (question_id, sort_order),
  CONSTRAINT fk_question_option_question
    FOREIGN KEY (question_id) REFERENCES question (id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='选择题选项表';

CREATE TABLE ai_explanation (
  id BIGINT NOT NULL AUTO_INCREMENT COMMENT 'AI解析记录ID',
  question_id BIGINT NOT NULL COMMENT '题目ID',
  model_name VARCHAR(100) DEFAULT NULL COMMENT '调用的大模型名称',
  prompt MEDIUMTEXT DEFAULT NULL COMMENT '发送给AI的提示词',
  explanation MEDIUMTEXT NOT NULL COMMENT 'AI生成的题目解析',
  status ENUM('success', 'failed') NOT NULL DEFAULT 'success' COMMENT '解析状态：success成功，failed失败',
  error_message TEXT DEFAULT NULL COMMENT '失败错误信息',
  generated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'AI解析生成时间',
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
  PRIMARY KEY (id),
  UNIQUE KEY uk_ai_explanation_question (question_id),
  KEY idx_ai_explanation_status (status),
  KEY idx_ai_explanation_generated_at (generated_at),
  CONSTRAINT fk_ai_explanation_question
    FOREIGN KEY (question_id) REFERENCES question (id)
    ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='AI解析记录表';

INSERT INTO user (id, username, password_hash, real_name, student_no, role, status) VALUES
(1, 'admin', 'sha256$math-ai-review$b9ef3da9ebe71810e3c5d67b311bf23ce15f70646de275e131db945ed7ff0555', '管理员', NULL, 'admin', 1);

INSERT INTO course (id, name, code, description, sort_order, status) VALUES
(1, '高等数学', 'advanced_math', '面向高校在校生的高等数学复习课程，覆盖函数、极限、微积分、级数、微分方程等核心内容。', 1, 1),
(2, '线性代数', 'linear_algebra', '面向高校在校生的线性代数复习课程，覆盖行列式、矩阵、向量组、线性方程组、特征值等核心内容。', 2, 1),
(3, '概率论与数理统计', 'probability_statistics', '面向高校在校生的概率论与数理统计复习课程，覆盖随机事件、随机变量、数字特征、参数估计和假设检验等核心内容。', 3, 1);

INSERT INTO chapter (course_id, parent_id, title, description, sort_order, status) VALUES
(1, NULL, '函数、极限与连续', '高等数学基础章节，包含函数、数列极限、函数极限与连续性。', 1, 1),
(1, NULL, '导数与微分', '包含导数概念、求导法则、高阶导数和微分。', 2, 1),
(1, NULL, '中值定理与导数应用', '包含微分中值定理、洛必达法则、单调性、极值和最值。', 3, 1),
(1, NULL, '不定积分与定积分', '包含原函数、不定积分、定积分概念、性质和计算。', 4, 1),
(1, NULL, '多元函数微积分', '包含偏导数、全微分、多元函数极值和二重积分基础。', 5, 1),
(1, NULL, '级数与微分方程', '包含常数项级数、幂级数和一阶微分方程基础。', 6, 1),

(2, NULL, '行列式', '线性代数基础章节，包含行列式定义、性质和计算。', 1, 1),
(2, NULL, '矩阵及其运算', '包含矩阵运算、逆矩阵、分块矩阵和初等变换。', 2, 1),
(2, NULL, '向量组与线性相关性', '包含向量组线性表示、线性相关、秩和极大无关组。', 3, 1),
(2, NULL, '线性方程组', '包含齐次和非齐次线性方程组的解结构。', 4, 1),
(2, NULL, '特征值与特征向量', '包含特征值、特征向量、相似矩阵和矩阵对角化。', 5, 1),
(2, NULL, '二次型', '包含二次型、合同变换、正定二次型和标准形。', 6, 1),

(3, NULL, '随机事件与概率', '概率论基础章节，包含随机试验、事件关系、概率性质和条件概率。', 1, 1),
(3, NULL, '随机变量及其分布', '包含离散型和连续型随机变量、分布函数与常见分布。', 2, 1),
(3, NULL, '多维随机变量', '包含二维随机变量、边缘分布、条件分布和独立性。', 3, 1),
(3, NULL, '随机变量的数字特征', '包含数学期望、方差、协方差和相关系数。', 4, 1),
(3, NULL, '大数定律与中心极限定理', '包含大数定律、中心极限定理及其基本应用。', 5, 1),
(3, NULL, '数理统计基础', '包含样本、统计量、参数估计和假设检验基础。', 6, 1);

INSERT INTO knowledge_point
(course_id, chapter_id, title, core_content, formulas, typical_questions, common_mistakes, exam_frequency, review_advice, sort_order, status)
VALUES
(1, 1, '函数的概念与基本性质',
'掌握函数定义域、值域、对应关系以及函数的有界性、单调性、奇偶性和周期性。复合函数、反函数和分段函数是后续极限与导数计算的基础。',
'奇函数：f(-x)=-f(x)；偶函数：f(-x)=f(x)；单调递增：x1<x2 时 f(x1)<=f(x2)。',
'求函数定义域；判断函数奇偶性、周期性、单调性；分析分段函数在某点附近的表达式。',
'忽略分母不为0、偶次根号非负、对数真数大于0等定义域限制；判断复合函数性质时只看外层函数。',
'high',
'先做定义域，再看表达式结构。遇到分段函数时画出分段点附近的左右表达式，为极限和连续性判断做准备。',
1, 1),
(1, 1, '数列极限与函数极限',
'理解极限描述的是变量趋近过程中函数值或数列项的趋势，重点掌握极限四则运算、等价无穷小替换和夹逼准则。',
'lim(x->0) sin x / x = 1；lim(x->0) (1-cos x)/x^2 = 1/2；lim(x->0) ln(1+x)/x = 1；lim(x->0) (e^x-1)/x = 1。',
'计算含三角函数、指数函数、对数函数的极限；使用等价无穷小化简；判断左右极限是否相等。',
'等价无穷小替换用于乘除关系更稳，不要随意在加减关系中局部替换；忽略左右极限会导致分段函数极限误判。',
'high',
'把常用等价无穷小单独背熟，并训练先化简再代入。分段函数题优先计算左极限和右极限。',
2, 1),
(1, 1, '函数连续性与间断点',
'掌握函数在一点连续的三个条件：函数值存在、极限存在、极限等于函数值。能够判断可去间断、跳跃间断和无穷间断。',
'f(x) 在 x0 连续：lim(x->x0) f(x)=f(x0)。闭区间连续函数性质：有界性、最值定理、介值定理。',
'判断函数在指定点是否连续；求参数使分段函数连续；分类讨论间断点类型。',
'只验证极限存在但忘记比较函数值；参数题中只令左右极限相等，忘记和该点函数值相等。',
'high',
'连续性题按“三步检查”写：函数值、极限、二者是否相等。参数题要把左右极限和点值放在同一条等式中。',
3, 1);

INSERT INTO question
(id, course_id, chapter_id, knowledge_point_id, exam_paper_id, title, content, question_type, difficulty,
 standard_answer, normal_explanation, source, source_year, from_exam_paper, paper_question_no, sort_order, status)
VALUES
(1, 1, 1, 2, NULL, '重要极限计算',
'计算极限：lim(x->0) sin x / x = ?',
'single_choice', 'basic',
'A',
'这是第一个重要极限，lim(x->0) sin x / x = 1。可由单位圆几何夹逼或泰勒展开得到。',
'高等数学基础题', 2024, 0, NULL, 1, 1),
(2, 1, 1, 3, NULL, '分段函数连续性',
'设 f(x)=x^2，当 x<1；f(x)=a，当 x=1；f(x)=2x-1，当 x>1。求 a 使 f(x) 在 x=1 处连续。',
'blank', 'medium',
'a=1',
'左极限为 1，右极限为 1。连续要求 f(1)=a 也等于该极限，因此 a=1。',
'高等数学章节练习', 2024, 0, NULL, 2, 1),
(3, 2, 7, NULL, NULL, '二阶行列式',
'计算行列式 |1 2; 3 4| 的值。',
'calculation', 'basic',
'-2',
'二阶行列式 ad-bc=1*4-2*3=4-6=-2。',
'线性代数基础题', 2024, 0, NULL, 3, 1),
(4, 2, 10, NULL, NULL, '齐次线性方程组解的判定',
'说明齐次线性方程组 Ax=0 何时存在非零解。',
'solution', 'medium',
'当系数矩阵 A 的秩 r(A) 小于未知量个数 n 时，齐次线性方程组 Ax=0 存在非零解。',
'齐次线性方程组的解空间维数为 n-r(A)。若 n-r(A)>0，则存在非零解。',
'线性代数章节练习', 2023, 0, NULL, 4, 1),
(5, 3, 13, NULL, NULL, '条件概率',
'设 P(A)=0.5，P(B)=0.4，P(AB)=0.2，求 P(A|B)。',
'single_choice', 'basic',
'B',
'条件概率公式 P(A|B)=P(AB)/P(B)=0.2/0.4=0.5。',
'概率论基础题', 2024, 0, NULL, 5, 1);

INSERT INTO question_option
(question_id, option_label, option_content, is_correct, sort_order)
VALUES
(1, 'A', '1', 1, 1),
(1, 'B', '0', 0, 2),
(1, 'C', '+∞', 0, 3),
(1, 'D', '不存在', 0, 4),
(5, 'A', '0.2', 0, 1),
(5, 'B', '0.5', 1, 2),
(5, 'C', '0.8', 0, 3),
(5, 'D', '1', 0, 4);
