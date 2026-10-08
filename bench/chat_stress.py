#!/usr/bin/env python3
# ChatDemo stress test tool
# Protocol: uint16 msg_id (BE) + uint16 body_len (BE) + UTF-8 JSON body
import argparse
import json
import socket
import struct
import threading
import time
import urllib.request
import uuid

IDS = {
    "chat_login": 1005,
    "chat_login_rsp": 1006,
    "text_chat_req": 1017,
    "text_chat_rsp": 1018,
    "notify_text_chat": 1019,
    "heartbeat_req": 1023,
    "heartbeat_rsp": 1024,
    "load_thread_req": 1025,
    "load_thread_rsp": 1026,
    "create_private_chat_req": 1027,
    "create_private_chat_rsp": 1028,
    "load_msg_req": 1029,
    "load_msg_rsp": 1030,
}


def http_json(url, payload, timeout=10):
    data = json.dumps(payload).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read().decode("utf-8"))


def recv_exact(sock, n):
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError("closed")
        buf += chunk
    return buf


def recv_frame(sock, timeout=10):
    sock.settimeout(timeout)
    head = recv_exact(sock, 4)
    msg_id, body_len = struct.unpack(">HH", head)
    body = recv_exact(sock, body_len)
    return msg_id, json.loads(body.decode("utf-8"))


def send_frame(sock, msg_id, obj):
    body = json.dumps(obj, ensure_ascii=False).encode("utf-8")
    sock.sendall(struct.pack(">HH", msg_id, len(body)) + body)


def pct(sorted_values, p):
    """Percentile (0.0 - 1.0) of an already sorted list, in milliseconds."""
    if not sorted_values:
        return 0.0
    idx = min(len(sorted_values) - 1, int(len(sorted_values) * p))
    return sorted_values[idx] * 1000.0


def report_latency(lat):
    lat.sort()
    if not lat:
        return
    print(f"  rtt ms p50={pct(lat, 0.50):.2f} p95={pct(lat, 0.95):.2f} "
          f"p99={pct(lat, 0.99):.2f} max={lat[-1] * 1000:.2f}")


def redis_query(host, port, *cmd):
    """Minimal RESP client, enough for INFO/HGET/HLEN/KEYS/DBSIZE."""
    payload = b"*%d\r\n" % len(cmd)
    for arg in cmd:
        raw = arg.encode()
        payload += b"$%d\r\n%s\r\n" % (len(raw), raw)
    s = socket.create_connection((host, int(port)), timeout=5)
    try:
        s.sendall(payload)
        time.sleep(0.05)
        chunks = []
        while True:
            try:
                data = s.recv(65536)
            except socket.timeout:
                break
            if not data:
                break
            chunks.append(data)
            if len(data) < 65536:
                break
        return b"".join(chunks).decode("utf-8", "replace")
    finally:
        s.close()


def build_pool(args):
    """Return a list of distinct test users (empty list = reuse --user)."""
    if args.users:
        return [u.strip() for u in args.users.split(",") if u.strip()]
    if args.distinct_users > 0:
        return [args.user_pattern % (args.user_start + i) for i in range(args.distinct_users)]
    return []


def pool_credentials(args, pool, i):
    if not pool:
        return args.user, args.passwd, args.email
    user = pool[i % len(pool)]
    return user, args.passwd, args.mail_pattern % user


class ChatClient:
    def __init__(self, host, port, uid=None, token=None):
        self.host = host
        self.port = port
        self.uid = uid
        self.token = token
        self.sock = None

    def connect(self):
        self.sock = socket.create_connection((self.host, self.port), timeout=10)

    def close(self):
        if self.sock:
            try:
                self.sock.close()
            except OSError:
                pass
            self.sock = None

    def chat_login(self):
        if self.uid is None or self.token is None:
            raise ValueError("uid/token required")
        send_frame(self.sock, IDS["chat_login"], {"uid": self.uid, "token": self.token})
        msg_id, rsp = recv_frame(self.sock)
        return msg_id, rsp

    def load_threads(self):
        send_frame(self.sock, IDS["load_thread_req"], {"uid": self.uid, "thread_id": 0})
        msg_id, rsp = recv_frame(self.sock)
        return msg_id, rsp

    def create_private_chat(self, other_uid):
        send_frame(self.sock, IDS["create_private_chat_req"], {"uid": self.uid, "other_id": other_uid})
        msg_id, rsp = recv_frame(self.sock)
        return msg_id, rsp

    def send_text(self, peer_uid, thread_id, content):
        send_frame(self.sock, IDS["text_chat_req"], {
            "fromuid": self.uid,
            "touid": peer_uid,
            "thread_id": thread_id,
            "text_array": [{"content": content, "unique_id": str(uuid.uuid4())}],
        })
        msg_id, rsp = recv_frame(self.sock)
        return msg_id, rsp

    def heartbeat(self):
        send_frame(self.sock, IDS["heartbeat_req"], {"fromuid": self.uid})
        msg_id, rsp = recv_frame(self.sock)
        return msg_id, rsp


