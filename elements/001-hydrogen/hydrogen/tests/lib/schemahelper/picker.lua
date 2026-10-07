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
assert(mysql:find('MYSQL_DB_HOST', 1, true), mysql)
assert(not mysql:find('MYSQL_DB_*', 1, true), mysql)

local mariadb = connect.picker_blurb('mariadb')
assert(mariadb:find('MARIADB_DB_HOST', 1, true), mariadb)
assert(not mariadb:find('MARIADB_DB_*', 1, true), mariadb)

local db2 = connect.picker_blurb('db2')
assert(db2:find('HYDROTST_DB_USER', 1, true), db2)
assert(not db2:find('HYDROTST_DB_*', 1, true), db2)

local mysql_test = connect.picker_blurb('mysql', 'test')
assert(mysql_test:find('schema test', 1, true), mysql_test)
assert(not mysql_test:find('schema demo', 1, true), mysql_test)

local mssql_test = connect.picker_blurb('mssql', 'test')
assert(mssql_test:find('schema testms', 1, true), mssql_test)
assert(not mssql_test:find('demoms', 1, true), mssql_test)

local sq_test = connect.picker_blurb('sqlite', 'test')
assert(sq_test == 'hydrotst.sqlite', sq_test)

local fb_test = connect.picker_blurb('firebird', 'test')
assert(fb_test:find('FIREBIRD_DB_PATH_TEST', 1, true), fb_test)
assert(not fb_test:find('FIREBIRD_DB_PATH_DEMO', 1, true), fb_test)

local label = W.wrapper_label({ engine = 'postgresql', path = '/tmp/schematool_postgresql.sh' })
assert(label:find('ACURANZO_DB_HOST', 1, true), label)
assert(not label:find('ACURANZO_DB_*', 1, true), label)

print('OK: picker blurbs use env names')
os.exit(0)
