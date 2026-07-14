# Mail Relay Blackbox Coverage Plan — OTP (Test 58) + Retry (Test 57)

## Purpose

This document is the detailed, gated implementation plan for adding **blackbox
(launch-time) coverage** for two Mail Relay source files that currently have no
production callers / no integrated path exercised by blackbox tests:

- `elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_otp.c` — only consumed by unit tests today; no
  production caller. Target blackbox: **Test 58** (`tests/test_58_mailrelay_api.sh`).
- `elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_retry.c` — only reached through
  `elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_workers.c` on a real SMTP send failure, which Test 57
  never triggers (its single `SendRawOnLaunch` send succeeds). Target blackbox:
  **Test 57** (`tests/test_57_mailrelay_outbound.sh`).

The strategy is **launch-time test seams** — config-gated knobs that drive a real
OTP send+verify and a forced transient SMTP failure at startup, so the integrated
code paths run inside the existing blackbox harness. No product behavior changes
when the seam flags are unset.

**Scope of THIS document:** design + step-by-step implementation. The actual
Test 57 / Test 58 script edits are intentionally deferred to a **separate task**;
this plan defines exactly what that task must do.

---

## Current State (as of 2026-07-14)

### Done (context, not in scope here)

- OTP unit tests renamed to convention and extended: `mailrelay_otp_test_generate.c`,
  `mailrelay_otp_test_send.c`, `mailrelay_otp_test_verify.c` (11 / 14 / 23 cases,
  all green). Unity coverage of `mailrelay_otp.c`: **95.50% lines / 97.04% branches**
  of 378 / 338. Remaining 17 lines are irreducible defensive floor (uncovered
  helper error fallbacks).
- `mailrelay_retry.c` unity coverage: **79.49%**. Blackbox will not target the
  uncovered edge branches (already hit by unit tests); it adds integrated
  worker-path confidence.

### Gaps this plan closes

| Source | Caller today | Blackbox path today | After this plan |
|---|---|---|---|
| `mailrelay_otp.c` (`generate_and_send`, `verify`) | none | none | launched by `SendOtpOnLaunch` seam → Test 58 |
| `mailrelay_retry.c` | `mailrelay_workers.c` on failure | never (Test 57 send always succeeds) | launched by `FailNextSendOnLaunch` seam → Test 57 |
| `mailrelay_workers.c` retry wrapper (lines ~81–118) | production | never exercised by Test 57 | exercised via forced failure |

### Relevant infrastructure already present (verified)

- OTP table + `auth.otp_code` template exist in Helium/Acuranzo migrations:
  - `HELIUM_ROOT=/mnt/extra/Projects/Philement/elements/002-helium`
  - QTC query IDs: `MAILRELAY_QREF_OTP_INSERT=112`, `_GET_ACTIVE=113`, `_CONSUME=114`.
  - Template seeds: `acuranzo_1217.lua`, `acuranzo_1261.lua` (key `auth.otp_code`).
- `MAIL_OTP_TEMPLATE_KEY = "auth.otp_code"` (`elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_otp.h:43`).
- Existing SMTP transport seam for injecting failures:
  - `mailrelay_smtp_set_transport(fn)` / `mailrelay_smtp_reset_transport()`
    (`elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_smtp.h:65,68`). Unit tests use a `mock_transport`
    with `g_fail_count` / `retryable` flags.
- OTP executor mock contract: repo params nested as
  `{"STRING":{...},"INTEGER":{...}}`; executor returning `false` (no callback)
  triggers empty-error fallback branches (lines 280, 542, 593/618/620/638).
- Config already has a precedent for a launch seam: `SendRawOnLaunch`
  (`config_mail_relay.h` ~line 53, `config_mail_relay.c` ~lines 125, 212–215, 449;
  `config_defaults.c` ~lines 419–420; wired in `launch_mail_relay.c` ~lines 267–298).

---

## Seam Design

### Seam A — OTP send + self-verify (drives Test 58)

**Config flag:** `config->mail_relay.Test.SendOtpOnLaunch` (bool).
**Behavior at launch** (guarded, default OFF):

1. Build an OTP request for a fixed test recipient + a deterministic test purpose.
2. Call `mailrelay_otp_generate_and_send(...)` → sends via the templated path
   (`auth.otp_code`) through the real worker/sink.
3. Immediately call `mailrelay_otp_verify(...)` with the code returned by step 2,
   against the same recipient/purpose.
4. Emit log markers:
   - `"MAILRELAY_OTP_LAUNCH_SENT"` (with masked recipient) on successful send.
   - `"MAILRELAY_OTP_LAUNCH_VERIFIED"` on successful verify.
   - `"MAILRELAY_OTP_LAUNCH_VERIFY_FAILED"` / `"MAILRELAY_OTP_LAUNCH_SEND_FAILED"`
     on the respective failure (so the test can assert negative cases too).
