# Course Builder

Operator surface for the 500 Courses build pipeline. Plan:
[`/elements/003-lithium/CATCHUP.md`](/elements/003-lithium/CATCHUP.md)
Phases 19–23. Pipeline:
[`/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md`](/mnt/extra/Projects/500-Courses-Reception/COURSEBUILDER.md).
Phase 23 finishes this page. Until then it is the QueryRef map.

Code: `src/managers/course-builder/`. Table path
`courses/course-builder` is Lookup 059 key **15**. Key **16** is free
(reserved for the Phase 21 run queue if it needs a Lookup row).

## QueryRefs

| Ref | File | Name | Params | Phase | Applied |
| --- | --- | --- | --- | --- | --- |
| 4513 | 4099 | Staff: List Suggestions (superseded by 4515) | — | 19 | yes |
| 4514 | 4100, 4111 | Staff: Save Working Title (TRANSACTION) | STRING WORKING_TITLE, INTEGER USERID, SUGGESTION_ID | 19 | yes |
| 4515 | 4102 | Staff: List Suggestions | — | 19 | yes |
| 4516 | 4103 | Staff: Insert Suggestion | — | 19 | yes |
| 4517 | 4104, 4112 | Staff: Update Suggestion | — | 19 | yes |
| 4518 | 4105, 4113 | Staff: Duplicate Suggestion | — | 19 | yes |
| 4519 | 4106 | Staff: Delete Suggestion | INTEGER SUGGESTION_ID | 19 | yes |
| 4520 | 4107 | Staff: Move to Intake | INTEGER SUGGESTION_ID, USERID | 19 | yes |
| 4521 | 4108 | Staff: Toggle Flag | INTEGER SUGGESTION_ID, USERID | 19 | yes |
| 4522 | 4115 | Staff: Move to Suggested | INTEGER SUGGESTION_ID, USERID | 19 | yes |
| **4523** | 4120 | Staff: List Runs by State | STRING STATE (`''` = all) | 20 | no |
| **4524** | 4121 | Staff: Get Run with Artifacts | INTEGER RUN_ID | 20 | no |
| **4525** | 4122 | Staff: List Run Events | INTEGER RUN_ID | 20 | no |
| **4526** | 4131 | Staff: Set Run State (logged override, P22a) | INTEGER SUGGESTION_ID, USERID; STRING FROM_STATE, TO_STATE | 22 | no |

Next caller ref **4527**. Next Courses file **4132** (4123–4127 Phase 21, 4128–4131 Phase 22).

## Tables (CB-S + CB-2)

- `course_build_runs` (4097, 4101, **4118**): one row per suggestion.
  4118 adds `template_key`, `price_cents`, `canvas_course_id`,
  `error_text` and `course_build_runs_idx_state`.
- `course_build_actions` (4098): append-only ledger. It is the event
  log; 4525 reads it. Every state transition writes one row.
- `course_build_artifacts` (**4119**): `run_id`, `kind`
  (`outline|material|module_html|checklist|…`), `module_index`,
  `body`, `model_id`. Append-only; latest = highest `artifact_id`.

States are the P20a / `CB-1-20261026` names, mirrored in
`RUN_STATES` (`course-build-runs.js`).

## Run list tableDef

`RUN_TABLE_DEF` in `src/managers/course-builder/course-build-runs.js`.
Same shape as a Lookup 059 tableDef. Visible: Working Title, State,
Updated. Phase 21 wires it to a LithiumTable and decides whether to
move it to Lookup 059 key 16.

## CB-2 SQL fixture (run after 4118–4122 apply)

PostgreSQL. Prefix tables with the Lithium schema. Pick a
`suggestion_id` that has a run, or use a scratch run id; delete after.

