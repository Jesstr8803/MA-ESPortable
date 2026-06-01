#!/usr/bin/env python3
"""ma_probe.py — connect to a Music Assistant server's WebSocket, run a couple
of commands, and dump the raw JSON. Used to capture real response shapes so the
firmware's Tier-2 models match reality (see firmware/tier2-ma-api.md).

Usage:
    pip install websockets
    python ma_probe.py http://192.168.86.37:8095            # no auth
    python ma_probe.py http://192.168.86.37:8095 --token XXX  # if schema>=28

It prints: the ServerInfo message, then the result of
music/albums/library_items (limit 3) and music/artists/library_items (limit 3).
"""
import asyncio, json, sys, argparse

try:
    import websockets
except ImportError:
    sys.exit("pip install websockets   (then re-run)")


def ws_url(base: str) -> str:
    u = base.rstrip("/")
    u = u.replace("https", "wss", 1) if u.startswith("https") else u.replace("http", "ws", 1)
    return u + "/ws"


async def recv_for(ws, message_id, timeout=10.0):
    """Collect result frames for a message_id (handles partial chunks)."""
    chunks = []
    while True:
        raw = await asyncio.wait_for(ws.recv(), timeout)
        msg = json.loads(raw)
        if msg.get("message_id") != message_id:
            continue  # event or other reply; skip
        if "error_code" in msg:
            return {"error": msg}
        chunks.append(msg.get("result"))
        if not msg.get("partial"):
            break
    return chunks[0] if len(chunks) == 1 else chunks


async def cmd(ws, mid, command, **args):
    await ws.send(json.dumps({"message_id": mid, "command": command, "args": args}))
    return await recv_for(ws, mid)


async def main(base, token):
    url = ws_url(base)
    print(f"connecting to {url} ...")
    async with websockets.connect(url, max_size=8_000_000) as ws:
        info = json.loads(await ws.recv())
        print("\n=== ServerInfoMessage ===")
        print(json.dumps(info, indent=2)[:2000])
        schema = info.get("schema_version", 0)
        if token:
            await ws.send(json.dumps({"message_id": "auth", "command": "auth",
                                      "args": {"token": token}}))
            print("\n=== auth ===")
            print(await recv_for(ws, "auth"))
        elif schema >= 28:
            print(f"\n!! schema_version {schema} >= 28 — a --token is required; "
                  "library calls will likely fail without it.")

        for i, (cmd_name, args) in enumerate([
            ("music/albums/library_items", {"limit": 3, "offset": 0}),
            ("music/artists/library_items", {"limit": 3, "offset": 0}),
        ]):
            print(f"\n=== {cmd_name} (limit 3) ===")
            try:
                res = await cmd(ws, f"q{i}", cmd_name, **args)
                print(json.dumps(res, indent=2)[:4000])
            except Exception as e:
                print(f"error: {e}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("server", help="e.g. http://192.168.86.37:8095")
    ap.add_argument("--token", default=None)
    a = ap.parse_args()
    asyncio.run(main(a.server, a.token))
