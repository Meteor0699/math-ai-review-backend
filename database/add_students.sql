USE math_ai_review;

INSERT INTO user (username, password_hash, real_name, role, status) VALUES
('student', 'sha256$dev123$f7f59778cf2f81a087d959beb59dacc0402831f34e1aaf50110a21977e672eae', '学生张三', 'student', 1),
('student2', 'sha256$dev123$f7f59778cf2f81a087d959beb59dacc0402831f34e1aaf50110a21977e672eae', '学生李四', 'student', 1);