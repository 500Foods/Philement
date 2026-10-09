-- schemahelper_queue.lua
-- Merge SchemaTool metadata + catalog JSON into a review queue.
-- Orchestrator: delegates pure helpers / state / load / decode / view /
-- render to sibling modules under lua/ (schemahelper_qutil, _qstate,
-- _qload, _qdecode, _qexplain, _qdiff, _qview, _qrender).
--
-- CHANGELOG
-- 0.6.10 - 2026-10-09 - Split into lua/ submodules for the 1000-line cap;
--   this file is now the thin orchestrator + find_by_id
-- 0.6.9 - 2026-10-07 - Default-row and unkeyed DML review text
-- 0.6.8 - 2026-10-07 - Dialect apply text; whole-row metadata does not replay DDL
-- 0.6.7 - 2026-10-07 - Queue type and dropped; info extras stay off the review list
-- 0.6.6 - 2026-10-07 - Pass sidecar role through to the state path
-- 0.6.5 - 2026-09-09 - Accept hash gate + accepted list + un-accept
-- 0.5.8 - 2026-08-25 - Split into lua/ submodules (qutil/qstate/qload/qdecode); this file is now the orchestrator
-- 0.5.7 - 2026-08-24 - Decode MySQL/MariaDB lowercase brotli_decompress(FROM_BASE64('...'))
-- 0.5.6 - 2026-08-24 - Decode DB2 brotli+base64; PostgreSQL brotli_decompress + CONVERT_FROM(DECODE)
-- 0.5.5 - 2026-08-24 - Phase 7: u_label + explain_check for catalog DDL apply
-- 0.5.4 - 2026-08-24 - Phase 5 slice: orphan [U] label + explain_check branch
-- 0.5.1 - 2026-08-23 - [U] is update; catalog findings show last fold ref
-- 0.5.0 - 2026-08-23 - Phase 5: review [U] reason (one-field apply)
-- 0.4.14 - 2026-08-23 - Explore Enter decodes highlighted brotli line
-- 0.4.13 - 2026-08-23 - One finding per field; decoded brotli is its own item
-- 0.4.11 - 2026-08-23 - Decode brotli/base64 in compare; explore view
-- 0.4.10 - 2026-08-23 - Migration vs DB sides; ignore 1000→1003 apply
-- 0.4.9 - 2026-08-23 - Explore: field-level expected/actual diff
-- 0.4.0 - 2026-08-23 - Phase 4: packet refs on dashboard / review
-- 0.2.2 - 2026-08-23 - Phase 2: explore lines, decision persistence, cursor tracking
-- 0.2.0 - 2026-08-23 - Phase 1: findings ingest, sidecar decisions, dashboard totals

local U = require("schemahelper_qutil")
local S = require("schemahelper_qstate")
local L = require("schemahelper_qload")
local D = require("schemahelper_qdecode")
local X = require("schemahelper_qexplain")
local DF = require("schemahelper_qdiff")
local V = require("schemahelper_qview")
local R = require("schemahelper_qrender")

local M = {}

-- Re-export all public functions from sibling modules.
M.default_state_path = S.default_state_path
M.load_state = S.load_state
M.create_state = S.create_state
M.artifacts_present = S.artifacts_present
M.jq_update_state = S.jq_update_state
M.save_cursor = S.save_cursor
M.save_decision = S.save_decision
M.remove_decision = S.remove_decision

M.load_metadata = L.load_metadata
M.load_catalog = L.load_catalog
M.load_rows = L.load_rows

M.decode_embedded = D.decode_embedded
M.has_embed = U.has_embed
M.build_line_decode_view = D.build_line_decode_view

M.sides_of = X.sides_of
M.explain_default = X.explain_default
M.explain_check = X.explain_check

M.focus_pair = DF.focus_pair
M.payload_map = DF.payload_map
M.line_diff_lines = DF.line_diff_lines
M.field_pair = DF.field_pair
M.compare_lines = DF.compare_lines

M.load_detail_section = V.load_detail_section
M.build_explore_view = V.build_explore_view
M.note_for = V.note_for
M.json_explore_lines = V.json_explore_lines
M.explore_lines = V.explore_lines
M.json_subobj = U.json_subobj
M.payload_text = U.payload_text
M.finding_hash = U.finding_hash

M.build_review_lines_detailed = R.build_review_lines_detailed
M.build_dashboard_lines = R.build_dashboard_lines
M.build_review_lines = R.build_review_lines
M.accept_holds = R.accept_holds
M.build = R.build
M.u_label = R.u_label
M.g_label = R.g_label
M.promote_label = R.promote_label

local function find_by_id(findings, id)
    for _, f in ipairs(findings) do
        if f.id == id then
            return f
        end
    end
    return nil
end

function M.find_finding(findings, id)
    return find_by_id(findings, id)
end

return M
