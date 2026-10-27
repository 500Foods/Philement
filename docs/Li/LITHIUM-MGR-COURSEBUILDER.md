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

Next caller ref **4526**. Next Courses file **4123**.

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
