-- test_data.sql
-- 测试数据初始化脚本
-- 使用方式：mysql -u root -p < database/schema.sql && mysql -u root -p < database/test_data.sql

USE math_ai_review;

-- ========== 1. 用户数据 ==========
-- 以下哈希仅用于本地开发数据，不应作为生产账号凭据。
INSERT INTO user (username, password_hash, real_name, role, status) VALUES
('admin',  'sha256$dev123$f7f59778cf2f81a087d959beb59dacc0402831f34e1aaf50110a21977e672eae', '管理员', 'admin', 1),
('student', 'sha256$dev123$f7f59778cf2f81a087d959beb59dacc0402831f34e1aaf50110a21977e672eae', '学生张三', 'student', 1),
('student2', 'sha256$dev123$f7f59778cf2f81a087d959beb59dacc0402831f34e1aaf50110a21977e672eae', '学生李四', 'student', 1);

-- ========== 2. 课程数据 ==========
INSERT INTO course (name, code, description, sort_order, status) VALUES
('高等数学', 'MATH101', '高等数学公共基础课程，涵盖函数、极限、微积分等核心内容', 1, 1),
('线性代数', 'MATH102', '线性代数公共基础课程，涵盖矩阵、向量空间、特征值等核心内容', 2, 1),
('概率论与数理统计', 'MATH103', '概率论与数理统计公共基础课程，涵盖概率、随机变量、统计推断等核心内容', 3, 1);

-- ========== 3. 章节数据 ==========
-- 高等数学章节
INSERT INTO chapter (course_id, title, description, sort_order, status) VALUES
(1, '第一章 函数与极限', '函数的概念、极限的定义与计算、无穷小与无穷大', 1, 1),
(1, '第二章 导数与微分', '导数的定义与计算、微分的概念与应用', 2, 1),
(1, '第三章 不定积分', '不定积分的概念、基本积分公式、换元积分法', 3, 1),
(1, '第四章 定积分', '定积分的定义与性质、牛顿-莱布尼茨公式、定积分应用', 4, 1);

-- 线性代数章节
INSERT INTO chapter (course_id, title, description, sort_order, status) VALUES
(2, '第一章 行列式', '行列式的定义、性质与计算', 1, 1),
(2, '第二章 矩阵及其运算', '矩阵的定义、矩阵运算、逆矩阵', 2, 1),
(2, '第三章 向量空间', '向量组的线性相关性、向量空间的基与维数', 3, 1);

-- 概率论与数理统计章节
INSERT INTO chapter (course_id, title, description, sort_order, status) VALUES
(3, '第一章 随机事件与概率', '随机事件、概率的定义与性质、条件概率', 1, 1),
(3, '第二章 随机变量及其分布', '离散型随机变量、连续型随机变量、分布函数', 2, 1),
(3, '第三章 数理统计基础', '样本与统计量、参数估计、假设检验', 3, 1);

-- ========== 4. 重点知识数据 ==========
-- 高等数学 - 第一章
INSERT INTO knowledge_point (course_id, chapter_id, title, core_content, formulas, typical_questions, common_mistakes, exam_frequency, review_advice, sort_order, status) VALUES
(1, 1, '函数极限的定义与计算', '函数极限是微积分的基础概念。极限描述的是函数在某一点附近的变化趋势。掌握ε-δ语言定义的极限概念，以及极限的四则运算法则、两个重要极限、等价无穷小替换等计算方法。', '1. 两个重要极限：lim(x→0) sinx/x = 1；lim(x→∞) (1+1/x)^x = e\n2. 等价无穷小：x→0时，sinx~x，tanx~x，ln(1+x)~x，e^x-1~x', '1. 利用两个重要极限求极限\n2. 利用等价无穷小替换求极限\n3. 洛必达法则求极限', '混淆无穷小与很小的数；等价无穷小替换条件不满足时使用；极限存在与函数值的关系不清', 'high', '重点掌握两个重要极限及其变形，熟练运用等价无穷小替换，注意使用条件', 1, 1),
(1, 1, '无穷小与无穷大', '无穷小与无穷大是极限理论中的重要概念。理解无穷小的阶的比较，掌握无穷小的运算性质。', '无穷小比较：若lim α/β=0，则α是比β高阶的无穷小\n若lim α/β=c≠0，则α与β是同阶无穷小', '判断无穷小的阶数；利用无穷小比较求极限', '混淆无穷小与负无穷大；无穷小的阶判断错误', 'medium', '理解无穷小比较的几何意义，多做练习题巩固', 2, 1);

