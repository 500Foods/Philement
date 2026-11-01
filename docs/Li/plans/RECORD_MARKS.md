<!-- markdownlint-disable MD007 MD024 MD013 MD036 -->

# Record marks

**Date:** 2026-10-28
**Status:** Proposed. Not approved. No phase has started.
**Home:** Query Manager first. Annotations uses manager 20. Notifications takes the next free manager id when its phase starts.
**Design:** Acuranzo, the shared product database. Lookups 0–199. Migration files 1000–1999.

Flags, notes, unseen rows, and watches hang off a Lithium grid without new columns on the business tables. The proof is Query Manager, both inboxes, and a login-time Queries badge. Live badge updates come after that and before any second grid. Tickets are a later plan. Course Builder's `flagged` and `annotation` columns stay where they are.

How a phase runs: [`/docs/PLANNING.md`](/docs/PLANNING.md).

## Resume

Wait for approval of this plan. Then start Phase 1. Re-check the next free Acuranzo file, caller-facing QueryRef, and lookup id on disk before numbering anything. A snapshot here goes stale. As of this writing the highest Acuranzo file on disk is `acuranzo_1435.lua`, and Reporting has already spoken for later numbers. Do not reuse them.

## Locks

1. **Subject.** `subject_kind` is the grid `tablePath` (`queries/query-manager`). `subject_key` is the key that grid already uses, one field or several joined with `::`.
2. **Flags are shared.** Many kinds per record. Anyone who can open the module can set or clear any of them. The grid shows at most three icons in catalog order, then a remainder such as `+2`.
3. **Flag history is append-only.** The grid shows only kinds that are on. "Show flag history" lists every set and clear, who, and when.
4. **Flag kinds are a Lookup**, seeded with the starter set below. Someone may add a kind later. A kind that has any event cannot be deleted. Retiring it hides it from the picker and keeps the name for history.
5. **Notes are permanent.** No update and no delete. A correction is a new note. Subject is plain text. Severity is a Lookup. Body is SunEditor HTML. The author is the signed-in account.
6. **The note popup** is a LithiumTable (when, who, severity, subject, newest first) and the selected body rendered as sanitized HTML. SunEditor mounts only while composing.
7. **The parent grid** gets two virtual columns from a follow-up query: flag icons and the note count. Zero notes is a blank cell. List QueryRefs are unchanged. Filters are "has this flag" and "note count is at least N" on the loaded rows.
8. **Unseen is personal.** Login computes the badge. The first visit to a module sets a baseline of now. Opening a record marks it seen. Mark all read moves that baseline to now. Don't track is per module and per user. The row highlight is not a flag icon.
9. **A missing query does not count.** Notes and flag events remain and still appear in the inboxes, labeled with the query ref. Watches stay until removed. The badge only counts queries that still exist.
10. **Watches** are opt-in, per module or per record, for flag, note, save, or all three. The control is the same on every LithiumTable that turns this on. The actor is not notified for their own act.
11. **A notification row** is the record that something needs to go out. Mail and Web Push are out of this plan. Hydrogen's Web Push plan is a later delivery channel.
12. **Writes are QueryRefs.** Lithium sends `INTEGER.USERID` the way other saves do. One action is one caller-facing QueryRef, and that query writes every row the action needs (current flag, event, activity, matching notifications) so a halfway save does not stick. The body may be Lua inside the QueryRef.
13. **Module access is the gate** for this plan. A finer worker role can sit in front of the same queries later.
14. **Other grids stay off** until Phase 13. Query Manager is the only constructor that opts in.

## Starter flag kinds

Edit this list before Phase 1 writes the migration. Order is the catalog order used for the three icon slots. Icons are Font Awesome names the Lithium icon pipeline already accepts.

| # | Name | Icon | Meaning |
| --- | --- | --- | --- |
| 1 | Star | `fa-star` | Marked to find again |
| 2 | Follow-up | `fa-clock` | Come back to this |
| 3 | Review | `fa-glasses` | Needs another pair of eyes |
| 4 | Question | `fa-circle-question` | Something here is unanswered |
| 5 | Blocked | `fa-ban` | Work cannot proceed |
| 6 | Bug | `fa-bug` | Something is wrong |
| 7 | Idea | `fa-lightbulb` | Worth keeping |
| 8 | Waiting | `fa-hourglass` | Waiting on someone else |
| 9 | Discuss | `fa-comments` | Talk before changing it |
| 10 | Reference | `fa-bookmark` | Kept as an example |
| 11 | Urgent | `fa-bolt` | Do this soon |
| 12 | Security | `fa-shield` | Careful with access or secrets |
| 13 | Later | `fa-calendar` | Not this sitting |
| 14 | Broken link | `fa-link-slash` | A reference from here is bad |
| 15 | Checked | `fa-circle-check` | Someone has looked and is satisfied |

