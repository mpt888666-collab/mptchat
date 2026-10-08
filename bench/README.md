# 压测工具

`chat_stress.py` 用纯 Python 标准库实现了 ChatDemo 的协议，不依赖 Qt 客户端，
可以在一台机器上模拟大量并发连接与消息收发。

## 协议

与客户端一致的自研二进制帧：

```
+--------+--------+-------------------+
| msg_id | length |  body (UTF-8 JSON) |
| uint16 | uint16 |  length 字节       |
+--------+--------+-------------------+
```

多字节整数均为**大端**（网络字节序）。

## 前置条件

1. MySQL 已导入 `db/schema.sql`，并导入压测账号：`mysql ... chatdb < db/seed_bench_users.sql`
2. Redis 已启动
3. `StatusServer`（`:50052`）、`GetServer`（HTTP GateServer，默认 8080）、`ChatServer`（`:8090` 和 `:8091` 两个实例）已启动

> **为什么要专门准备一批账号**：`utoken_<uid>` 在 Redis 里是单个 key，
> 同一个账号再登录会覆盖旧 token，旧连接随即失效（单点登录语义）。
> 所以「并发登录」和「并发长连接」必须用**互不相同的账号**，
> 否则只会成功 1 个 —— 这是协议行为，不是压测工具的问题。

## 用法

```bash
# 1) 登录风暴：并发登录（HTTP 登录 + TCP 建连 + chat_login 握手）后断开
python bench/chat_stress.py --mode login --clients 1000 --distinct-users 1000

# 2) 连接风暴：只建连 + 发一次登录帧（用于观察连接建立能力），token 故意非法
python bench/chat_stress.py --mode connect --clients 2000

# 3) 并发长连接：N 条连接同时保持 --duration 秒，每 --hb-interval 秒心跳一次
#    结束时打印 handshake_ok / alive_at_end / hb_err / drop，并用 Redis 交叉验证在线数
python bench/chat_stress.py --mode sessions --clients 1000 --distinct-users 1000 \
    --duration 60 --hb-interval 5 --redis-port 6379

# 4) 单连接心跳：同一条长连接上串行打心跳，测最纯的往返时延
python bench/chat_stress.py --mode heartbeat --duration 30

# 5) 消息收发：user1 → user2 持续发消息，统计发送 QPS、ack 时延与对端收到条数
python bench/chat_stress.py --mode msg --duration 60 --rate 5 --msg-size 32 \
    --user user1 --peer-user user2
```

> **默认跟随网关下发的实例**：`chat_stress.py` 按登录响应里的 `chat_host` / `chat_port` 连接，
> 也就是 StatusServer 均衡选出来的那台 ChatServer。`login` / `sessions` 模式结束时会打印
> `assigned instances: 127.0.0.1:8090=N 127.0.0.1:8091=M`，可以直接看出分布。
> 要固定压某一台，用 `--chat-host` / `--chat-port` 显式覆盖；`connect` 模式没有登录响应，固定压 `--chat-port`。

## 参数

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `--mode` | `login` / `connect` / `heartbeat` / `msg` / `sessions` | `login` |
| `--gate` | GateServer 地址 | `http://localhost:8080` |
| `--chat-host` / `--chat-port` | 强制指定 ChatServer 地址；默认跟随登录响应里的实例 | 跟随登录响应 |
| `--clients` | 并发客户端数 | `50` |
| `--duration` | 持续秒数（heartbeat / msg / sessions） | `10` |
| `--rate` | 每轮发送条数（msg） | `10` |
| `--msg-size` | 消息体字节数（msg） | `32` |
| `--hb-interval` | 心跳间隔秒数（sessions） | `5` |
| `--user` / `--passwd` / `--email` | 发送方账号 | `user1` / `123456` / `user1@qq.com` |
| `--peer-user` / `--peer-passwd` / `--peer-email` | 接收方账号 | `user2` / `123456` / `user2@qq.com` |

账号池参数（`login` / `sessions` 模式用）：

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `--distinct-users` | 生成 N 个互不相同的账号（`0` = 所有客户端复用 `--user`） | `0` |
| `--user-pattern` | 账号名模板 | `bench%04d` |
| `--user-start` | 账号起始编号 | `1` |
| `--users` | 显式逗号分隔的账号列表 | 空 |
| `--mail-pattern` | 账号对应的邮箱模板 | `%s@bench.local` |

服务端交叉验证（`sessions` 模式用）：

| 参数 | 说明 | 默认值 |
| --- | --- | --- |
| `--redis-host` / `--redis-port` | Redis 地址，`0` = 关闭交叉验证 | `127.0.0.1` / `0` |
| `--server-name` | 查询 `logincount` 用的实例名 | `chatserver1` |

## 指标口径

| 指标 | 定义 |
| --- | --- |
| QPS | 成功次数 / 实际耗时 |
| 响应时延 | 从 `send_frame` 到收到对应响应帧的时间，含服务端处理 + 回包 |
| p50 / p95 / p99 | 时延样本升序排序后的分位数 |
| 错误率 | 失败次数 / 总请求数 |
| `handshake_ok` | `sessions` 模式里完成「HTTP 登录 + 建连 + chat_login」的条数 |
| `alive_at_end` | 保持到时间结束仍未掉线的连接数 |
| `drop` | 保持期间心跳失败（视为掉线）的连接数 |
| `loss` | msg 模式里 `sent - recv_count`，对端未收到或去重后少掉的条数 |

## 实测结果

本次发版实测的原始输出在 [`results/`](results/) 目录，汇总表格见根目录 README 的「七、性能压测」。

## 辅助脚本

这两个 PowerShell 脚本是本次发版压测时用的，Windows 环境可直接跑（脚本里的本机路径按需修改）：

| 脚本 | 作用 |
| --- | --- |
| `multiproc_sessions.ps1` | 把并发长连接拆成 N 个进程 × M 条连接（默认 5 × 100），规避单进程 Python 的 GIL 惊群，并汇总各进程结果、采样服务端 CPU/RSS、与 Redis 在线数交叉验证 |
| `logging_ab.ps1` | 对照实验：同一份二进制，服务端 stdout 重定向到日志文件 vs 重定向到 `NUL`，量化同步日志的开销 |

```powershell
# 500 条并发长连接，5 个进程 x 100 连接，保持 60 秒
powershell -File bench/multiproc_sessions.ps1 -Total 500 -PerProc 100 -Duration 60

# 服务端日志开销 A/B（需要先起好服务端，脚本会自行重启 ChatServer）
powershell -File bench/logging_ab.ps1
```
## 注意

- 压测机与服务端同机时，瓶颈经常出现在**压测脚本**而不是服务端：500 并发跑在单个 Python
  进程里时心跳 p95 会飙到 3 秒，改成多进程后降到 0.29 秒 —— 差异来自 Python 线程集中唤醒争抢 GIL。
  高并发场景建议另起一台机器作为压测客户端，或像 `multiproc_sessions.ps1` 那样拆成多进程。
- `msg` 模式会把真实消息写进 `chat_message` 表（20 秒约 1 万条），压测后需要自己清理。
- 结果受机器配置、MySQL / Redis 是否同机、服务端日志级别影响很大，请记录环境后再对比。