def login_one(gate, user, pwd, email):
    info = http_json(gate + "/user_login", {"user": user, "passwd": pwd, "email": email})
    if info.get("error", 1) != 0:
        raise RuntimeError(f"http login failed: {info}")
    return info


def chat_target(args, info):
    """Chat server this client should connect to.

    An explicit --chat-host/--chat-port wins; otherwise follow the instance the
    gate assigned at login, i.e. the result of StatusServer load balancing.
    """
    host = args.chat_host or info.get("chat_host") or "127.0.0.1"
    port = int(args.chat_port or info.get("chat_port") or 8090)
    return host, port


def print_targets(targets):
    if targets:
        print("  assigned instances: " + " ".join(f"{k}={targets[k]}" for k in sorted(targets)))


def run_login_storm(args):
    """HTTP login -> TCP connect -> chat_login handshake -> close."""
    pool = build_pool(args)
    lat = []
    errs = []
    targets = {}
    lock = threading.Lock()

    def worker(i):
        c = None
        t0 = time.perf_counter()
        try:
            user, pwd, email = pool_credentials(args, pool, i)
            info = login_one(args.gate, user, pwd, email)
            host, port = chat_target(args, info)
            with lock:
                key = f"{host}:{port}"
                targets[key] = targets.get(key, 0) + 1
            c = ChatClient(host, port, info["uid"], info["token"])
            c.connect()
            msg_id, rsp = c.chat_login()
            if rsp.get("error", 1) != 0:
                raise RuntimeError(f"chat_login error={rsp.get('error')}")
            with lock:
                lat.append(time.perf_counter() - t0)
        except Exception as e:
            with lock:
                errs.append(str(e))
        finally:
            if c is not None:
                c.close()

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(args.clients)]
    t_start = time.perf_counter()
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    wall = time.perf_counter() - t_start
    scope = f"distinct={len(pool)}" if pool else f"single={args.user}"
    print(f"login storm: clients={args.clients} {scope} wall={wall:.3f}s "
          f"ok={len(lat)} err={len(errs)} qps={len(lat) / max(wall, 1e-9):.1f}")
    print_targets(targets)
    report_latency(lat)
    for e in errs[:3]:
        print(f"  sample error: {e}")


def run_connect_storm(args):
    """Pure TCP accept + rejected handshake (bad token) -> close."""
    errs = []
    ok = 0
    lock = threading.Lock()

    def worker(i):
        nonlocal ok
        c = None
        try:
            c = ChatClient(args.chat_host or "127.0.0.1", args.chat_port or 8090, 1, "bad-token")
            c.connect()
            send_frame(c.sock, IDS["chat_login"], {"uid": 1, "token": "bad-token"})
            try:
                recv_frame(c.sock, timeout=3)
            except Exception:
                pass
            with lock:
                ok += 1
        except Exception as e:
            with lock:
                errs.append(str(e))
        finally:
            if c is not None:
                c.close()

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(args.clients)]
    t_start = time.perf_counter()
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    wall = time.perf_counter() - t_start
    print(f"connect storm: clients={args.clients} wall={wall:.3f}s ok={ok} err={len(errs)} "
          f"accept_rate={ok / max(args.clients, 1) * 100:.1f}%")
    for e in errs[:3]:
        print(f"  sample error: {e}")