-- 高等数学 - 第二章
INSERT INTO knowledge_point (course_id, chapter_id, title, core_content, formulas, typical_questions, common_mistakes, exam_frequency, review_advice, sort_order, status) VALUES
(1, 2, '导数的定义与几何意义', '导数描述函数在某一点的变化率。导数的几何意义是曲线在该点处切线的斜率。掌握导数定义、基本求导公式、四则运算求导法则。', '导数定义：f''(x0)=lim(Δx→0) [f(x0+Δx)-f(x0)]/Δx\n基本求导公式：(c)''=0, (x^n)''=nx^(n-1), (sinx)''=cosx, (cosx)''=-sinx, (e^x)''=e^x', '利用导数定义求极限；求复合函数的导数；求隐函数的导数', '混淆可导与连续的关系；复合函数求导遗漏链式法则', 'high', '牢记基本求导公式，理解导数的几何意义，多做复合函数求导练习', 1, 1);

-- 线性代数 - 第一章
INSERT INTO knowledge_point (course_id, chapter_id, title, core_content, formulas, typical_questions, common_mistakes, exam_frequency, review_advice, sort_order, status) VALUES
(2, 5, '行列式的计算', '行列式是线性代数中的重要工具。掌握二阶、三阶行列式的对角线法则，以及高阶行列式的按行（列）展开计算方法。', '二阶行列式：|a b;c d| = ad-bc\n三阶行列式按第一行展开\n拉普拉斯展开定理', '计算具体数字行列式；含参数行列式的计算；利用行列式性质简化计算', '展开时符号错误；混淆行列式与矩阵的概念', 'high', '熟练掌握行列式的性质和计算方法，注意符号变化', 1, 1),
(2, 5, '行列式的性质', '行列式具有多种重要性质，包括转置不变性、互换行（列）变号、倍乘性质、倍加性质等。利用这些性质可以简化行列式的计算。', '|A^T| = |A|；互换两行：|A| → -|A|\n某行乘以k：|A| → k|A|；某行加到另一行：|A|不变', '利用行列式性质化简后计算；证明行列式恒等式', '性质使用条件混淆；运算顺序错误', 'medium', '理解每条性质的几何意义，通过练习掌握灵活运用', 2, 1);

-- 概率论 - 第一章
INSERT INTO knowledge_point (course_id, chapter_id, title, core_content, formulas, typical_questions, common_mistakes, exam_frequency, review_advice, sort_order, status) VALUES
(3, 8, '条件概率与全概率公式', '条件概率是概率论的核心概念。掌握条件概率的定义、乘法公式、全概率公式和贝叶斯公式。', '条件概率：P(A|B) = P(AB)/P(B)\n全概率公式：P(A) = ΣP(Bi)P(A|Bi)\n贝叶斯公式：P(Bi|A) = P(Bi)P(A|Bi)/P(A)', '利用全概率公式求概率；利用贝叶斯公式求后验概率；实际应用题', '混淆P(A|B)与P(AB)；全概率公式中Bi不构成完备事件组', 'high', '理解条件概率的实际含义，熟练掌握全概率和贝叶斯公式的应用场景', 1, 1);

