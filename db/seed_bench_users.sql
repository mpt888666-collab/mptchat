-- ChatDemo 压测账号：bench0001 ~ bench1000
-- 密码统一 123456，邮箱 benchNNNN@bench.local
-- 可重复执行（先按前缀删除再插入）。
-- 导入：mysql -h127.0.0.1 -uroot -p --default-character-set=utf8mb4 chatdb < db/seed_bench_users.sql

DELETE FROM user WHERE username LIKE 'bench%';
INSERT INTO user (username, password, email, nick)
WITH RECURSIVE seq(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM seq WHERE n < 1000)
SELECT CONCAT('bench', LPAD(n,4,'0')), '123456', CONCAT('bench', LPAD(n,4,'0'), '@bench.local'), CONCAT('压测账号', n) FROM seq;
SELECT COUNT(*) AS bench_users FROM user WHERE username LIKE 'bench%';