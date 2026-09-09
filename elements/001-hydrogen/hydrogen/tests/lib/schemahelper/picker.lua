require('schemahelper_test_env')
local connect = require('schemahelper_connect')
local W = require('schemahelper_wrappers')

local pg = connect.picker_blurb('postgresql')
assert(pg:find('ACURANZO_DB_HOST', 1, true), pg)
assert(pg:find('ACURANZO_DB_USER', 1, true), pg)
assert(pg:find('ACURANZO_DB_PASS', 1, true), pg)
assert(pg:find('ACURANZO_DB_NAME', 1, true), pg)
assert(not pg:find('ACURANZO_DB_*', 1, true), pg)
assert(pg:find('schema demo', 1, true), pg)

local yb = connect.picker_blurb('yugabytedb')
assert(yb:find('YUGABYTE_DB_HOST', 1, true), yb)
assert(not yb:find('ACURANZO', 1, true), yb)
assert(not yb:find('YUGABYTE_DB_*', 1, true), yb)

local sq = connect.picker_blurb('sqlite')
assert(sq == 'hydrodemo.sqlite', sq)

local mysql = connect.picker_blurb('mysql')
assert(mysql:find('CANVAS_DB_HOST', 1, true), mysql)
assert(not mysql:find('CANVAS_DB_*', 1, true), mysql)

local db2 = connect.picker_blurb('db2')
assert(db2:find('HYDROTST_DB_USER', 1, true), db2)
assert(not db2:find('HYDROTST_DB_*', 1, true), db2)

local label = W.wrapper_label({ engine = 'postgresql', path = '/tmp/schematool_postgresql.sh' })
assert(label:find('ACURANZO_DB_HOST', 1, true), label)
assert(not label:find('ACURANZO_DB_*', 1, true), label)

print('OK: picker blurbs use env names; yugabyte is never ACURANZO')
os.exit(0)
