# Conduit Cap Query Blackbox Test

Test: [`test_56_cap_query.sh`](/elements/001-hydrogen/hydrogen/tests/test_56_cap_query.sh)

Exercises `POST /api/conduit/cap_query`. The script header sets `TEST_NAME` to Conduit Cap Query, `TEST_ABBR` to CCQ, and `TEST_VERSION` to 1.3.2.

## What It Does

1. Launches Hydrogen with [`hydrogen_test_56_cap_query.json`](/elements/001-hydrogen/hydrogen/tests/configs/hydrogen_test_56_cap_query.json). `WebServer.Port` is 5560, so this server does not share port 5500 with tests 50 and 51 in the parallel 5x group.
2. Sends two negative requests on that first launch. The environment's `CHACHA_SERVER` is left as it is.
3. Stops that server.
4. Starts a second launch with `CHACHA_SERVER` set to `http://127.0.0.1:1`, so siteverify fails and the fallback path runs. The previous `CHACHA_SERVER` value is restored afterward.
5. Sends the protected INSERT cases from `PROTECTED_CAP_QUERY_REFS` in [`conduit_utils.sh`](/elements/001-hydrogen/hydrogen/tests/lib/conduit_utils.sh).

## Cases

Negative requests use `database` `Demo_PG` and `query_ref` 1. Both must return HTTP 400.

| Case | Body | `error` |
| --- | --- | --- |
| Missing Token | `{"query_ref":1,"database":"Demo_PG"}` | `CAP_TOKEN_MISSING` |
| Invalid Token | `{"query_ref":1,"database":"Demo_PG","chachaToken":"invalid-token"}` | `CAP_VERIFY_FAILED` |

Positive requests are posted to the fallback server against `Demo_PG`. Each body adds `chachaToken` `test-token` and `chachaSite` `a9b8369d3b`, plus the parameter object stored on that QueryRef.

| QueryRef | Name |
| --- | --- |
| 4000 | Insert Course Suggestion |
| 86 | Insert Contact Submission |

A positive response must be HTTP 200 with `success` true, a row id (`suggestion_id` or `submission_id`), `cap_fallback` true, and `queue_used` `slow`.

The config lists eight demo connections (`Demo_PG`, `Demo_MY`, `Demo_SQL`, `Demo_DB2`, `Demo_MR`, `Demo_FB`, `Demo_YB`, `Demo_MS`). These cases address `Demo_PG`. QueryRef 4000 and QueryRef 86 must already be registered there. Changelog 1.3.2 records that the course suggestion path uses QueryRef 4000.

## Prerequisites

- A built Hydrogen binary (`hydrogen`, `hydrogen_debug`, `hydrogen_release`, or `hydrogen_coverage`) under the project root.
- `HYDROGEN_DEMO_USER_NAME`, `HYDROGEN_DEMO_USER_PASS`, and `HYDROGEN_DEMO_API_KEY`.
- `CHACHA_SERVER`, `CHACHA_SITEID`, and `CHACHA_SECRET`. The script warns when any of those is unset: the negative cases still run, and the positive cases fail.
- `jq` and `curl`.

## Running

```bash
# From the project root (HYDROGEN_ROOT defined)
zsh -ic 'mkt'   # ensure a current Hydrogen binary exists
bash tests/test_56_cap_query.sh
```

## Related

- Endpoint: [`cap_query.md`](/docs/H/api/conduit/cap_query.md)
- Fixture map: [`conduit_utils.sh`](/elements/001-hydrogen/hydrogen/tests/lib/conduit_utils.sh)
- Suite index: [`TESTING.md`](/docs/H/tests/TESTING.md)