5. **Determinism:** force the OTP code to a known value for the launch seam
   (the unit tests already exercise random generation; blackbox only needs a
   stable, verifiable code). A helper `mailrelay_otp_set_fixed_code()` (or a
   launch-only flag) makes the verify step hermetic regardless of RNG.

### Seam B — forced transient SMTP failure (drives Test 57)

**Config flag:** `config->mail_relay.Test.FailNextSendOnLaunch` (bool).
**Behavior at launch** (guarded, default OFF):

1. Before the existing `SendRawOnLaunch` send, install a launch-scoped transport
   that fails its first N attempts with `retryable=true`, then succeeds
   (mirrors the unit-test `mock_transport` contract via `mailrelay_smtp_set_transport`).
2. Let the normal `SendRawOnLaunch` path run through `mailrelay_workers.c`; the
   worker hit `mailrelay_retry.c`, observe the transient failure, back off, and
   retry, eventually succeeding.
3. Emit log markers:
   - `"MAILRELAY_LAUNCH_SEND_RETRY"` on each retry attempt.
   - `"MAILRELAY_LAUNCH_SEND_OK"` once the injected transport succeeds.
4. Restore default transport (`mailrelay_smtp_reset_transport()`) after the
   launch send so normal runtime sending is unaffected.
5. **Determinism:** test config sets small `mail_relay.retry.InitialDelaySeconds`
   and `MaxDelaySeconds` so the retry finishes within the blackbox timeout.

---

## Implementation Steps

### Step 1 — Config struct + defaults (no behavior change)

- `elements/001-hydrogen/hydrogen/src/config/config_mail_relay.h`: extend `MailRelayTest` struct with
  `bool SendOtpOnLaunch;` and `bool FailNextSendOnLaunch;` (place next to the
  existing `SendRawOnLaunch` member, ~line 53).
- `src/config/config_defaults.c` (~lines 419–420): initialize both to `false`.
- `elements/001-hydrogen/hydrogen/src/config/config_mail_relay.c`:
  - init (~line 125) set both to `false`;
  - parse (~lines 212–215 area) read keys `Test.SendOtpOnLaunch`,
    `Test.FailNextSendOnLaunch`;
  - dump (~line 449 area) print both.
- **Verify:** `mkt`, `mkp`.

### Step 2 — OTP deterministic-code helper (hermetic verify)

- In `elements/001-hydrogen/hydrogen/src/mailrelay/mailrelay_otp.c` / `.h`: add `mailrelay_otp_set_fixed_code(const char* code)`
  and a `mailrelay_otp_clear_fixed_code()`. When set, `generate_and_send` uses the
  fixed code instead of RNG (all-zero-bytes random still maps to `"000000"`, but
  launch must be RNG-independent). Guard behind the same internal flag used by
  tests so it is inert in production.
- **Verify:** `mkt`, `mkp`; optionally extend `mailrelay_otp_test_generate.c` with
  one case for the fixed-code path (separate task / optional).

### Step 3 — Launch wiring

- `elements/001-hydrogen/hydrogen/src/launch/launch_mail_relay.c`, near the existing `SendRawOnLaunch` block
  (~lines 267–298):
  - If `FailNextSendOnLaunch`: install the failing-then-succeeding transport via
    `mailrelay_smtp_set_transport(...)` + log markers before the send; restore
    after (`mailrelay_smtp_reset_transport()`).
  - If `SendOtpOnLaunch`: call `mailrelay_otp_set_fixed_code("<known>")`,
    `mailrelay_otp_generate_and_send(...)`, capture code, `mailrelay_otp_verify(...)`
    with the known code, emit the seam-A log markers, then
    `mailrelay_otp_clear_fixed_code()`.

  Order note: if both flags set, run the retry seam first (transient transport)
  then the OTP seam (default transport) so OTP send is not accidentally failed.
- **Verify:** `mkt`, `mkp`.

### Step 4 — Test config JSON (per test)

- Test 58 config: set `mail_relay.Test.SendOtpOnLaunch=true`; real DB + sink +
  `auth.otp_code` template present (already the case per Test 58 harness).
- Test 57 config: set `mail_relay.Test.FailNextSendOnLaunch=true` and small retry
  delays; keep `SendRawOnLaunch=true` so one send is attempted.
- **Verify:** `mkt`, `mkp`.

### Step 5 — Test scripts (DEFERRED to separate task — specified here)

**Test 58 (`tests/test_58_mailrelay_api.sh`):**