def run_heartbeat(args):
    """Serial heartbeat RTT on a single long connection."""
    info = login_one(args.gate, args.user, args.passwd, args.email)
    host, port = chat_target(args, info)
    c = ChatClient(host, port, info["uid"], info["token"])
    c.connect()
    c.chat_login()
    lat = []
    errs = 0
    end = time.time() + args.duration
    while time.time() < end:
        t0 = time.perf_counter()
        try:
            c.heartbeat()
            lat.append(time.perf_counter() - t0)
        except Exception:
            errs += 1
            time.sleep(0.01)
    c.close()
    print(f"heartbeat: n={len(lat)} err={errs} qps={len(lat) / max(args.duration, 1e-9):.1f}")
    report_latency(lat)


def run_sessions(args):
    """N concurrent long connections held for --duration, heartbeating every --hb-interval."""
    pool = build_pool(args)
    n = args.clients
    if len(pool) < n:
        raise SystemExit(
            f"sessions mode needs >= {n} distinct users (got {len(pool)}); "
            f"pass --distinct-users {n} (seed db/seed_bench_users.sql first)"
        )

    lock = threading.Lock()
    settled = [0]
    connected = [0]
    alive = [0]
    hb_count = [0]
    hb_err = [0]
    drops = []
    lat = []
    errs = []
    targets = {}
    hold_start = [0.0]
    ready = threading.Event()

    def worker(i):
        user = pool[i]
        c = None
        try:
            info = login_one(args.gate, user, args.passwd, args.mail_pattern % user)
            host, port = chat_target(args, info)
            with lock:
                key = f"{host}:{port}"
                targets[key] = targets.get(key, 0) + 1
            c = ChatClient(host, port, info["uid"], info["token"])
            c.connect()
            msg_id, rsp = c.chat_login()
            if rsp.get("error", 1) != 0:
                raise RuntimeError(f"chat_login error={rsp.get('error')}")
            with lock:
                connected[0] += 1
                settled[0] += 1
                if settled[0] == n:
                    hold_start[0] = time.time()
                    ready.set()
        except Exception as e:
            with lock:
                errs.append(f"{user}: {e}")
                settled[0] += 1
                if settled[0] == n:
                    hold_start[0] = time.time()
                    ready.set()
            if c is not None:
                c.close()
            return

        ready.wait(timeout=180)
        deadline = hold_start[0] + args.duration
        while time.time() < deadline:
            t0 = time.perf_counter()
            try:
                c.heartbeat()
                with lock:
                    lat.append(time.perf_counter() - t0)
                    hb_count[0] += 1
            except Exception:
                with lock:
                    hb_err[0] += 1
                    if len(drops) < 5:
                        drops.append(user)
                c.close()
                return
            time.sleep(args.hb_interval)
        with lock:
            alive[0] += 1
        c.close()

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(n)]
    t_start = time.perf_counter()
    for t in threads:
        t.start()
    handshake_ok = ready.wait(timeout=180)
    handshake_wall = time.perf_counter() - t_start
    time.sleep(2)  # let server-side routing state settle before sampling

    redis_line = None
    if args.redis_port:
        try:
            hlen = redis_query(args.redis_host, args.redis_port, "HLEN", "logincount").strip()
            uip = redis_query(args.redis_host, args.redis_port, "KEYS", "uip_*")
            redis_line = (f"redis: logincount_keys={hlen} uip_keys={uip.count('uip_')} "
                          f"self_count={redis_query(args.redis_host, args.redis_port, 'HGET', 'logincount', args.server_name).strip()}")
        except Exception as e:
            redis_line = f"redis: query failed ({e})"

    for t in threads:
        t.join()

    total = time.perf_counter() - t_start
    print(f"sessions: clients={n} handshake_ok={connected[0]} ({connected[0] / max(n, 1) * 100:.1f}%) "
          f"alive_at_end={alive[0]} hb_total={hb_count[0]} hb_err={hb_err[0]} drop={len(drops)}")
    print(f"  handshake_wall={handshake_wall:.3f}s hold={args.duration:.0f}s total_wall={total:.3f}s "
          f"ready={handshake_ok}")
    if drops:
        print(f"  dropped users: {', '.join(drops)}")
    print_targets(targets)
    for e in errs[:3]:
        print(f"  sample error: {e}")
    report_latency(lat)
    if redis_line:
        print(f"  {redis_line}")


