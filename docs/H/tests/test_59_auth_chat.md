# Authenticated Chat Blackbox Test

Test: [`test_59_auth_chat.sh`](/elements/001-hydrogen/hydrogen/tests/test_59_auth_chat.sh)

SQLite-only blackbox for authenticated chat. A local Node mock stands in for the LLM. The script header sets `TEST_NAME` to Auth Chat, `TEST_ABBR` to ACH, and `TEST_VERSION` to 1.8.5.

## What It Does

1. Starts the mock at [`tests/lib/mock_llm/server.js`](/elements/001-hydrogen/hydrogen/tests/lib/mock_llm/server.js) and waits for a `READY` line.
2. Copies [`hydrodemo.sqlite`](/elements/001-hydrogen/hydrogen/tests/artifacts/database/sqlite/hydrodemo.sqlite) and points every engine endpoint at `http://127.0.0.1:15901/v1/chat/completions` with provider `openai`.
3. Rewrites [`hydrogen_test_59_auth_chat.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_59_auth_chat.json) into a runtime config: Chat enabled on each connection, WebServer port 15900, WebSocket port 15902, heartbeat `PingIntervalSeconds` 1, and `Chat.RateLimit` enabled at 3 requests per 5 seconds.
4. Sets `CHAT_CACHE_DIR` under the diagnostics directory so the chat LRU cache stays out of the project tree.
5. Starts Hydrogen, waits for `/api/system/health` and `READY FOR REQUESTS`, logs in with the demo user, and mints chat JWTs (`aud` `hydrogen-chat`, role `chat`) with separate `sub` values.
6. Exercises `/api/conduit/auth_chat`, `/api/conduit/auth_chats`, and the chat WebSocket on `/wss`.

## Ports

| Listener | Port |
| --- | --- |
| WebServer | 15900 |
| Mock LLM | 15901 |
| WebSocket | 15902 |

WebSocket chat and heartbeat URLs are `ws://127.0.0.1:15902/wss`. The hydrogen subprotocol and `Authorization: Key ${WEBSOCKET_KEY}` come from `websocat`.

## Checks

The script prints these subtests:

- Prerequisites
- Start mock LLM
- Prepare SQLite + config
- Start Hydrogen
- Login for JWT
- `auth_chat` no auth returns 401
- `auth_chat` wrong method returns 405
- `auth_chat` invalid JSON returns 400
- `auth_chat` missing `messages` returns 400
- `auth_chat` non-chat JWT returns 403
- `auth_chat` stream returns 200 SSE (up to 3 attempts)
- `auth_chat` success via the mock LLM returns 200
- `auth_chat` named engine missing returns 400
- `POST /api/conduit/auth_chat/stream` returns 404
- `auth_chats` no auth returns 401
- `auth_chats` wrong method returns 405
- `auth_chats` invalid JSON returns 400
- `auth_chats` missing `engines` returns 400
- `auth_chats` empty `engines` returns 400
- `auth_chats` too many `engines` returns 400
- `auth_chats` success via the mock LLM returns 200
- `auth_chats` all unknown engines returns 400
- `auth_chats` one valid and one unknown engine returns 200, and the unknown engine is dropped
- WebSocket media upload (`storage_media.c`, `storage_hex.c`)
- `auth_chat` media reference (retrieve and hex-to-binary)
- WebSocket streaming chat (`proxy_mq.c`, `proxy_mc.c`)
- WebSocket `chat_error`
- WebSocket non-stream `chat_done`
- WebSocket heartbeat PING
- Phase 10b per-sub rate limiting
- Summary

Rate limiting uses fresh JWTs with `sub` `3` and `sub` `4`. Four posts from `sub` 3 expect HTTP 200, 200, 200, then 429. The 429 body must have `success` false, `error` `rate_limited`, and `error_code` 4291. A post from `sub` 4 still returns 200. After a 6-second sleep, `sub` 3 returns 200 again.

## Prerequisites

- A built Hydrogen binary under the project root.
- `node`, `sqlite3`, `jq`, `curl`, `openssl`, and `websocat`. The WebSocket steps call `websocat`.
- `HYDROGEN_DEMO_USER_NAME`, `HYDROGEN_DEMO_USER_PASS`, `HYDROGEN_DEMO_API_KEY`, `HYDROGEN_DEMO_JWT_KEY`, `PAYLOAD_KEY`, and `WEBSOCKET_KEY`.
- A `WEBSOCKET_KEY` that fails `validate_websocket_key` is replaced for this run with an ephemeral 64-character key.

## Running

```bash
# From the project root (HYDROGEN_ROOT defined)
zsh -ic 'mkt'   # ensure a current Hydrogen binary exists
bash tests/test_59_auth_chat.sh
```

## Related

- Single-engine API: [`auth_chat.md`](/docs/H/api/chat/auth_chat.md)
- Multi-engine API: [`auth_chats.md`](/docs/H/api/chat/auth_chats.md)
- Plan: [`CHAT_FINALE_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md)
- Suite index: [`TESTING.md`](/docs/H/tests/TESTING.md)