## Starter severities

| # | Name | Use |
| --- | --- | --- |
| 1 | Info | Ordinary context |
| 2 | Note | Worth reading, nothing is wrong |
| 3 | Caution | Easy to misuse or to undo |
| 4 | Important | Read this before changing the record |

## Proposed tables

Names are proposals. Confirm them at the start of the phase that creates them.

| Table | Holds |
| --- | --- |
| `record_flags` | Kinds that are on. Unique on subject plus kind. Clearing deletes the row. |
| `record_flag_events` | Every set and clear. Never updated. |
| `record_notes` | One row per note. Subject, severity, HTML body, author, time. |
| `record_activity` | One row per save, flag change, or new note. Unseen reads this table. |
| `module_cursors` | Per user, per `tablePath`: baseline time, and whether tracking is on. |
| `record_seen` | Per user, per record: when they last opened it. |
| `record_watches` | Per user. Blank `subject_key` means the whole module. Which events. |
| `record_notifications` | Per recipient. Subject, source, sentence, created, read time. |

Standard Helium audit columns apply where a row can change (`record_flags`, cursors, seen, watches, notifications). Events and notes are insert-only.

## Phases

One phase per conversation, top to bottom. A packet phase ends when the packet is handed over, and it is complete when the apply is reported. The agent does not apply migrations.

| Phase | What | Kind | Status |
| --- | --- | --- | --- |
| 0 | Locks | Decision | Proposed |
| 1 | Flag store | Helium, human apply | Not started |
| 2 | Flags on Query Manager | Lithium UI | Not started |
| 3 | Note store | Helium, human apply | Not started |
| 4 | Notes on Query Manager | Lithium UI | Not started |
| 5 | Annotations inbox | Lithium UI | Not started |
| 6 | Unseen store | Helium, human apply | Not started |
| 7 | Unseen on Query Manager | Lithium UI | Not started |
| 8 | Watches and notification rows | Helium, human apply | Not started |
| 9 | Watches on Query Manager | Lithium UI | Not started |
| 10 | Notifications inbox | Lithium UI | Not started |
| 11 | Live badge updates | Hydrogen + Lithium | Not started |
| 12 | Scripting | Lithium UI | Not started |
| 13 | Remaining grids | Lithium UI | Not started |

Tickets are not a phase. Web Push is not a phase. Course Builder is not a phase.

## Phase 0 — Locks

**Goal:** The decisions above are the contract.

**Entry gate:** None.

**Work items:**

1. Record the locks, the starter seeds, and the phase order in this file.

**Done means:** This file is the plan a later session can resume.

**Exit gate:** The owner approves the plan.

**Status:** Proposed. Not approved.

## Phase 1 — Flag store

**Goal:** Query Manager can store shared flags and their history, before the grid shows them.

**Entry gate:** Phase 0 approved. Next free Acuranzo file, QueryRef, and lookup id re-checked on disk.

**Work items:**

1. Confirm the flag-kind names, icons, and the table names.
2. Seed the flag-kind Lookup. Each row has a name, an icon, a color, and the sort order above.
3. Create `record_flags` and `record_flag_events`.
4. One QueryRef lists the current flags for `queries/query-manager` and a set of keys.
5. One QueryRef toggles one kind for one key: write the event, insert or delete the current row, and refuse a retired or unknown kind.
6. One QueryRef lists flag history for one key.
7. Deleting a flag kind is refused when any event uses it. Retire stays available.
8. Hand over the packet. Do not apply it.

**Done means:** After apply, a toggle QueryRef turns a kind on and off for one query, and the history QueryRef returns both acts with the account id that was sent.

**Exit gate:** The owner reports the apply.

**Status:** Not started.

## Phase 2 — Flags on Query Manager

**Goal:** The Queries grid shows current flags, edits them, and opens history.

**Entry gate:** Phase 1 applied.

**Work items:**

