local env = require('schemahelper_test_env')
local apply = require('schemahelper_apply')
local errs = {}

local required = {
    'qualify_queries', 'qualify_table', 'confirm_token', 'refuse_reason',
    'can_apply', 'field_literal', 'build_catalog_sql', 'build_sql', 'write_log',
}
for _, e in ipairs(env.require_fns(apply, 'apply', required)) do
    errs[#errs + 1] = e
end

local finding = { id='cat:accounts:id:nullable', class='catalog', kind='nullable',
  object='accounts', column='id', ref=1148 }
local refuse = apply.refuse_reason(finding, false)
if refuse ~= 'need --allow-write' then
    errs[#errs + 1] = 'refuse_reason no-write=' .. tostring(refuse)
end

local can = apply.can_apply(finding, false)
if can then
    errs[#errs + 1] = 'can_apply should be false without allow_write'
end

local non_meta = { id='cat:accounts:foo', class='content drift', field='foo', ref=1148 }
local refuse2 = apply.refuse_reason(non_meta, true)
if refuse2 ~= 'not a metadata field' then
    errs[#errs + 1] = 'refuse_reason non-meta=' .. tostring(refuse2)
end

local can2 = apply.can_apply(finding, true)
if type(can2) ~= 'boolean' then
    errs[#errs + 1] = 'can_apply returned non-boolean'
end

local orphan = { id='cat:orphans:1290', kind='orphan', ref=1290 }
local refuse3 = apply.refuse_reason(orphan, true)
if refuse3 ~= nil then
    errs[#errs + 1] = 'refuse_reason orphan should be nil, got ' .. tostring(refuse3)
end

local tok = apply.confirm_token(finding)
if tok ~= 'accounts.id' then
    errs[#errs + 1] = 'confirm_token=' .. tostring(tok)
end

local fl = apply.field_literal('sqlite', 'hello')
if type(fl) ~= 'string' or fl == '' then
    errs[#errs + 1] = 'field_literal sqlite empty'
end

local qt = apply.qualify_table('sqlite', '', 'accounts')
if qt ~= 'accounts' then
    errs[#errs + 1] = 'qualify_table sqlite=' .. tostring(qt)
end

env.fail_if(errs)
print('OK: all apply function checks passed (' .. #required .. ' functions)')
os.exit(0)