-- ========== 5. 题库数据 ==========
-- 高等数学题目
INSERT INTO question (course_id, chapter_id, title, content, question_type, difficulty, standard_answer, normal_explanation, source, source_year, from_exam_paper, sort_order, status) VALUES
(1, 1, '极限计算题', '求极限：lim(x→0) (sin3x)/(2x)', 'calculation', 'basic', '3/2', '解法一：利用等价无穷小替换，x→0时sin3x~3x，所以lim(x→0) sin3x/(2x) = lim(x→0) 3x/(2x) = 3/2。\n解法二：利用重要极限lim(x→0) sinx/x = 1，原式=lim(x→0) (sin3x)/(3x) · 3/2 = 3/2。', '2023年期末试卷', 2023, 1, 1, 1),
(1, 1, '极限计算题2', '求极限：lim(x→∞) (1+2/x)^x', 'calculation', 'medium', 'e^2', '利用重要极限lim(x→∞) (1+1/x)^x = e。令t=x/2，则原式=lim(t→∞) (1+1/t)^(2t) = [lim(t→∞) (1+1/t)^t]^2 = e^2。', '2024年期中试卷', 2024, 1, 2, 1),
(1, 2, '导数计算题', '求函数 f(x)=x^3·sinx 的导数', 'calculation', 'basic', 'f''(x)=3x^2·sinx + x^3·cosx', '利用乘积求导法则：(uv)''=u''v+uv''。u=x^3, u''=3x^2；v=sinx, v''=cosx。所以f''(x)=3x^2·sinx + x^3·cosx。', '课后习题', NULL, 0, 3, 1),
(1, 2, '导数应用题', '求曲线 y=x^3-3x^2+2 的极值点', 'calculation', 'medium', '极大值点(0,2)，极小值点(2,-2)', '求导得y''=3x^2-6x=3x(x-2)，令y''=0得x=0或x=2。\nx<0时y''>0，0<x<2时y''<0，x>2时y''>0。\n所以x=0为极大值点，极大值为2；x=2为极小值点，极小值为-2。', '2024年期末试卷', 2024, 1, 4, 1);

-- 线性代数题目
INSERT INTO question (course_id, chapter_id, title, content, question_type, difficulty, standard_answer, normal_explanation, source, source_year, from_exam_paper, sort_order, status) VALUES
(2, 5, '行列式计算', '计算三阶行列式：|1 2 3; 4 5 6; 7 8 9|', 'calculation', 'basic', '0', '计算过程：\n=1·5·9 + 2·6·7 + 3·4·8 - 3·5·7 - 2·4·9 - 1·6·8\n=45 + 84 + 96 - 105 - 72 - 48\n=225 - 225 = 0\n该行列式值为0，因为第三行等于前两行之和（线性相关）。', '课后习题', NULL, 0, 1, 1),
(2, 6, '矩阵运算', '已知矩阵 A=[1 2; 3 4]，B=[5 6; 7 8]，求 A+B 和 AB', 'calculation', 'basic', 'A+B=[6 8; 10 12]；AB=[19 22; 43 50]', 'A+B各元素对应相加即可。\nAB计算：第一行第一列=1·5+2·7=19，第一行第二列=1·6+2·8=22，第二行第一列=3·5+4·7=43，第二行第二列=3·6+4·8=50。', '2023年期末试卷', 2023, 1, 2, 1);

-- 概率论题目
INSERT INTO question (course_id, chapter_id, title, content, question_type, difficulty, standard_answer, normal_explanation, source, source_year, from_exam_paper, sort_order, status) VALUES
(3, 8, '条件概率题', '已知P(A)=0.5, P(B)=0.4, P(AB)=0.2，求P(A|B)和P(B|A)', 'calculation', 'basic', 'P(A|B)=0.5, P(B|A)=0.4', 'P(A|B)=P(AB)/P(B)=0.2/0.4=0.5\nP(B|A)=P(AB)/P(A)=0.2/0.5=0.4', '课后习题', NULL, 0, 1, 1),
(3, 8, '全概率公式', '某工厂有三条生产线，产量分别占总产量的30%、30%、40%，不合格率分别为2%、3%、1%。随机抽取一件产品，求其为不合格品的概率。', 'calculation', 'medium', '0.019', '设事件A为抽到不合格品，B1,B2,B3为分别来自三条生产线。\nP(A)=P(B1)P(A|B1)+P(B2)P(A|B2)+P(B3)P(A|B3)\n=0.3×0.02+0.3×0.03+0.4×0.01\n=0.006+0.009+0.004=0.019', '2024年期中试卷', 2024, 1, 2, 1);
