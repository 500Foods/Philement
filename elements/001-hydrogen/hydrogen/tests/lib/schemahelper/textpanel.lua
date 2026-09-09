require('schemahelper_test_env')
local ok, TextPanel = pcall(require, 'terminal.ui.panel.text')
if not ok then
    print('ERR: cannot require terminal.ui.panel.text')
    os.exit(1)
end
print('OK: TextPanel loaded: ' .. tostring(TextPanel))
os.exit(0)