def run_msg(args):
    info_a = login_one(args.gate, args.user, args.passwd, args.email)
    info_b = login_one(args.gate, args.peer_user, args.peer_passwd, args.peer_email)
    host_a, port_a = chat_target(args, info_a)
    host_b, port_b = chat_target(args, info_b)
    a = ChatClient(host_a, port_a, info_a["uid"], info_a["token"])
    b = ChatClient(host_b, port_b, info_b["uid"], info_b["token"])
    a.connect()
    b.connect()
    a.chat_login()
    b.chat_login()
    peer_uid = info_b["uid"]
    thread_id = args.thread_id
    if not thread_id:
        _, rsp = a.load_threads()
        privates = [t for t in rsp.get("threads", []) if t.get("type") == "private"]
        for t in privates:
            other = t.get("user2_id") if t.get("user1_id") == a.uid else t.get("user1_id")
            if other == peer_uid:
                thread_id = int(t.get("thread_id", 0))
                break
        if not thread_id:
            _, rsp = a.create_private_chat(peer_uid)
            thread_id = int(rsp.get("thread_id", 0))
    if not thread_id:
        raise RuntimeError("no thread id")

    recv_count = [0]
    stop = threading.Event()

    def receiver():
        while not stop.is_set():
            try:
                mid, rsp = recv_frame(b.sock, timeout=1)
                if mid == IDS["notify_text_chat"]:
                    recv_count[0] += 1
            except socket.timeout:
                continue
            except Exception:
                break

    t = threading.Thread(target=receiver)
    t.start()
    lat = []
    errs = 0
    ack_err = 0
    end = time.time() + args.duration
    sent = 0
    payload = "x" * args.msg_size
    while time.time() < end:
        for _ in range(args.rate):
            t0 = time.perf_counter()
            try:
                _, rsp = a.send_text(peer_uid, thread_id, payload)
                if rsp.get("error", 1) != 0:
                    ack_err += 1
                lat.append(time.perf_counter() - t0)
                sent += 1
            except Exception:
                errs += 1
            if time.time() >= end:
                break
    time.sleep(0.5)
    stop.set()
    t.join(timeout=2)
    b.close()
    a.close()
    wall = args.duration
    loss = sent - recv_count[0]
    print(f"msg: thread_id={thread_id} sent={sent} ack_err={ack_err} io_err={errs} "
          f"sender_qps={sent / max(wall, 1e-9):.1f} recv_count={recv_count[0]} loss={loss}")
    report_latency(lat)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--mode", choices=["login", "connect", "heartbeat", "msg", "sessions"], default="login")
    ap.add_argument("--gate", default="http://localhost:8080")
    ap.add_argument("--chat-host", default=None,
                    help="force a chat host; default = follow the instance assigned at login")
    ap.add_argument("--chat-port", type=int, default=None,
                    help="force a chat port; default = follow the instance assigned at login")
    ap.add_argument("--user", default="user1")
    ap.add_argument("--passwd", default="123456")
    ap.add_argument("--email", default="user1@qq.com")
    ap.add_argument("--peer-user", default="user2")
    ap.add_argument("--peer-passwd", default="123456")
    ap.add_argument("--peer-email", default="user2@qq.com")
    ap.add_argument("--clients", type=int, default=50)
    ap.add_argument("--duration", type=float, default=10)
    ap.add_argument("--rate", type=int, default=10)
    ap.add_argument("--msg-size", type=int, default=32)
    ap.add_argument("--thread-id", type=int, default=0)
    ap.add_argument("--hb-interval", type=float, default=5, help="sessions mode heartbeat interval (s)")
    # distinct-user pool (needed for concurrent logins / concurrent sessions)
    ap.add_argument("--distinct-users", type=int, default=0, help="0 = reuse --user for every client")
    ap.add_argument("--user-pattern", default="bench%04d")
    ap.add_argument("--user-start", type=int, default=1)
    ap.add_argument("--users", default="", help="explicit comma separated user list")
    ap.add_argument("--mail-pattern", default="%s@bench.local")
    # server-side verification via Redis
    ap.add_argument("--redis-host", default="127.0.0.1")
    ap.add_argument("--redis-port", type=int, default=0, help="0 = skip redis cross-check")
    ap.add_argument("--server-name", default="chatserver1")
    args = ap.parse_args()

    if args.mode == "login":
        run_login_storm(args)
    elif args.mode == "connect":
        run_connect_storm(args)
    elif args.mode == "heartbeat":
        run_heartbeat(args)
    elif args.mode == "msg":
        run_msg(args)
    elif args.mode == "sessions":
        run_sessions(args)


if __name__ == "__main__":
    main()