```sql
-- 1. run columns exist
SELECT run_id, state, template_key, price_cents, canvas_course_id, error_text
FROM course_build_runs ORDER BY run_id DESC LIMIT 3;

-- 2. artifact + ledger row on an existing run (:run = a run_id from 1)
INSERT INTO course_build_artifacts
  (artifact_id, run_id, kind, module_index, body, model_id,
   created_id, created_at, updated_id, updated_at)
VALUES ((SELECT COALESCE(MAX(artifact_id),0)+1 FROM course_build_artifacts),
        :run, 'outline', NULL, '{"fixture":true}', NULL, 0, now(), 0, now());
INSERT INTO course_build_actions
  (action_id, run_id, actor_account_id, action_name,
   created_id, created_at, updated_id, updated_at)
VALUES ((SELECT COALESCE(MAX(action_id),0)+1 FROM course_build_actions),
        :run, NULL, 'fixture', 0, now(), 0, now());

-- 3. through Lithium/auth_query: 4523 {"STRING":{"STATE":""}},
--    4524 {"INTEGER":{"RUN_ID":run}}, 4525 {"INTEGER":{"RUN_ID":run}}

-- 4. clean up
DELETE FROM course_build_artifacts WHERE body = '{"fixture":true}';
DELETE FROM course_build_actions WHERE action_name = 'fixture';
```

## Phase 21 — Run tab and human gates

The queue is the existing suggestion list (Lookup 059 key **15**,
QueryRef 4515). Key **16** is not used. The **Run** tab
(`course-build-gate.js`) finds the selected suggestion's run in 4523
(`STATE` `''`), then reads 4524 (artifacts) and 4525 (latest ten
events, newest first). Artifact bodies render as text only.

| State | Buttons | Script | Moves to |
|-------|---------|--------|----------|
| `submission` | Accept | `Build.Accept` (4124) | `accepted` |
| `submission` | Decline (confirm) | `Build.Decline` (4125) | `declined` |
| `approve` | Approve (confirm) | `Build.Approve` (4126) | `research` |
| `approve` | Abandon (confirm) | `Build.Abandon` (4127) | `abandoned` |

All four are `invokable` 1 and call the `Build.Gate` module (4123,
`invokable` 0): staff gate (`Enroll.StaffGate`), one conditional
`UPDATE … WHERE run_id = :RUN_ID AND state = :FROM_STATE`, then one
`course_build_actions` row named after the new state. Payload:
`{ script, params: { run_id }, wait: true, timeout_seconds: 15 }`.
Failures: `{ ok=false, code }` with `validation`, `not_staff`,
`missing_identity`, `lookup_error`, `not_found`, `write_error`, or
`wrong_state` (adds `state` and `expected`). A double click or a second
operator gets `wrong_state`. No mail (CB-4 templates not written).
No Research button exists.

### Phase 21 SQL fixture (after 4123–4127 apply)

Intake → Submission is CB-9 and is not built. Andrew approved
(2026-10-27) setting fixture run states by SQL until CB-9 exists. Set
states by SQL for the test only. Note each run's state first and restore it after.

```sql
SELECT run_id, suggestion_id, state FROM course_build_runs ORDER BY run_id;
UPDATE course_build_runs SET state = 'submission' WHERE run_id = 1; -- Decline test
UPDATE course_build_runs SET state = 'approve'    WHERE run_id = 2; -- Approve/Abandon test
-- In Lithium, Course Builder > Run tab: Decline run 1, Approve run 2.
-- Also click Decline twice quickly or re-run via DevTools: expect wrong_state.
SELECT run_id, state FROM course_build_runs WHERE run_id IN (1, 2);
SELECT run_id, action_name, actor_account_id, created_at
  FROM course_build_actions WHERE run_id IN (1, 2)
 ORDER BY action_id DESC LIMIT 4;
-- restore
UPDATE course_build_runs SET state = 'intake' WHERE run_id IN (1, 2);
```

### Phase 21 result (2026-10-27)

Applied 4123–4127 (APPLY 4127). Decline on run 1 → `declined`,
Approve on run 2 → `research`, ledger rows named after each state,
actor 9001. Both runs restored to `intake` by SQL; the UI has no path
back to `intake` until CB-9. Abandon deferred; Accept not clicked.

## Phase 22 — Outline, Inspect, Publish (complete 2026-10-27)