1. Opt in only the Query Manager constructor.
2. After the list loads, merge current flags into the rows.
3. Show up to three icons and a `+N` remainder. A blank cell means no flags.
4. The flag control sets and clears kinds. Show flag history opens the event list.
5. A header filter can keep rows that have a chosen kind.
6. `npm test` and `npm run lint` from `elements/003-lithium`.

**Done means:** On Query Manager, two people can take turns setting and clearing a star, the icons match what is on, and history shows both acts.

**Exit gate:** The owner tries that on Query Manager, and the named npm commands passed.

**Status:** Not started.

## Phase 3 — Note store

**Goal:** A query can have many permanent notes.

**Entry gate:** Phase 2 complete.

**Work items:**

1. Confirm the severity names and the note table name.
2. Seed the severity Lookup.
3. Create `record_notes`. Body is text. There is no update and no delete QueryRef.
4. One QueryRef lists subject, severity, author, and time for one key. It does not return the body.
5. One QueryRef returns one note's body.
6. One QueryRef inserts a note. Subject is plain text. Body is stored as submitted. `USERID` is the author.
7. One QueryRef returns the note count for a set of query keys.
8. Hand over the packet.

**Done means:** After apply, an insert shows up in the list and the count, and the body query returns that HTML.

**Exit gate:** The owner reports the apply.

**Status:** Not started.

## Phase 4 — Notes on Query Manager

**Goal:** The Queries grid shows the count and can read and create notes.

**Entry gate:** Phase 3 applied.

**Work items:**

1. Merge the count into the rows. Zero is a blank cell.
2. The count opens the popup. The left LithiumTable lists notes, newest first. The right side renders the selected body through `sanitizeHtml`.
3. Compose uses SunEditor, plus subject and severity. Save calls the insert QueryRef and refreshes the count.
4. A header filter can keep rows whose count is at least N.
5. `npm test` and `npm run lint`.

**Done means:** A new note on a query raises its count, appears in the popup, and cannot be edited or deleted there.

**Exit gate:** The owner tries that on Query Manager, and the named npm commands passed.

**Status:** Not started.

## Phase 5 — Annotations inbox

**Goal:** Manager 20 lists every note on queries and can create one.

**Entry gate:** Phase 4 complete.

**Work items:**

1. Replace the Annotation Manager placeholder.
2. List notes across `queries/query-manager`, including notes whose query row is gone. A missing query shows its ref.
3. Selecting a note shows the sanitized body. A control opens that query in Query Manager when the row still exists.
4. Create picks a query, then subject, severity, and SunEditor, and calls the same insert QueryRef.
5. `npm test` and `npm run lint`.

**Done means:** A note created in the inbox shows on the query's count, and a note whose query was deleted still opens in the inbox.

**Exit gate:** The owner tries both, and the named npm commands passed.

**Status:** Not started.

## Phase 6 — Unseen store

**Goal:** The database can say which queries are unseen for one account.

**Entry gate:** Phase 5 complete.

**Work items:**

1. Confirm the cursor, seen, and activity table names.
2. Create `record_activity`, `module_cursors`, and `record_seen`.
3. The flag toggle and the note insert also write `record_activity`.
4. Query Manager's save and delete write a save activity row for that query. Delete does not remove notes, events, or watches.
5. One QueryRef returns the unseen count for Queries, using the baseline, the per-record seen time, the don't-track flag, and "the query still exists."
6. One QueryRef marks one query seen. One sets the module baseline to now (mark all read). One turns tracking off or on. The first read for a user with no cursor sets the baseline to now and returns zero.
7. Hand over the packet.

**Done means:** After apply, a new flag or note makes the count 1, mark-seen makes it 0, and a deleted query does not add to the count.

**Exit gate:** The owner reports the apply.

**Status:** Not started.

## Phase 7 — Unseen on Query Manager

**Goal:** The menu shows the Queries badge, and the grid highlights unseen rows.

**Entry gate:** Phase 6 applied.

**Work items:**

1. At login, paint the Queries menu count from the count QueryRef. Other modules show no badge.
2. Highlight unseen rows. The highlight is separate from the flag icons.
3. Opening a query marks it seen and the badge drops.
4. Mark all read and Don't track are available on Query Manager.
5. A filter can keep only unseen rows.
6. `npm test` and `npm run lint`.

**Done means:** After login, a query with new activity shows a badge and a highlight, and opening it clears both for that account only.

**Exit gate:** The owner tries that with two accounts, and the named npm commands passed.

**Status:** Not started.

## Phase 8 — Watches and notification rows

**Goal:** A watch writes a notification row when a matching event happens.

