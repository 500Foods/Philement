# Mail Relay Inbound SMTP Blackbox Test

Test: [`test_61_mailrelay_inbound.sh`](/elements/001-hydrogen/hydrogen/tests/test_61_mailrelay_inbound.sh)

End-to-end inbound SMTP. Hydrogen accepts one message on its listener and the outbound path delivers it to the local `mailval` sink. The script header sets `TEST_NAME` to MailRelay Inbound SMTP Listener, `TEST_ABBR` to MRI, and `TEST_VERSION` to 1.0.4.

## What It Does

1. Locates the Hydrogen binary and the `mailval` binary.
2. Validates [`hydrogen_test_61_mailrelay_inbound.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_61_mailrelay_inbound.json).
3. Starts `mailval` on `127.0.0.1:5580`. Transcripts go to a diagnostics directory. `MAILVAL_BIN` overrides the default binary path.
4. Exports `MAILRELAY_INBOUND_TEST_MODE=1` and starts Hydrogen with the config file as a positional argument.
5. Waits until the log contains `STARTUP COMPLETE` and `127.0.0.1:5581` accepts TCP, with a 60-second timeout.
6. Opens an SMTP dialogue on `/dev/tcp`: greeting, `EHLO testrunner.example.com`, `MAIL FROM:<alice@testrunner.example.com>`, `RCPT TO:<bob@example.com>`, `DATA`, a From/To/Subject body, the terminating dot, and `QUIT`.
7. Treats a final response whose first three characters are `250` as acceptance.
8. Polls `mailval` JSON transcripts, up to 100 times at 0.2 seconds apart. A delivery is a `*.json` file whose `.commands[]` entry has `.text` `DATA` and `.dir` `command`.
9. Sends `SIGINT`, then `SIGKILL` if Hydrogen is still running after about 30 seconds.

## Listeners

| Role | Address |
| --- | --- |
| `mailval` sink | `127.0.0.1:5580` |
| Inbound SMTP | `127.0.0.1:5581` |

`WebServer.EnableIPv4` and `WebServer.EnableIPv6` are false in this config.

## Config

`ServerName` is `hydrogen-test-61`. Mail Relay has outbound and inbound enabled, `InboundRequireAuth` false, `ListenPort` 5581, and one worker. `DefaultFrom` is `mailrelay-test@hydrogen.local`. `Queue.Persist` is false. `Servers[0]` is `127.0.0.1` port `5580`, plaintext (`UseTLS` false, `TLSMode` 0, `AuthMode` 0), with `TimeoutSeconds` 15.

## Prerequisites

- A built Hydrogen binary (`hydrogen`, `hydrogen_debug`, `hydrogen_release`, or `hydrogen_coverage`) under the project root.
- A prebuilt `mailval` binary at `extras/mailval/build/mailval`. Build it with:

```bash
cd extras/mailval && cmake -S . -B build && cmake --build build
```

- `extras/mailval/gen_cert.sh` runs when `mailval.pem` or `mailval.key` is missing. This plaintext case does not use that certificate for the sink session.

## Running

```bash
# From the project root (HYDROGEN_ROOT defined)
zsh -ic 'mkt'   # ensure a current Hydrogen binary exists
bash tests/test_61_mailrelay_inbound.sh
```

## Assertions

The script prints these subtests. The framework owns `TEST_COUNTER`.

- Locate Hydrogen Binary
- Locate mailval Binary
- Validate Configuration File
- Start mailval SMTP sink
- Start Hydrogen with inbound SMTP listener
- Inject SMTP message through inbound listener
- Wait for message delivery to mailval sink
- Shutdown Hydrogen
- Test Summary

## Related

- Plan: [Mail Relay Subsystem Implementation Plan](/docs/H/plans/complete/MAILRELAY_PLAN_COMPLETE.md)
- Outbound test: [Mail Relay Outbound Blackbox Test](/docs/H/tests/test_57_mailrelay_outbound.md)
- API test: [Mail Relay API Blackbox Test](/docs/H/tests/test_58_mailrelay_api.md)
- Sink source: [`/elements/001-hydrogen/hydrogen/extras/mailval/`](/elements/001-hydrogen/hydrogen/extras/mailval/)
- Suite index: [`TESTING.md`](/docs/H/tests/TESTING.md)