- Add a variant (or new case) that launches with `SendOtpOnLaunch=true`.
- Assert log contains `MAILRELAY_OTP_LAUNCH_SENT` and `MAILRELAY_OTP_LAUNCH_VERIFIED`.
- Assert DB row for the test recipient/purpose consumed (`_CONSUME` path) — via
  QTC query 114 or a direct check.
- Update `docs/H/tests/test_58_mailrelay_api.md` with the new case + assertions.

**Test 57 (`tests/test_57_mailrelay_outbound.sh`):**

- Add a variant that launches with `FailNextSendOnLaunch=true` (and small retry
  delays).
- Assert log contains `MAILRELAY_LAUNCH_SEND_RETRY` (≥1) AND
  `MAILRELAY_LAUNCH_SEND_OK` (eventual success).
- Assert the outbound message was ultimately delivered/sunk (existing sink check).
- Update `docs/H/tests/test_57_mailrelay_outbound.md` with the new case + assertions.

### Step 6 — Coverage validation

- `extras/add_coverage.sh` lives at
  `elements/001-hydrogen/hydrogen/extras/add_coverage.sh` and takes a **path
  relative to that hydrogen dir** (e.g. `src/mailrelay/mailrelay_otp.c`). It
  diffs `build/unity/src/<path>.c.gcov` against `build/coverage/src/<path>.c.gcov`
  and prints lines uncovered in BOTH — so both a unity run and a coverage
  (blackbox) run must have produced gcov first.
- Workflow:
  1. `mkt` (coverage config), run the unity mailrelay suite to refresh
     `build/unity/.../*.gcov`.
  2. Run Test 57 + Test 58 (coverage build) to refresh
     `build/coverage/.../*.gcov`.
  3. `cd elements/001-hydrogen/hydrogen && ./extras/add_coverage.sh src/mailrelay/mailrelay_otp.c`
     — confirm the previously-uncovered launch lines are now covered by the
     blackbox run (i.e. no longer in the "uncovered in both" output).
  4. `./extras/add_coverage.sh src/mailrelay/mailrelay_retry.c` — confirm the
     worker integrated path now shows coverage (combined with the 79.49% unity
     baseline).
- **Verify:** `mkt`, `mka`, `mkp`, both blackbox tests green.

### Step 7 — Docs / changelog

- Update `docs/H/plans/MAILRELAY_PLAN.md` "Resume here" block to point at this
  plan once seams land.
- If the repo keeps a CHANGELOG, add an entry for the two new test seams
  (config flags + log markers), clearly marked as test-only / inert by default.
- Run SITEMAP / test_04 doc-hygiene if the project's doc-lint expects new
  test docs registered.

---

## Log Markers (single source of truth)

| Marker | Seam | Meaning |
|---|---|---|
| `MAILRELAY_OTP_LAUNCH_SENT` | A | OTP generated + sent via real worker/sink |
| `MAILRELAY_OTP_LAUNCH_VERIFIED` | A | Self-verify of the launched OTP succeeded |
| `MAILRELAY_OTP_LAUNCH_SEND_FAILED` | A | Send step failed (negative-case assert) |
| `MAILRELAY_OTP_LAUNCH_VERIFY_FAILED` | A | Verify step failed (negative-case assert) |
| `MAILRELAY_LAUNCH_SEND_RETRY` | B | A transient SMTP failure triggered a retry |
| `MAILRELAY_LAUNCH_SEND_OK` | B | Injected transport eventually succeeded |

## Critical Constraints

- Both seam flags default OFF; no runtime behavior change when unset.
- No production caller is added for OTP beyond the launch seam (consistent with
  "prep for keycloak" intent).
- C changes must pass `mkp` (cppcheck). Add
  `// cppcheck-suppress nullPointerOutOfMemory` + `TEST_ASSERT_NOT_NULL` for any
  OOM-guarded pointers in new/extended test code.
- Do NOT modify CMake (`file(GLOB_RECURSE UNITY_TEST_SOURCES ".../*_test*.c")`
  already matches both `<source>_test.c` and `<source>_test_<fn>.c`).

## Exit Gate

- `mkt`, `mka`, `mkp` green.
- Test 57 + Test 58 pass with the new variants and all listed log/DB assertions.
- `extras/add_coverage.sh` shows gcov entries for both `mailrelay_otp.c` and
  `mailrelay_retry.c` from the blackbox runs (run from
  `elements/001-hydrogen/hydrogen/`, path `src/mailrelay/<file>.c`).
- Docs + (optional) CHANGELOG updated; plan linked from `MAILRELAY_PLAN.md`.

## Deferred to separate task

- All edits to `tests/test_57_mailrelay_outbound.sh`,
  `tests/test_58_mailrelay_api.sh`, and their `.md` docs (Step 5).
- Optional: unit case for the fixed-code OTP path (Step 2 extension).