**Entry gate:** Phase 7 complete.

**Work items:**

1. Confirm the watch and notification table names.
2. Create `record_watches` and `record_notifications`.
3. One QueryRef adds a watch and one removes it. Scope is the Queries module or one query. Events are flag, note, save, or all.
4. The flag, note, and save QueryRefs insert notification rows for matching watches, excluding `USERID`.
5. One QueryRef lists one account's notification rows. One marks one row read.
6. Hand over the packet.

**Done means:** After apply, Bobby watching a query gets a row when Billy flags it, and Billy does not get a row for his own flag.

**Exit gate:** The owner reports the apply.

**Status:** Not started.

## Phase 9 — Watches on Query Manager

**Goal:** The Queries grid can set the same watch every later grid will use.

**Entry gate:** Phase 8 applied.

**Work items:**

1. Add the watch control beside Flag and Annotate.
2. The popup can watch this query or the whole Queries module, and can choose flag, note, save, or all.
3. A watched row is marked from a follow-up query. That mark is not a flag icon.
4. `npm test` and `npm run lint`.

**Done means:** A watch set on a query produces a notification row for the other account when the chosen event happens.

**Exit gate:** The owner tries that with two accounts, and the named npm commands passed.

**Status:** Not started.

## Phase 10 — Notifications inbox

**Goal:** One module lists notification rows and can add or remove watches.

**Entry gate:** Phase 9 complete.

**Work items:**

1. Register the Notifications manager on the existing footer button. Take the next free manager id and record it here.
2. List the signed-in account's rows. Selecting one marks it read and opens the query when the row still exists.
3. Create and remove watches from here, for the Queries module or for a picked query, using the same QueryRefs as the grid.
4. `npm test` and `npm run lint`.

**Done means:** A notice created by a watch appears in the inbox, marks read, and the same watch can be removed without going back to the grid.

**Exit gate:** The owner tries that, and the named npm commands passed.

**Status:** Not started.

## Phase 11 — Live badge updates

**Goal:** The Queries badge moves on an open session without a new login.

**Entry gate:** Phase 10 complete. Hydrogen work approved for this phase.

**Work items:**

1. When an activity row is written for Queries, publish a per-account badge delta.
2. Relay it on the allowlisted WebSocket path. NATS carries it across Hydrogen instances.
3. Lithium applies the delta to the Queries menu count and, if that grid is open, to the row highlight.
4. A missed socket event is repaired by the next login count.
5. `zsh -ic 'mkt'` and `zsh -ic 'mkp'` for the C. `npm test` and `npm run lint` for the client.

**Done means:** Billy flags a query and Bobby's Queries badge changes while Queries stays closed, without Bobby logging in again.

**Exit gate:** The owner sees that on two sessions, and the named commands passed.

**Status:** Not started.

## Phase 12 — Scripting

**Goal:** The same family works on a compound key.

**Entry gate:** Phase 11 complete.

**Work items:**

1. Opt in the Scripting constructor. `subject_key` is `group_name::script_name`.
2. Flags, notes, unseen, and watches use the same QueryRefs and the same popups.
3. The menu badge is per module, so Scripting has its own count.
4. The inboxes include scripting notes and notices, and can target a script.
5. `npm test` and `npm run lint`.

**Done means:** Flagging a script, noting it, and watching it behave as they do for a query, and the menu shows a separate Scripting count.

**Exit gate:** The owner tries that, and the named npm commands passed.

**Status:** Not started.

## Phase 13 — Remaining grids

**Goal:** Every other LithiumTable that edits records can opt in the same way.

**Entry gate:** Phase 12 complete.

**Work items:**

1. Walk [`/docs/Li/LITHIUM-TAB-SURVEY.md`](/docs/Li/LITHIUM-TAB-SURVEY.md). Opt in each editable grid. Leave the login language picker alone. Leave Course Builder's own flag and annotation columns alone.
2. Skip a grid that has no stable primary key, and name it in the phase close-out.
3. Give each opted-in module its own badge and inbox scope.
4. `npm test` and `npm run lint`.

**Done means:** An opted-in grid shows flags, notes, unseen, and watches, and a grid that was not opted in is unchanged.

**Exit gate:** The owner names which grids were tried, and the named npm commands passed.

**Status:** Not started.

## Working log

### 2026-10-28 — Plan drafted

Locks 1–14 and the phase order came from the design sitting. Phase 0 is not approved. No migration was written and no source was edited.
