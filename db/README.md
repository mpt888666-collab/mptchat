# 数据库

`schema.sql` 是 ChatDemo 的完整表结构（MySQL 8 / 9，字符集 `utf8mb4`），共 8 张表：

| 表名 | 作用 | 关键索引 / 约束 |
| --- | --- | --- |
| `user` | 账号、昵称、密码、头像 | `uniq(username)` |
| `private_chat` | 单聊会话（双方 uid ↔ thread_id） | `uniq(user1_id, user2_id)`、`idx(user1_id, thread_id)`、`idx(user2_id, thread_id)` |
| `group_chat` | 群聊会话 | `pk(thread_id)` |
| `group_chat_member` | 群成员 | `pk(thread_id, user_id)`、`idx(user_id)` |
| `chat_thread` | 会话索引（会话列表） | `pk(id)` |
| `chat_message` | 消息明细（文本 / 图片 / 文件） | `pk(message_id)`、`idx(thread_id, created_at)`、`idx(thread_id, message_id)` |
| `friend` | 好友关系 | `uniq(owner_uid, friend_uid)` |
| `friend_apply` | 好友申请 | `uniq(from_uid, to_uid)` |

## 导入

```bash
# 从仓库根目录执行
mysql -h 127.0.0.1 -P 3306 -u root -p \
  -e "CREATE DATABASE IF NOT EXISTS chatdb DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"

mysql -h 127.0.0.1 -P 3306 -u root -p --default-character-set=utf8mb4 chatdb < db/schema.sql

# 校验：应输出 8
mysql -h 127.0.0.1 -P 3306 -u root -p -N \
  -e "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='chatdb';"
```

> `schema.sql` 由 `mysqldump --no-data` 导出，可直接重复执行（脚本开头会 `DROP TABLE IF EXISTS`）。

## 压测账号

`bench/chat_stress.py` 的 `--mode msg` 默认使用 `user1 / user2`。
`--mode login` 和 `--mode sessions` 需要**互不相同的账号**（原因见 `bench/README.md`），
直接导入压测账号即可：

```bash
mysql -h 127.0.0.1 -P 3306 -u root -p --default-character-set=utf8mb4 chatdb \
  < db/seed_bench_users.sql
```

`seed_bench_users.sql` 会插入 `bench0001` ~ `bench1000` 共 1000 个账号
（密码统一 `123456`，邮箱 `benchNNNN@bench.local`），可以用
`--distinct-users 1000` 直接压到 1000 条并发长连接。脚本可重复执行（先按前缀删再插）。

> ⚠️ **密码目前是明文入库**（`user.password`），这是已知的待改进项，见根目录 README 的 Roadmap。
> 压测账号用的是弱口令，**只用于本地压测**，不要在生产环境照搬。