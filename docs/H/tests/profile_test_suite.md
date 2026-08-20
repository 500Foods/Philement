# Profile Test Suite Script Documentation

## Overview

The `profile_test_suite.sh` script profiles Hydrogen test scripts under `strace`. It counts successful `execve` calls by basename, groups them with the project `tables` executable, and writes dated trace/summary/error files.

## Script Information

- **Script**: `/elements/001-hydrogen/hydrogen/tests/profile_test_suite.sh`
- **Version**: 1.2.2
- **Purpose**: Bounded execve profiling of the suite or a single test
- **Dependencies**: strace, GNU timeout, awk, jq, grep, date; `tables` for the summary (plain-text fallback if missing)

## Usage

```bash
./profile_test_suite.sh                       # full suite (test_00_all.sh)
./profile_test_suite.sh test_40_auth.sh       # one test
./profile_test_suite.sh --timeout 600 ...     # hard cap in seconds (default 1800)
./profile_test_suite.sh --skip-email ...      # skip make-email/mutt
./profile_test_suite.sh --mono ...            # no ANSI colours
./profile_test_suite.sh --help
```

`PROFILE_TIMEOUT` overrides the default cap. `0` disables it (unbounded; not recommended).

## Output Files

Dated so a later day does not clobber an earlier run. Same-day re-runs overwrite that day's files:

- `profile_trace-YYYYMMDD.txt`: raw strace (`-f -z -e execve`)
- `profile_summary-YYYYMMDD.txt`: tables summary (mono archive)
- `profile_error-YYYYMMDD.txt`: diagnostics and strace stderr

Legacy undated `profile_trace.txt` / `profile_summary.txt` / `profile_error.txt` are removed at start.

## How It Works

1. Resolve the target script (cwd, `./name`, or `tests/`).
2. Run `timeout -k 10 N strace -f -z -s 256 -e trace=execve` on it. GNU timeout process-group kills strace and every tracee when the cap fires. That is the only reliable stop for `strace -f`.
3. `-z` keeps successful execve only, so PATH-walk `ENOENT` probes do not inflate bash/sh/Uncategorized.
4. One awk pass maps each path basename through a catalog (plus GNU aliases such as `gawk` → `awk`).
5. `tables` renders Command + Count with a hidden Category break column. Zeros are blank. Zero-count rows are annotated so Command `summary: "count"` is the number of non-zero rows and Count `summary: "sum"` is total execve. Column widths are omitted so the table, title, and footer auto-size.

## Command Categories

Catalog groups (zeros are listed so missing tools stay visible):

- **Hydrogen**: `hydrogen` and `hydrogen_*` variants
- **Shell**: bash, sh, dash, zsh, xargs, `*.sh` (test scripts and helpers)
- **SysUtils**: cat, find, date, printf, env, …
- **PathTools**: mkdir, rm, cp, realpath, …
- **TextTools**: grep, sed, awk, jq, curl, …
- **Build**: cmake, ninja, cc, gcov, …
- **Lint**: cppcheck, shellcheck, jsonschema-cli, …
- **Reporting**: cloc, tables, Oh, lua, mutt, …
- **Process**: flock, sleep, timeout, kill, …
- **DB**: sqlite3, psql, mysql, mariadb
- **Misc**: python3, node, perl, addto, mailval
- **Uncategorized**: anything not in the catalog (listed with counts in the error log)

## Tables Layout

The summary uses the project `tables` binary (Blue theme), not hand-drawn separators:

- Hidden `category` column with `break: true` draws group rules
- Count is `datatype: num` with thousands separators; zeros render blank (tables default)
- Zero-count rows set `"annotate": true` so they still appear but are excluded from summaries
- Command `summary: "count"` is the number of non-zero rows; Count `summary: "sum"` is total execve
- No fixed `width` on columns; title and footer size to content
- Title is `basename @ HH:MM:SS` (for example `test_18_signals.sh @ 11:30:28`)
- Footer is duration (and “partial trace” if the timeout fired)

## Timeout

`strace -f` follows every fork. Background processes that outlive the script (mutt on SMTP, orphaned hydrogen servers, `hbm_browser`) keep strace alive until something kills the tree. GNU `timeout -k 10` sends TERM to the process group, then KILL. A timeout is the only reliable bound; there is no clean “suite finished” signal once `-f` is attached.

`--skip-email` sets `HYDROGEN_DISABLE_EMAIL=1` so `make-email.sh` / mutt are not started.

## Troubleshooting

- **strace not found**: install strace in `/usr/bin`
- **timeout not found**: GNU coreutils `timeout` is required
- **Permission denied**: target script must be executable
- **Partial trace**: timeout fired; summary is from whatever was flushed
- **Large Uncategorized bucket**: see `Uncategorized execve counts` in the error log and add catalog rows if they are real tools

## Related Documentation

- [Test 00 All](/docs/H/tests/test_00_all.md) - Main test orchestration script
- [Framework Library](/docs/H/tests/framework.md) - Test framework utilities
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Complete library documentation index
