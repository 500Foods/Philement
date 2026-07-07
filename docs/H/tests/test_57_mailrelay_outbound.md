# Mail Relay Outbound Blackbox Test

Test: `test_57_mailrelay_outbound.sh`

Validates Hydrogen Mail Relay outbound delivery end-to-end without Python, using our own C mail validator (`extras/mailval`) as an SMTP sink.

## What It Does

1. Starts the `mailval` SMTP sink (prebuilt binary) listening on `127.0.0.1`.
2. Starts Hydrogen with a config that enables `MailRelay.Test.SendRawOnLaunch`, which fires one synchronous raw send during launch.
3. Asserts Hydrogen logged `SendRawOnLaunch success` and that the sink captured exactly one delivered message (`stored_uid=yes`) with the expected subject.
4. Stops Hydrogen and the sink cleanly.

## Transport Variants

| Variant | Port | Transport | Notes |
| --- | --- | --- | --- |
| Plaintext | 5570 | `smtp://` | No TLS; fastest path. |
| STARTTLS | 5571 | `smtp://` + STARTTLS | Self-signed cert from `extras/mailval`; CA pinned via `Servers[0].CAPath`. |

## Config Files

- `tests/configs/hydrogen_test_57_mailrelay_outbound.json` (plaintext)
- `tests/configs/hydrogen_test_57_mailrelay_outbound_tls.json` (STARTTLS)

Secrets (SMTP password, CA cert path) are injected via `${env.*}` resolved by the config loader, so no credentials live in the committed configs.

## Prerequisites

- A built Hydrogen binary (`hydrogen`, `hydrogen_debug`, `hydrogen_release`, or `hydrogen_coverage`) under the project root.
- A prebuilt `mailval` binary at `extras/mailval/build/mailval`. Build it with:

```bash
cd extras/mailval && cmake -S . -B build && cmake --build build
```

- The self-signed cert/key for the TLS variant is generated on demand by `extras/mailval/gen_cert.sh` if missing. The private key must never be committed or logged.

## Running

```bash
# From the project root (HYDROGEN_ROOT defined)
zsh -ic 'mkt'   # ensure a current Hydrogen binary exists
bash tests/test_57_mailrelay_outbound.sh
```

## Assertions

- Hydrogen reaches `STARTUP COMPLETE` and logs `SendRawOnLaunch success`.
- The sink writes a `mailval_smtp_*.json` transcript whose `stored_uid` metadata is `yes` and whose `subject` matches the canned test subject.

## Related

- Plan: [Mail Relay Subsystem Implementation Plan](/docs/H/plans/MAILRELAY_PLAN.md)
- Sink source: `/elements/001-hydrogen/hydrogen/extras/mailval/`
- Phase 2 exit gate: one configured raw outbound email to a local C SMTP sink.