4128–4131 applied (APPLY 4131). Inspect Pass and Publish validated;
Inspect Fail manual click deferred. Next Courses file **4132**, next
ref **4527**.

The Run tab adds an outline section (`course-build-outline.js`):
latest `outline` artifact rendered from JSON with text nodes only,
materials (latest per `module_index`), and the outline version list.
**Open LMS** links `CANVAS_BASE_URL/courses/<canvas_course_id>` when
the run has one. No token.

| State | Buttons | Script | Moves to |
|-------|---------|--------|----------|
| `inspect` | Inspect Pass | `Build.InspectPass` (4128) | `checklist` |
| `inspect` | Inspect Fail (confirm) | `Build.InspectFail` (4129) | `build` |
| `draft` | Publish (confirm) | `Build.Publish` (4130) | `publishing` |

Publish shows disabled at `build`, `inspect`, `checklist`. All three
are stubs over `Build.Gate`: state + ledger only. No Canvas, catalog,
or Stripe call until CB-28.

### Phase 22 SQL fixture (after 4128–4130 apply)

```sql
SELECT run_id, state, canvas_course_id FROM course_build_runs WHERE run_id IN (1, 2);
INSERT INTO course_build_artifacts
  (artifact_id, run_id, kind, module_index, body, model_id,
   created_id, created_at, updated_id, updated_at)
VALUES ((SELECT COALESCE(MAX(artifact_id),0)+1 FROM course_build_artifacts),
        1, 'outline', NULL,
        '{"fixture":"p22","title":"<img src=x onerror=alert(1)>","modules":["<script>alert(2)</script>","Intro"]}',
        'fixture', 0, now(), 0, now());
UPDATE course_build_runs SET state = 'inspect' WHERE run_id = 1; -- Inspect Pass/Fail
UPDATE course_build_runs SET state = 'draft'   WHERE run_id = 2; -- Publish
-- Run tab: run 1 shows the outline as text (no alert), Inspect Pass/Fail,
-- disabled Publish. Click Inspect Pass (-> checklist). Run 2: click Publish (-> publishing).
SELECT run_id, state FROM course_build_runs WHERE run_id IN (1, 2);
SELECT run_id, action_name, actor_account_id, created_at
  FROM course_build_actions WHERE run_id IN (1, 2) ORDER BY action_id DESC LIMIT 4;
-- Optional Fail: UPDATE ... SET state='inspect' WHERE run_id=1; click Inspect Fail (-> build).
-- restore
UPDATE course_build_runs SET state = 'intake' WHERE run_id IN (1, 2);
DELETE FROM course_build_artifacts WHERE body LIKE '{"fixture":"p22"%';
```

### Phase 22a — Stage picker override (4526, complete)

The first-tab Stage picker writes `course_build_runs.state`, the same
column the Run tab gates move. Normal moves stay on 4520 (Intake) and
4522 (Suggested). An "Override (staff)" group lists every P20a state
and calls **4526** with `FROM_STATE` (what the screen showed) and
`TO_STATE`. One transaction writes one `course_build_actions` row
(`action_name` `override`, `prompt` = previous state, `result_ref` =
new state) and the guarded UPDATE. 0 rows → `wrong_state` toast,
refresh. `suggested` clears `receipt_mail`, so the next receipt pass
sends again. Automatic targets (`outline`, `research`, `build`,
`checklist`, `publishing`, `announcing`) get a red warning in the
confirm. The picker labels append ` (Auto)` for `AUTOMATIC_STATES`
(client-only; confirm copy keeps plain names). Applied 2026-10-27.

```sql
-- After an override: newest ledger rows for a suggestion
SELECT a.action_id, a.run_id, a.action_name, a.prompt AS from_state,
       a.result_ref AS to_state, a.actor_account_id, a.created_at
FROM course_build_actions a
JOIN course_build_runs r ON r.run_id = a.run_id
WHERE r.suggestion_id = :suggestion_id
ORDER BY a.action_id DESC LIMIT 5;
```
