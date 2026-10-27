/**
 * Course Builder run list definitions (CATCHUP Phase 20 / CB-2).
 *
 * Queries only. No screen uses these yet; the queue UI is Phase 21.
 * 4523 lists runs by state (STRING STATE, '' = all).
 * 4524 returns a run with its artifacts (INTEGER RUN_ID).
 * 4525 returns a run's ledger events (INTEGER RUN_ID).
 * The tableDef lives here until Phase 21 decides whether it moves
 * to a Lookup 059 row. Key 16 stays free.
 */

export const RUN_LIST_QUERY_REF = 4523;
export const RUN_GET_QUERY_REF = 4524;
export const RUN_EVENTS_QUERY_REF = 4525;
export const RUN_TABLE_PATH = 'courses/course-build-runs';

/** P20a state names, in pipeline order. */
export const RUN_STATES = [
  'suggested', 'received', 'intake',
  'duplicate', 'rejected', 'junk',
  'submission', 'accepted', 'declined',
  'outline', 'approve', 'research', 'review', 'build', 'inspect',
  'checklist', 'draft', 'publishing', 'announcing', 'complete',
  'abandoned', 'failed',
];

const col = (field, title, coltype, visible, extra = {}) => ({
  title, field, coltype, visible, headerFilter: false, editable: false, ...extra,
});

/** LithiumTable tableDef for QueryRef 4523. Same shape as Lookup 059 rows. */
export const RUN_TABLE_DEF = {
  $version: '1.0.0',
  $description: 'Course Builder run list. QueryRef 4523.',
  table: 'course-build-runs',
  title: 'Course Builder Runs',
  queryRef: RUN_LIST_QUERY_REF,
  readonly: true,
  selectableRows: 1,
  layout: 'fitColumns',
  responsiveLayout: 'collapse',
  resizableColumns: true,
  persistSort: true,
  persistFilter: false,
  initialSort: [
    { column: 'updated_at', dir: 'desc' },
    { column: 'run_id', dir: 'desc' },
  ],
  columns: {
    working_title: col('working_title', 'Working Title', 'string', true),
    state: col('state', 'State', 'string', true),
    updated_at: col('updated_at', 'Updated', 'datetime', true),
    price_cents: col('price_cents', 'Price (cents)', 'integer', false),
    artifact_count: col('artifact_count', 'Artifacts', 'integer', false),
    run_id: col('run_id', 'Run', 'integer', false, { primaryKey: true }),
    suggestion_id: col('suggestion_id', 'Suggestion', 'integer', false),
    template_key: col('template_key', 'Template', 'string', false),
    canvas_course_id: col('canvas_course_id', 'Canvas Course', 'integer', false),
    error_text: col('error_text', 'Error', 'string', false),
    flagged: col('flagged', 'Flagged', 'integer', false),
    course_name: col('course_name', 'Suggested Course Name', 'string', false),
    submitter_name: col('submitter_name', 'Submitter', 'string', false),
    created_at: col('created_at', 'Created', 'datetime', false),
  },
};

/** Params for 4523. Unknown or blank state lists every run. */
export function buildRunListParams(state) {
  const value = RUN_STATES.includes(state) ? state : '';
  return { STRING: { STATE: value } };
}

/** Params for 4524 and 4525. */
export function buildRunIdParams(runId) {
  return { INTEGER: { RUN_ID: Number(runId) } };
}
