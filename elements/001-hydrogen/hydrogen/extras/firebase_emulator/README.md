# Firestore Emulator

This folder installs and starts the **Cloud Firestore emulator** for local
Hydrogen work (Test 37 later). It is **not** a UDF extra.

Firestore cannot load C extensions the way PostgreSQL, MySQL, SQLite, and
DB2 do. Base64, Brotli, SHA-256, JSON ingest/extract, and timezone
conversion for the firebase engine run **in-process in Hydrogen**
(`src/database/firebase/fns_*.c`). There is no `brotli_udf_firebase/`
and this folder does not install Cloud Functions.

## Requirements

- Node.js 20+ and npm (to install the pinned `firebase-tools` here)
- Java 21+ JRE on `PATH` (the Firestore emulator is a JVM app)
- `curl` and `jq` (`start.sh` waits for REST and reads `firebase.json`)

No Google account. Do not run `firebase login`. The emulator project id
is `hydrodemo`.

## One-time install

```bash
cd extras/firebase_emulator
npm install
```

`start.sh` runs `npm install` in this folder if
`node_modules/.bin/firebase` is missing. The first start may also
download the Firestore emulator JAR into `~/.cache/firebase/emulators/`
(can take a few minutes).

## Start / stop

```bash
./start.sh
./stop.sh
```

- `start.sh` is idempotent: if this folder already started the emulator
  and `:8080` answers, it exits 0.
- `start.sh` fails if `:8080` is already in use by something else.
- `stop.sh` stops **only** the process this folder started (PID file).
  It will not kill a foreign listener on `:8080`.

The emulator UI is **disabled** (Firebase's default UI port is 4000).
This extra binds **only** Firestore on `127.0.0.1:8080`.

REST base (emulator, no auth):

```text
http://127.0.0.1:8080/v1/projects/hydrodemo/databases/(default)/documents/
```

## Ports

| Service | Bind | Notes |
| --- | --- | --- |
| Firestore emulator | `127.0.0.1:8080` | Only port this extra opens |
| Emulator UI | off | `emulators.ui.enabled = false` in `firebase.json` |

## Environment

Names match [SECRETS.md](/docs/H/SECRETS.md) section 8. Values are not
secrets on the emulator. If you override host/port, keep them in sync
with `firebase.json`.

| Variable | Default | Meaning |
| --- | --- | --- |
| `FIREBASE_PROJECT` | `hydrodemo` | GCP project id (`User` in Hydrogen config) |
| `FIREBASE_EMULATOR_HOST` | `127.0.0.1` | Emulator host (`Host`) |
| `FIREBASE_EMULATOR_PORT` | `8080` | Emulator port (`Port`) |
| `FIREBASE_SA_JSON` | empty | Service-account JSON **path**; empty on emulator (`Pass`) |
| `FIREBASE_EMULATOR_WAIT` | `300` | Seconds `start.sh` waits for REST |

`FIREBASE_SA_JSON` is unused by these scripts. Empty `Pass` plus
`firestore.googleapis.com` must fail closed in the C engine (Phase 3);
do not point this extra at production.

## What you do not install here

- C UDFs, Cloud Functions, Firebase Auth, Realtime Database, FCM
- The Firebase C++ SDK or gRPC
- A Google service-account JSON (emulator REST has no auth)
