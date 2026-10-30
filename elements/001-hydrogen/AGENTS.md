# Hydrogen — Agent Guide

Read this before editing Hydrogen. The contract is [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md). That file wins if this one and the contract disagree.

The C tree is [`/elements/001-hydrogen/hydrogen/`](/elements/001-hydrogen/hydrogen/). Docs live in [`/docs/H/`](/docs/H/), outside that tree. A session opened at the Philement root loads the repo [`/AGENTS.md`](/AGENTS.md) and then this file when the user says the work is Hydrogen. A session opened in `elements/001-hydrogen` or `elements/001-hydrogen/hydrogen` loads both at startup. The build directory for `mkt` and the test scripts is `elements/001-hydrogen/hydrogen`.

## What is already loaded

Start Grok in `elements/001-hydrogen/hydrogen` (`cdh`, then launch). That directory's `.grok/rules/` symlinks are injected in full:

- [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md)
- [`/docs/H/tests/TESTING.md`](/docs/H/tests/TESTING.md)
- [`/docs/H/tests/TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md)

Do not open those three again unless you are editing them. A session started at the Philement root does not load the symlinks. Read the three files then.

## What else to read

1. [`/docs/PLANNING.md`](/docs/PLANNING.md) when the task is a phase of a plan. One phase per conversation.
2. [`/docs/H/TODO.md`](/docs/H/TODO.md) when the user asks what to do next.
