# PLANNING.md

How a phased plan is written, and how one phase is worked. This file is
the repo default. [`/AGENTS.md`](/AGENTS.md) points here. A plan restates
the steps that apply to it. When a plan's own close-out is thinner than
this file, follow this file.

A small fix with no plan file follows the element's own guide:
[`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md),
[`/docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md), or
[`/elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md).

## Session brief

A new conversation may start with only a pointer to the plan.

1. Read the plan's Resume or pause point, the phase-index Status column,
   and the previous phase's Status, Accomplished, Lessons learned, and
   Handoff.
2. Re-read only the next incomplete phase: Goal, Work items, Done means,
   and Exit gate. Open a lock when that phase cites it.
3. The locks and the current phase body are the contract. A Working Log
   entry keeps the numbers and the decisions of the day it was written.
4. Resume and the Priority band have to agree. When the plan is waiting
   on another plan, Resume says so. A line that says the next phase can
   start means the previous gate is green and the owner has pulled this
   plan in.
5. A phase whose own entry gate says earlier phases may still be open
   follows that gate. Every other phase waits until the previous phase
   Status is complete and its Exit gate is green.
6. Ask the qualifying questions. Wait for approval before editing source
   on a phase that has not been approved.
7. Do that phase only. Then stop. The next phase starts in a new
   conversation.

## Opening a phase

1. Confirm the prior phase is done. Re-read its Status and Exit gate.
   Memory of an earlier session is not the check.
2. Discuss the phase before editing. Ask qualifying questions first.
   Ask when a lock names a behavior and leaves the stored string, the
   column, the units, or the order unpinned.
3. Wait for explicit approval before editing source.
4. Ask again when a requirement is ambiguous during the work. Leave a
   schema change, a stored string, and a column name for the owner to
   confirm.

## Closing a phase

Do this before marking the phase complete:

1. The named verification command has passed. Intent to verify is not
   verification. A packet phase ends when the packet is handed over.
   It is complete only after the owner reports the apply. The agent
   prepares the packet and does not run SchemaTool or SchemaHelper
   apply.
2. Write **Status**: date, result, variances, and any file number,
   QueryRef, or lookup id that was assigned.
3. Write **Accomplished**: what landed, with paths and the commands
   that passed.
4. Write **Lessons learned**, including the small ones.
5. Write **Handoff**: what the next phase should re-check, what is
   waiting on a person, and any tip that would be expensive to
   rediscover.
6. Append the **Working Log**.
7. Stop. The next phase starts in a new conversation.

Check a work item `[x]` only after that item's verification passed.
Defer with `[~]`, one line, and the phase it moves to.

A sitting that ends before Done means sets Status to **in progress**,
names the last finished work item under Accomplished, and puts the
resume point in Handoff. Leave unfinished items unchecked.

Add Accomplished, Lessons learned, and Handoff when the phase is
worked. Leave them off phases that have not started.

## What a phase contains

Goal. Entry gate. Work items. Done means. Exit gate. Status. After
work has started: Accomplished, Lessons learned, and Handoff.

Done means is one testable sentence. The Exit gate names the command
or the human report that proves it.

## Numbers and migrations

Re-check the next free file, QueryRef, and lookup id on disk before
numbering anything. A snapshot in the plan goes stale. The migration
number, `query_id`, and a caller-facing QueryRef are three different
values. See
[`/elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md).

After a migration edit, regenerate the payload (`payload-generate.sh`
or `zsh -ic 'mka'`) before tests 30–38 or 71. `mkt` does not refresh
the archive.

## Verification

| Change | Check |
| --- | --- |
| Hydrogen C | `zsh -ic 'mkt'`, then `zsh -ic 'mkp'` |
| Shell | `zsh -ic 'mks'` |
| Lithium JS / CSS | `npm test`, `npm run lint`, `npm run lint:css` from `elements/003-lithium` |
| Designer UI | Browser pass of the phase's flow. A screenshot is not the gate |
| Helium packet | Hand it over. Test 31 and Test 98 when asked |
| Markdown | Absolute links rooted at `/docs/...` or `/elements/...`. No `:line` references. `zsh -ic 'mkl'` |
