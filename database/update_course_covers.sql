-- 为已有数据库补充课程封面地址。
-- 使用方式：mysql -u root -p math_ai_review < database/update_course_covers.sql

USE math_ai_review;

UPDATE course
SET cover_url = '/textbook-covers/tongji-advanced-math-1.svg'
WHERE code = 'advanced_math';

UPDATE course
SET cover_url = '/textbook-covers/engineering-math-linear-algebra.svg'
WHERE code = 'linear_algebra';

UPDATE course
SET cover_url = '/textbook-covers/znufe-probability-statistics.svg'
WHERE code = 'probability_statistics';
