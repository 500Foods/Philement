-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local _M = {
	gettime = require "luatz.gettime";
	parse = require "luatz.parse";
	strftime = require "luatz.strftime";
	timetable = require "luatz.timetable";
	tzcache = require "luatz.tzcache";
}

--- Top-level aliases for common functions

_M.time = _M.gettime.gettime
_M.get_tz = _M.tzcache.get_tz

--- Handy functions

_M.time_in = function(tz, now)
	return _M.get_tz(tz):localize(now)
end

_M.now = function()
	local ts = _M.gettime.gettime()
	return _M.timetable.new_from_timestamp(ts)
end

--- C-like functions

_M.gmtime = function(ts)
	return _M.timetable.new_from_timestamp(ts)
end

_M.localtime = function(ts)
	ts = _M.time_in(nil, ts)
	return _M.gmtime(ts)
end

_M.ctime = function(ts)
	return _M.strftime.asctime(_M.localtime(ts))
end

return _M
