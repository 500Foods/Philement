local schemagui = os.getenv('SCHEMAGUI')
local fixture = os.getenv('FIXTURE_DIR')
if not schemagui or schemagui == '' or not fixture or fixture == '' then
    print('ERR: SCHEMAGUI/FIXTURE_DIR not set')
    os.exit(1)
end
package.path = schemagui .. '/lua/?.lua;' .. package.path

local M = {
    schemagui = schemagui,
    fixture = fixture,
}

function M.require_fns(mod, prefix, names)
    local errs = {}
    for _, fn in ipairs(names) do
        if type(mod[fn]) ~= 'function' then
            errs[#errs + 1] = prefix .. '.' .. fn .. ' is not a function'
        end
    end
    return errs
end

function M.fail_if(errs)
    if #errs > 0 then
        for _, e in ipairs(errs) do
            print('ERR: ' .. e)
        end
        os.exit(1)
    end
end

function M.stub_terminal(t)
    t.width = 80
    t.height = 24
    t.output.write = function() end
    t.cursor.position.set = function() end
    t.text.push_seq = function() return nil end
    t.text.pop_seq = function() return nil end
    t.text.width.utf8swidth = function(s) return #tostring(s or '') end
    t.text.width.truncate_ellipsis = function(w, s, _)
        return tostring(s or ''):sub(1, w)
    end
    return t
end

return M
