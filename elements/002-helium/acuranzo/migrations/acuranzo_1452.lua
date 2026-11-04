-- Migration: acuranzo_1452.lua
-- Seed luatz and resolve IANA zones in Reports.Render (Reporting Phase 22a)
--
-- Library pack (one migration seeds every module, the exception to
-- one-script-per-migration): luatz v0.4-4-ga7c0310, commit
-- a7c031006fad5511858b3bb70e7661a0aad5fc54, github.com/daurnimator/luatz,
-- MIT. Group `luatz`, script name = require path after the first dot
-- (init, gettime, parse, strftime, timetable, tzcache, tzfile, tzinfo,
-- version), invokable=0, mcp_access=0. Pure Lua, no C. One local patch:
-- tzcache.set_base_path(path) for the zoneinfo root. luatz.version
-- returns the pin string.
--
-- zoneinfo is NOT bundled: tzfiles are read from the host tzdata package
-- (/usr/share/zoneinfo) via io.open. Reports.Render (copied forward from
-- 1451) resolves Explicit / report-default / env-default IANA zones
-- through luatz at the generation instant. User with a JWT tz claim keeps
-- _hydrogen.tzoffset (drift accepted). Unknown zone, missing luatz or a
-- sandboxed io -> UTC + warn.
--
-- Config hooks (TBD home; env with hardcoded defaults for now):
--   REPORTS_TZ_ZONEINFO_PATH  reports.tz.zoneinfo_path  default /usr/share/zoneinfo
--   REPORTS_TZ_DEFAULT_ZONE   reports.tz.default_zone   used when the report has no defaultZone
--   reports.tz.tzdata_version (tzdata.zi "# version") not read yet - TBD.
--
-- Update / refresh: zoneinfo refreshes with the OS tzdata package on image
-- rebuild (cadence TBD Andrew). luatz code moves only by a later migration
-- that replaces the luatz.* rows and bumps luatz.version; never edit this
-- file once applied. No cron.
--
-- Reverse restores the 1451 Render body and deletes only the luatz rows.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-03 - Seed luatz; Render resolves IANA zones

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1452"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render"

local LUATZ = {}
LUATZ[#LUATZ + 1] = { "init", [==[
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
]==] }
LUATZ[#LUATZ + 1] = { "gettime", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local _M = {}

_M.source, _M.resolution, _M.gettime = (function()
	local has_syscall, syscall = pcall(require, "syscall")
	if has_syscall and syscall.clock_gettime and syscall.c.CLOCK then
		local clock_id = syscall.c.CLOCK.REALTIME
		local function timespec_to_number(timespec)
			return tonumber(timespec.tv_sec) + tonumber(timespec.tv_nsec) * 1e-9
		end
		return "syscall.clock_gettime(CLOCK_REALTIME)",
			syscall.clock_getres and timespec_to_number(syscall.clock_getres(clock_id)) or 1e-9,
			function()
				return timespec_to_number(syscall.clock_gettime(clock_id))
			end
	end

	local has_unix, unix = pcall(require, "unix")
	-- On Apple devices lunix only uses gettimeofday()
	if has_unix and unix.clock_gettime and unix.uname and unix.uname().sysname ~= "Darwin" then
		return "unix.clock_gettime(CLOCK_REALTIME)", 1e-9, function()
			return unix.clock_gettime()
		end
	end

	if has_syscall and syscall.gettimeofday then
		local function timeval_to_number(timeval)
			return tonumber(timeval.tv_sec) + tonumber(timeval.tv_nsec) * 1e-6
		end
		return "syscall.gettimeofday()", 1e-6,
			function()
				return timeval_to_number(syscall.gettimeofday())
			end
	end

	if has_unix and unix.gettimeofday then
		return "unix.gettimeofday()", 1e-6, unix.gettimeofday
	end

	local has_socket, socket = pcall(require, "socket")
	if has_socket and socket.gettime then
		-- on windows, this uses GetSystemTimeAsFileTime, which has resolution of 1e-7
		-- on linux, this uses gettimeofday, which has resolution of 1e-6
		return "socket.gettime()", 1e-6, socket.gettime
	end

	if ngx and ngx.now then -- luacheck: ignore 113
		return "ngx.now()", 1e-3, ngx.now -- luacheck: ignore 113
	end

	return "os.time()", 1, os.time
end)()

return _M
]==] }
LUATZ[#LUATZ + 1] = { "parse", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local new_timetable = require "luatz.timetable".new

--- Parse an RFC 3339 datetime at the given position
-- Returns a time table and the `tz_offset`
-- Return value is not normalised (this preserves a leap second)
-- If the timestamp is only partial (i.e. missing "Z" or time offset) then `tz_offset` will be nil
-- TODO: Validate components are within their boundarys (e.g. 1 <= month <= 12)
local function rfc_3339(str, init)
	local year, month, day, hour, min, sec, patt_end = str:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)[Tt](%d%d):(%d%d):(%d%d%.?%d*)()", init) -- luacheck: ignore 631
	if not year then
		return nil, "Invalid RFC 3339 timestamp"
	end
	year  = tonumber(year, 10)
	month = tonumber(month, 10)
	day   = tonumber(day, 10)
	hour  = tonumber(hour, 10)
	min   = tonumber(min, 10)
	sec   = tonumber(sec)  -- Allow fractional seconds

	local tt = new_timetable(year, month, day, hour, min, sec)

	local tz_offset
	if str:match("^[Zz]", patt_end) then
		tz_offset = 0
	else
		local hour_offset, min_offset = str:match("^([+-]%d%d):(%d%d)", patt_end)
		if hour_offset then
			tz_offset = tonumber(hour_offset, 10) * 3600 + tonumber(min_offset, 10) * 60
		else -- luacheck: ignore 542
			-- Invalid RFC 3339 timestamp offset (should be Z or (+/-)hour:min)
			-- tz_offset will be nil
		end
	end

	return tt, tz_offset
end

return {
	rfc_3339 = rfc_3339;
}
]==] }
LUATZ[#LUATZ + 1] = { "strftime", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local strformat = string.format
local floor = math.floor
local function idiv(n, d)
	return floor(n / d)
end

local c_locale = {
	abday = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	day = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
	abmon = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	mon = {"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December"};
	am_pm = {"AM", "PM"};
}

--- ISO-8601 week logic
-- ISO 8601 weekday as number with Monday as 1 (1-7)
local function iso_8601_weekday(wday)
	if wday == 1 then
		return 7
	else
		return wday - 1
	end
end
local iso_8601_week do
	-- Years that have 53 weeks according to ISO-8601
	local long_years = {}
	for _, v in ipairs {
		  4,   9,  15,  20,  26,  32,  37,  43,  48,  54,  60,  65,  71,  76,  82,
		 88,  93,  99, 105, 111, 116, 122, 128, 133, 139, 144, 150, 156, 161, 167,
		172, 178, 184, 189, 195, 201, 207, 212, 218, 224, 229, 235, 240, 246, 252,
		257, 263, 268, 274, 280, 285, 291, 296, 303, 308, 314, 320, 325, 331, 336,
		342, 348, 353, 359, 364, 370, 376, 381, 387, 392, 398
	} do
		long_years[v] = true
	end
	local function is_long_year(year)
		return long_years[year % 400]
	end
	function iso_8601_week(self)
		local wday = iso_8601_weekday(self.wday)
		local n = self.yday - wday
		local year = self.year
		if n < -3 then
			year = year - 1
			if is_long_year(year) then
				return year, 53, wday
			else
				return year, 52, wday
			end
		elseif n >= 361 and not is_long_year(year) then
			return year + 1, 1, wday
		else
			return year, idiv(n + 10, 7), wday
		end
	end
end

--- Specifiers
local t = {}
function t:a(locale)
	return "%s", locale.abday[self.wday]
end
function t:A(locale)
	return "%s", locale.day[self.wday]
end
function t:b(locale)
	return "%s", locale.abmon[self.month]
end
function t:B(locale)
	return "%s", locale.mon[self.month]
end
function t:c(locale)
	return "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
		locale.abday[self.wday], locale.abmon[self.month],
		self.day, self.hour, self.min, self.sec, self.year
end
-- Century
function t:C()
	return "%02d", idiv(self.year, 100)
end
function t:d()
	return "%02d", self.day
end
-- Short MM/DD/YY date, equivalent to %m/%d/%y
function t:D()
	return "%02d/%02d/%02d", self.month, self.day, self.year % 100
end
function t:e()
	return "%2d", self.day
end
-- Short YYYY-MM-DD date, equivalent to %Y-%m-%d
function t:F()
	return "%d-%02d-%02d", self.year, self.month, self.day
end
-- Week-based year, last two digits (00-99)
function t:g()
	return "%02d", iso_8601_week(self) % 100
end
-- Week-based year
function t:G()
	return "%d", iso_8601_week(self)
end
t.h = t.b
function t:H()
	return "%02d", self.hour
end
function t:I()
	return "%02d", (self.hour-1) % 12 + 1
end
function t:j()
	return "%03d", self.yday
end
function t:m()
	return "%02d", self.month
end
function t:M()
	return "%02d", self.min
end
-- New-line character ('\n')
function t:n() -- luacheck: ignore 212
	return "\n"
end
function t:p(locale)
	return self.hour < 12 and locale.am_pm[1] or locale.am_pm[2]
end
-- TODO: should respect locale
function t:r(locale)
	return "%02d:%02d:%02d %s",
		(self.hour-1) % 12 + 1, self.min, self.sec,
		self.hour < 12 and locale.am_pm[1] or locale.am_pm[2]
end
-- 24-hour HH:MM time, equivalent to %H:%M
function t:R()
	return "%02d:%02d", self.hour, self.min
end
function t:s()
	return "%d", self:timestamp()
end
function t:S()
	return "%02d", self.sec
end
-- Horizontal-tab character ('\t')
function t:t() -- luacheck: ignore 212
	return "\t"
end
-- ISO 8601 time format (HH:MM:SS), equivalent to %H:%M:%S
function t:T()
	return "%02d:%02d:%02d", self.hour, self.min, self.sec
end
function t:u()
	return "%d", iso_8601_weekday(self.wday)
end
-- Week number with the first Sunday as the first day of week one (00-53)
function t:U()
	return "%02d", idiv(self.yday - self.wday + 7, 7)
end
-- ISO 8601 week number (00-53)
function t:V()
	return "%02d", select(2, iso_8601_week(self))
end
-- Weekday as a decimal number with Sunday as 0 (0-6)
function t:w()
	return "%d", self.wday - 1
end
-- Week number with the first Monday as the first day of week one (00-53)
function t:W()
	return "%02d", idiv(self.yday - iso_8601_weekday(self.wday) + 7, 7)
end
-- TODO make t.x and t.X respect locale
t.x = t.D
t.X = t.T
function t:y()
	return "%02d", self.year % 100
end
function t:Y()
	return "%d", self.year
end
-- TODO timezones
function t:z() -- luacheck: ignore 212
	return "+0000"
end
function t:Z() -- luacheck: ignore 212
	return "GMT"
end
-- A literal '%' character.
t["%"] = function(self) -- luacheck: ignore 212
	return "%%"
end

local function strftime(format_string, timetable)
	return (string.gsub(format_string, "%%([EO]?)(.)", function(locale_modifier, specifier)
		local func = t[specifier]
		if func then
			return strformat(func(timetable, c_locale))
		else
			error("invalid conversation specifier '%"..locale_modifier..specifier.."'", 3)
		end
	end))
end

local function asctime(timetable)
	-- Equivalent to the format string "%c\n"
	return strformat(t.c(timetable, c_locale)) .. "\n"
end

return {
	strftime = strftime;
	asctime = asctime;
}
]==] }
LUATZ[#LUATZ + 1] = { "timetable", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local strftime = require "luatz.strftime".strftime
local strformat = string.format
local floor = math.floor
local idiv do
	-- Try and use actual integer division when available (Lua 5.3+)
	local idiv_loader = (loadstring or load)([[return function(n,d) return n//d end]], "idiv") -- luacheck: ignore 113
	if idiv_loader then
		idiv = idiv_loader()
	else
		idiv = function(n, d)
			return floor(n/d)
		end
	end
end


local mon_lengths = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
-- Number of days in year until start of month; not corrected for leap years
local months_to_days_cumulative = {0}
for i = 2, 12 do
	months_to_days_cumulative[i] = months_to_days_cumulative[i-1] + mon_lengths[i-1]
end
-- For Sakamoto's Algorithm (day of week)
local sakamoto = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

local function is_leap(y)
	if (y % 4) ~= 0 then
		return false
	elseif (y % 100) ~= 0 then
		return true
	else
		return (y % 400) == 0
	end
end

local function month_length(m, y)
	if m == 2 then
		return is_leap(y) and 29 or 28
	else
		return mon_lengths[m]
	end
end

local function leap_years_since(year)
	return idiv(year, 4) - idiv(year, 100) + idiv(year, 400)
end

local function day_of_year(day, month, year)
	local yday = months_to_days_cumulative[month]
	if month > 2 and is_leap(year) then
		yday = yday + 1
	end
	return yday + day
end

local function day_of_week(day, month, year)
	if month < 3 then
		year = year - 1
	end
	return(year + leap_years_since(year) + sakamoto[month] + day) % 7 + 1
end

local function borrow(tens, units, base)
	local frac = tens % 1
	units = units + frac * base
	tens = tens - frac
	return tens, units
end

local function carry(tens, units, base)
	if units >= base then
		tens  = tens + idiv(units, base)
		units = units % base
	elseif units < 0 then
		tens  = tens + idiv(units, base)
		units = (base + units) % base
	end
	return tens, units
end

-- Modify parameters so they all fit within the "normal" range
local function normalise(year, month, day, hour, min, sec)
	-- `month` and `day` start from 1, need -1 and +1 so it works modulo
	month, day = month - 1, day - 1

	-- Convert everything (except seconds) to an integer
	-- by propagating fractional components down.
	year , month = borrow(year , month, 12)
	-- Carry from month to year first, so we get month length correct in next line around leap years
	year , month = carry(year, month, 12)
	month, day   = borrow(month, day  , month_length(floor(month + 1), year))
	day  , hour  = borrow(day  , hour , 24)
	hour , min   = borrow(hour , min  , 60)
	min  , sec   = borrow(min  , sec  , 60)

	-- Propagate out of range values up
	-- e.g. if `min` is 70, `hour` increments by 1 and `min` becomes 10
	-- This has to happen for all columns after borrowing, as lower radixes may be pushed out of range
	min  , sec   = carry(min , sec , 60) -- TODO: consider leap seconds?
	hour , min   = carry(hour, min , 60)
	day  , hour  = carry(day , hour, 24)
	-- Ensure `day` is not underflowed
	-- Add a whole year of days at a time, this is later resolved by adding months
	-- TODO[OPTIMIZE]: This could be slow if `day` is far out of range
	while day < 0 do
		month = month - 1
		if month < 0 then
			year = year - 1
			month = 11
		end
		day = day + month_length(month + 1, year)
	end
	year, month = carry(year, month, 12)

	-- TODO[OPTIMIZE]: This could potentially be slow if `day` is very large
	while true do
		local i = month_length(month + 1, year)
		if day < i then break end
		day = day - i
		month = month + 1
		if month >= 12 then
			month = 0
			year = year + 1
		end
	end

	-- Now we can place `day` and `month` back in their normal ranges
	-- e.g. month as 1-12 instead of 0-11
	month, day = month + 1, day + 1

	return year, month, day, hour, min, sec
end

local leap_years_since_1970 = leap_years_since(1970)
local function timestamp(year, month, day, hour, min, sec)
	year, month, day, hour, min, sec = normalise(year, month, day, hour, min, sec)

	local days_since_epoch = day_of_year(day, month, year)
		+ 365 * (year - 1970)
		-- Each leap year adds one day
		+ (leap_years_since(year - 1) - leap_years_since_1970) - 1

	return days_since_epoch * (60*60*24)
		+ hour * (60*60)
		+ min  * 60
		+ sec
end


local timetable_methods = {}

function timetable_methods:unpack()
	return assert(self.year , "year required"),
		assert(self.month, "month required"),
		assert(self.day  , "day required"),
		self.hour or 12,
		self.min  or 0,
		self.sec  or 0,
		self.yday,
		self.wday
end

function timetable_methods:normalise()
	local year, month, day
	year, month, day, self.hour, self.min, self.sec = normalise(self:unpack())

	self.day   = day
	self.month = month
	self.year  = year
	self.yday  = day_of_year(day, month, year)
	self.wday  = day_of_week(day, month, year)

	return self
end
timetable_methods.normalize = timetable_methods.normalise -- American English

function timetable_methods:timestamp()
	return timestamp(self:unpack())
end

function timetable_methods:rfc_3339()
	local year, month, day, hour, min, fsec = self:unpack()
	local sec, msec = borrow(fsec, 0, 1000)
	msec = math.floor(msec)
	return strformat("%04u-%02u-%02uT%02u:%02u:%02d.%03d", year, month, day, hour, min, sec, msec)
end

function timetable_methods:strftime(format_string)
	return strftime(format_string, self)
end

local timetable_mt

local function coerce_arg(t)
	if getmetatable(t) == timetable_mt then
		return t:timestamp()
	end
	return t
end

timetable_mt = {
	__index    = timetable_methods;
	__tostring = timetable_methods.rfc_3339;
	__eq = function(a, b)
		return a:timestamp() == b:timestamp()
	end;
	__lt = function(a, b)
		return a:timestamp() < b:timestamp()
	end;
	__sub = function(a, b)
		return coerce_arg(a) - coerce_arg(b)
	end;
}

local function cast_timetable(tm)
	return setmetatable(tm, timetable_mt)
end

local function new_timetable(year, month, day, hour, min, sec, yday, wday)
	return cast_timetable {
		year  = year;
		month = month;
		day   = day;
		hour  = hour;
		min   = min;
		sec   = sec;
		yday  = yday;
		wday  = wday;
	}
end

function timetable_methods:clone()
	return new_timetable(self:unpack())
end

local function new_from_timestamp(ts)
	if type(ts) ~= "number" then
		error("bad argument #1 to 'new_from_timestamp' (number expected, got " .. type(ts) .. ")", 2)
	end
	return new_timetable(1970, 1, 1, 0, 0, ts):normalise()
end

return {
	is_leap = is_leap;
	day_of_year = day_of_year;
	day_of_week = day_of_week;
	normalise = normalise;
	timestamp = timestamp;

	new = new_timetable;
	new_from_timestamp = new_from_timestamp;
	cast = cast_timetable;
	timetable_mt = timetable_mt;
}
]==] }
LUATZ[#LUATZ + 1] = { "tzcache", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local read_tzfile = require "luatz.tzfile".read_tzfile

local base_zoneinfo_path = "/usr/share/zoneinfo/"
local local_zoneinfo_path = "/etc/localtime"
local tz_cache = {}

local function name_to_zoneinfo_path(name)
	if name == nil then
		return local_zoneinfo_path
	elseif name:sub(1, 1) == "/" then
		return name
	else
		return base_zoneinfo_path .. name
	end
end

local function clear_tz_cache(name)
	tz_cache[name_to_zoneinfo_path(name)] = nil
end

local function get_tz(name)
	local path = name_to_zoneinfo_path(name)
	-- TODO: stat path
	local tzinfo = tz_cache[path]
	if tzinfo == nil then
		tzinfo = read_tzfile(path)
		tz_cache[path] = tzinfo
	end
	return tzinfo
end

local function set_base_path(path)
	-- Philement: reports.tz.zoneinfo_path hook (default /usr/share/zoneinfo).
	if type(path) == "string" and path ~= "" then
		base_zoneinfo_path = path:sub(-1) == "/" and path or path .. "/"
		tz_cache = {}
	end
end

return {
	get_tz = get_tz;
	clear_tz_cache = clear_tz_cache;
	set_base_path = set_base_path;
}
]==] }
LUATZ[#LUATZ + 1] = { "tzfile", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local tz_info_mt = require "luatz.tzinfo".tz_info_mt
local tt_info_mt = require "luatz.tzinfo".tt_info_mt

local read_int32be, read_int64be

-- luacheck: push std max
if string.unpack then
	-- Only available in Lua 5.3+
	function read_int32be(fd)
		local data, err = fd:read(4)
		if data == nil then return nil, err end
		return string.unpack(">i4", data)
	end

	function read_int64be(fd)
		local data, err = fd:read(8)
		if data == nil then return nil, err end
		return string.unpack(">i8", data)
	end
else -- luacheck: pop
	function read_int32be(fd)
		local data, err = fd:read(4)
		if data == nil then return nil, err end
		local o1, o2, o3, o4 = data:byte(1, 4)

		local unsigned = o4 + o3*2^8 + o2*2^16 + o1*2^24
		if unsigned >= 2^31 then
			return unsigned - 2^32
		else
			return unsigned
		end
	end

	function read_int64be(fd)
		local data, err = fd:read(8)
		if data == nil then return nil, err end
		local o1, o2, o3, o4, o5, o6, o7, o8 = data:byte(1, 8)

		local unsigned = o8 + o7*2^8 + o6*2^16 + o5*2^24 + o4*2^32 + o3*2^40 + o2*2^48 + o1*2^56
		if unsigned >= 2^63 then
			return unsigned - 2^64
		else
			return unsigned
		end
	end
end

local function read_flags(fd, n)
	local data, err = fd:read(n)
	if data == nil then return nil, err end

	local res = {}
	for i=1, n do
		res[i] = data:byte(i,i) ~= 0
	end
	return res
end

local fifteen_nulls = ("\0"):rep(15)
local function read_tz(fd)
	assert(fd:read(4) == "TZif", "Invalid TZ file")
	local version = assert(fd:read(1))
	if version == "\0" or version == "2" or version == "3" then
		local MIN_TIME = -2^32+1

		assert(assert(fd:read(15)) == fifteen_nulls, "Expected 15 nulls")

		-- The number of UTC/local indicators stored in the file.
		local tzh_ttisgmtcnt = assert(read_int32be(fd))

		-- The number of standard/wall indicators stored in the file.
		local tzh_ttisstdcnt = assert(read_int32be(fd))

		-- The number of leap seconds for which data is stored in the file.
		local tzh_leapcnt = assert(read_int32be(fd))

		-- The number of "transition times" for which data is stored in the file.
		local tzh_timecnt = assert(read_int32be(fd))

		-- The number of "local time types" for which data is stored in the file (must not be zero).
		local tzh_typecnt = assert(read_int32be(fd))

		-- The number of characters of "timezone abbreviation strings" stored in the file.
		local tzh_charcnt = assert(read_int32be(fd))

		local transition_times = {}
		for i=1, tzh_timecnt do
			transition_times[i] = assert(read_int32be(fd))
		end
		local transition_time_ind = {assert(fd:read(tzh_timecnt)):byte(1, -1)}

		local ttinfos = {}
		for i=1, tzh_typecnt do
			ttinfos[i] = {
				gmtoff = assert(read_int32be(fd));
				isdst  = assert(fd:read(1)) ~= "\0";
				abbrind = assert(fd:read(1)):byte();
			}
		end

		local abbreviations = assert(fd:read(tzh_charcnt))

		local leap_seconds = {} -- luacheck: ignore 241
		for i=1, tzh_leapcnt do
			leap_seconds[i] = {
				offset = assert(read_int32be(fd));
				n = assert(read_int32be(fd));
			}
		end

		local isstd = assert(read_flags(fd, tzh_ttisstdcnt))

		local isgmt = assert(read_flags(fd, tzh_ttisgmtcnt))

		local TZ

		if version == "2" or version == "3" then
			--[[
			For version-2-format timezone files, the above header and data is followed by a second header and data,
			identical in format except that eight bytes are used for each transition time or leap-second time.
			]]
			assert(fd:read(4) == "TZif")
			assert(fd:read(1) == version)
			assert(assert(fd:read(15)) == fifteen_nulls, "Expected 15 nulls")

			MIN_TIME = -2^64+1

			-- The number of UTC/local indicators stored in the file.
			tzh_ttisgmtcnt = assert(read_int32be(fd))

			-- The number of standard/wall indicators stored in the file.
			tzh_ttisstdcnt = assert(read_int32be(fd))

			-- The number of leap seconds for which data is stored in the file.
			tzh_leapcnt = assert(read_int32be(fd))

			-- The number of "transition times" for which data is stored in the file.
			tzh_timecnt = assert(read_int32be(fd))

			-- The number of "local time types" for which data is stored in the file (must not be zero).
			tzh_typecnt = assert(read_int32be(fd))

			-- The number of characters of "timezone abbreviation strings" stored in the file.
			tzh_charcnt = assert(read_int32be(fd))

			transition_times = {}
			for i=1, tzh_timecnt do
				transition_times[i] = assert(read_int64be(fd))
			end
			transition_time_ind = {assert(fd:read(tzh_timecnt)):byte(1, -1)}

			ttinfos = {}
			for i=1, tzh_typecnt do
				ttinfos[i] = {
					gmtoff = assert(read_int32be(fd));
					isdst  = assert(fd:read(1)) ~= "\0";
					abbrind = assert(fd:read(1)):byte();
				}
			end

			abbreviations = assert(fd:read(tzh_charcnt))

			leap_seconds = {}
			for i=1, tzh_leapcnt do
				leap_seconds[i] = {
					offset = assert(read_int64be(fd));
					n = assert(read_int32be(fd));
				}
			end

			isstd = assert(read_flags(fd, tzh_ttisstdcnt))

			isgmt = assert(read_flags(fd, tzh_ttisgmtcnt))

			--[[
			After the second header and data comes a newline-enclosed, POSIX-TZ-environment-variable-style string
			for use in handling instants after the last transition time stored in the file
			(with nothing between the newlines if there is no POSIX representation for such instants).
			]]

			--[[
			For version-3-format time zone files, the POSIX-TZ-style string may
			use two minor extensions to the POSIX TZ format, as described in newtzset (3).
			First, the hours part of its transition times may be signed and range from
			-167 through 167 instead of the POSIX-required unsigned values
			from 0 through 24.  Second, DST is in effect all year if it starts
			January 1 at 00:00 and ends December 31 at 24:00 plus the difference
			between daylight saving and standard time.
			]]

			assert(assert(fd:read(1)) == "\n", "Expected newline at end of version 2 header")

			TZ = assert(fd:read("*l"))
			if #TZ == 0 then
				TZ = nil
			end
		end

		for i=1, tzh_typecnt do
			local v = ttinfos[i]
			v.abbr = abbreviations:sub(v.abbrind+1, v.abbrind+3)
			v.isstd = isstd[i] or false
			v.isgmt = isgmt[i] or false
			setmetatable(v, tt_info_mt)
		end

		--[[
		Use the first standard-time ttinfo structure in the file
		(or simply the first ttinfo structure in the absence of a standard-time structure)
		if either tzh_timecnt is zero or the time argument is less than the first transition time recorded in the file.
		]]
		local first = 1
		do
			for i=1, tzh_ttisstdcnt do
				if isstd[i] then
					first = i
					break
				end
			end
		end

		local res = {
			future = TZ;
			[0] = {
				transition_time = MIN_TIME;
				info = ttinfos[first];
			}
		}
		for i=1, tzh_timecnt do
			res[i] = {
				transition_time = transition_times[i];
				info = ttinfos[transition_time_ind[i]+1];
			}
		end
		return setmetatable(res, tz_info_mt)
	else
		error("Unsupported version")
	end
end

local function read_tzfile(path)
	local fd = assert(io.open(path, "rb"))
	local tzinfo = read_tz(fd)
	fd:close()
	return tzinfo
end

return {
	read_tz = read_tz;
	read_tzfile = read_tzfile;
}
]==] }
LUATZ[#LUATZ + 1] = { "tzinfo", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
local gettime = require "luatz.gettime".gettime
local timetable_mt = require "luatz.timetable".timetable_mt

local function to_timestamp(o)
	if type(o) == "number" then
		return o
	elseif getmetatable(o) == timetable_mt then
		return o:timestamp()
	end
end

local tz_info_methods = { }
local tz_info_mt = {
	__name = "luatz.tz_info";
	__index = tz_info_methods;
}
local tt_info_mt = {
	__name = "luatz.tt_info";
	__tostring = function(self)
		return string.format("tt_info:%s=%d", self.abbr, self.gmtoff)
	end;
}

-- Binary search
local function find_current(tzinfo, target, i, j)
	if i >= j then return j end

	local half = math.ceil((j+i) / 2)

	if target >= tzinfo[half].transition_time then
		return find_current(tzinfo, target, half, j)
	else
		return find_current(tzinfo, target, i, half-1)
	end
end

local function find_current_local(tzinfo, ts_local)
	-- Find two best possibilities by searching back and forward a day (assumes transition is never by more than 24 hours)
	local tz_first = find_current(tzinfo, ts_local-86400, 0, #tzinfo)
	local tz_last  = find_current(tzinfo, ts_local+86400, 0, #tzinfo)

	local n_candidates = tz_last - tz_first + 1

	if n_candidates == 1 then
		return tz_first
	elseif n_candidates == 2 then
		local tz_first_ob = tzinfo[tz_first]
		local tz_last_ob  = tzinfo[tz_last]

		local first_gmtoffset = tz_first_ob.info.gmtoff
		local last_gmtoffset  = tz_last_ob .info.gmtoff

		local t_start = tz_last_ob.transition_time + first_gmtoffset
		local t_end   = tz_last_ob.transition_time + last_gmtoffset

		-- If timestamp is before start or after end
		if ts_local < t_start then
			return tz_first
		elseif ts_local > t_end then
			return tz_last
		end

		-- If we get this far, the local time is ambiguous
		return tz_first, tz_last
	else
		error("Too many transitions in a 2 day period")
	end
end

function tz_info_methods:find_current(current)
	current = assert(to_timestamp(current), "invalid timestamp to :find_current")
	return self[find_current(self, current, 0, #self)].info
end

function tz_info_methods:localise(utc_ts)
	utc_ts = utc_ts or gettime()
	return utc_ts + self:find_current(utc_ts).gmtoff
end
tz_info_methods.localize = tz_info_methods.localise

function tz_info_methods:utctime(ts_local)
	ts_local = assert(to_timestamp(ts_local), "invalid timestamp to :utctime")
	local tz1, tz2 = find_current_local(self, ts_local)
	tz1 = self[tz1].info
	if tz2 == nil then
		return ts_local - tz1.gmtoff
	else -- Local time is ambiguous
		tz2 = self[tz2].info

		return ts_local - tz2.gmtoff, ts_local - tz2.gmtoff
	end
end

return {
	tz_info_mt = tz_info_mt;
	tt_info_mt = tt_info_mt;
}
]==] }
LUATZ[#LUATZ + 1] = { "version", [==[
-- luatz v0.4-4-ga7c0310 a7c031006fad5511858b3bb70e7661a0aad5fc54 (github.com/daurnimator/luatz, MIT)
return "luatz v0.4-4-ga7c0310 (a7c031006fad5511858b3bb70e7661a0aad5fc54)"
]==] }

local sql_old = [==[
-- Reports.Render (Reporting Phase 22: compose with value format, Parameter items, paging, system datetime and user)
-- params: { stage = "compose", definition = {...}, sources = { name = [rows] } }
-- Returns the formatted intermediate { pages = [ { bands = [...] }, ... ], log = [...] }.
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Field values go through the item mask (Value format); null prints blank.
-- Paging: band heights against the page content box (page height less top/bottom margins).
-- A group header moves with its first detail row; headers do not repeat.
-- System datetime (5) prints one generation instant (params.now or H.system.now()) in the item timezone
-- (User | UTC | Explicit; report default zone definition.defaultZone). System user (6) prints _hydrogen.username.

local P = params or {}
local log = {}

local function add_log(level, message)
    log[#log + 1] = { level = level, message = message }
end

local function fail(message)
    add_log("error", message)
    H.set_result_json({ error = message, log = log })
    return 0
end

local FIELD_KEYS = { [20] = true, [21] = true, [24] = true }
local LATER_KEYS = {
    [42] = true, [43] = true, [44] = true, [45] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [35] = true, [38] = true,
}

local function plain(v)
    if v == nil then return "" end
    if math.type(v) == "float" then
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", math.floor(v))
        end
        local s = string.format("%.15g", v)
        if tonumber(s) ~= v then s = string.format("%.17g", v) end
        return s
    end
    return tostring(v)
end

-- Value format (Phase 20). Masks are locale-free; see REPORTING_PLAN Value format.
local function decimal_string(a)
    -- a >= 0. Shortest round-trip digits, never scientific.
    local s = string.format("%.15g", a)
    if tonumber(s) ~= a then s = string.format("%.17g", a) end
    local mant, exp = s:match("^([%d%.]+)e([-+]%d+)$")
    if not mant then return s end
    local ip, fp = mant:match("^(%d*)%.?(%d*)$")
    local digits, point = ip .. fp, #ip + tonumber(exp)
    if point <= 0 then return "0." .. string.rep("0", -point) .. digits end
    if point >= #digits then return digits .. string.rep("0", point - #digits) end
    return digits:sub(1, point) .. "." .. digits:sub(point + 1)
end

local function plain_number(v)
    if v ~= v or v == math.huge or v == -math.huge then return tostring(v) end
    if math.type(v) == "integer" then return tostring(v) end
    local s = decimal_string(math.abs(v))
    if s:find("%.") then s = s:gsub("0+$", ""):gsub("%.$", "") end
    if v < 0 and s:match("[1-9]") then s = "-" .. s end
    return s
end

local function round_digits(int, frac, places)
    -- Half away from zero on the decimal string (sign handled by caller).
    if #frac <= places then return int, frac .. string.rep("0", places - #frac) end
    local up = frac:byte(places + 1) >= 53
    local all = int .. frac:sub(1, places)
    if up then
        local d, i = {}, #all
        for k = 1, #all do d[k] = all:byte(k) - 48 end
        while i >= 1 do
            if d[i] == 9 then d[i] = 0 i = i - 1 else d[i] = d[i] + 1 break end
        end
        for k = 1, #all do d[k] = string.char(d[k] + 48) end
        all = (i == 0 and "1" or "") .. table.concat(d)
    end
    local n = #all - places
    return all:sub(1, n), all:sub(n + 1)
end

local function number_mask(mask)
    local m, paren = mask, false
    local inner = m:match("^%((.*)%)$")
    if inner then m, paren = inner, true end
    local prefix = ""
    if m:sub(1, 1) == "$" then prefix, m = "$", m:sub(2) end
    local pct = false
    if m:sub(-1) == "%" then pct, m = true, m:sub(1, -2) end
    local ip, fp = m:match("^([#0,]*)%.([#0]*)$")
    if not ip then ip, fp = m:match("^([#0,]*)$"), "" end
    if not ip or (ip .. fp) == "" or not (ip .. fp):find("[#0]") then return nil end
    if fp:find("#0") then return nil end -- zeros must lead the fraction
    local _, minint = ip:gsub("0", "")
    local _, minfrac = fp:gsub("0", "")
    return {
        paren = paren, prefix = prefix, pct = pct, comma = ip:find(",") ~= nil,
        minint = minint, minfrac = minfrac, maxfrac = #fp,
    }
end

local function apply_number(v, spec)
    local x = spec.pct and v * 100 or v
    local neg = x < 0
    local s = decimal_string(math.abs(x))
    local int, frac = s:match("^(%d*)%.?(%d*)$")
    int, frac = round_digits(int, frac, spec.maxfrac)
    int = int:gsub("^0+", "")
    if #int < spec.minint then int = string.rep("0", spec.minint - #int) .. int end
    while #frac > spec.minfrac and frac:sub(-1) == "0" do frac = frac:sub(1, -2) end
    if spec.comma and #int > 3 then
        int = int:reverse():gsub("(%d%d%d)", "%1,"):reverse():gsub("^,", "")
    end
    local body = spec.prefix .. int .. (frac ~= "" and ("." .. frac) or "") .. (spec.pct and "%" or "")
    if not (int .. frac):find("[1-9]") then neg = false end
    if neg then return spec.paren and ("(" .. body .. ")") or ("-" .. body) end
    return body
end

local MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }

local function civil_from_days(z)
    z = z + 719468
    local era = (z >= 0 and z or z - 146096) // 146097
    local doe = z - era * 146097
    local yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365
    local doy = doe - (365 * yoe + yoe // 4 - yoe // 100)
    local mp = (5 * doy + 2) // 153
    local d = doy - (153 * mp + 2) // 5 + 1
    local m = mp < 10 and mp + 3 or mp - 9
    return yoe + era * 400 + (m <= 2 and 1 or 0), m, d
end

local function parse_datetime(v)
    if type(v) == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return nil end
        local secs = math.floor(v)
        local days, rem = secs // 86400, secs % 86400
        local y, m, d = civil_from_days(days)
        return { y = y, m = m, d = d, hh = rem // 3600, nn = rem % 3600 // 60, ss = rem % 60, time = rem ~= 0 }
    end
    if type(v) ~= "string" then return nil end
    local y, m, d = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)$")
    local hh, nn, ss
    if not y then
        y, m, d, hh, nn, ss = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)$")
        if not y then return nil end
    end
    local t = { y = tonumber(y), m = tonumber(m), d = tonumber(d),
        hh = tonumber(hh) or 0, nn = tonumber(nn) or 0, ss = tonumber(ss) or 0, time = hh ~= nil }
    if t.m < 1 or t.m > 12 or t.d < 1 or t.d > 31 or t.hh > 23 or t.nn > 59 or t.ss > 59 then return nil end
    return t
end

local DATE_TOKENS = { "yyyy", "mmm", "yy", "mm", "dd", "hh", "nn", "ss", "d" }

local function apply_date(t, mask)
    local out, i, found = {}, 1, false
    while i <= #mask do
        local hit
        for _, tok in ipairs(DATE_TOKENS) do
            if mask:sub(i, i + #tok - 1) == tok then hit = tok break end
        end
        if hit == "yyyy" then out[#out + 1] = string.format("%04d", t.y)
        elseif hit == "yy" then out[#out + 1] = string.format("%02d", t.y % 100)
        elseif hit == "mmm" then out[#out + 1] = MONTHS[t.m]
        elseif hit == "mm" then out[#out + 1] = string.format("%02d", t.m)
        elseif hit == "dd" then out[#out + 1] = string.format("%02d", t.d)
        elseif hit == "d" then out[#out + 1] = tostring(t.d)
        elseif hit == "hh" then out[#out + 1] = string.format("%02d", t.hh)
        elseif hit == "nn" then out[#out + 1] = string.format("%02d", t.nn)
        elseif hit == "ss" then out[#out + 1] = string.format("%02d", t.ss)
        else out[#out + 1] = mask:sub(i, i) end
        if hit then found = true end
        i = i + (hit and #hit or 1)
    end
    return found and table.concat(out) or nil
end

local function raw(v)
    if type(v) == "number" then return plain_number(v) end
    return plain(v)
end

local warned_format = {}
local function format_warn(item_id, band_id, message)
    local k = tostring(band_id) .. "/" .. tostring(item_id)
    if warned_format[k] then return end
    warned_format[k] = true
    add_log("warn", "item " .. tostring(item_id) .. " in band " .. tostring(band_id) .. " " .. message)
end

-- typeKey 21 number, 24 datetime, 20 text. Null prints blank and skips the mask.
local function format_value(v, tk, mask, item_id, band_id)
    if v == nil then return "" end
    if mask == "" then mask = nil end
    if tk == 21 then
        local n = type(v) == "number" and v or (type(v) == "string" and tonumber(v)) or nil
        if mask == nil then return n and plain_number(n) or raw(v) end
        local spec = number_mask(mask)
        if not spec then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        if not n or n ~= n or n == math.huge or n == -math.huge then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a number; printed raw")
            return raw(v)
        end
        return apply_number(n, spec)
    elseif tk == 24 then
        local t = parse_datetime(v)
        if not t then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a date; printed raw")
            return raw(v)
        end
        if mask == nil then
            return t.time and apply_date(t, "yyyy-mm-dd hh:nn") or apply_date(t, "yyyy-mm-dd")
        end
        local s = apply_date(t, mask)
        if not s then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        return s
    end
    if type(v) == "number" then return plain_number(v) end
    return raw(v)
end

local function compose()
    local stage = P.stage or "compose"
    if stage ~= "compose" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose")
    end
    local def = P.definition
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

    -- Phase 22: one generation instant for every item; timezone User | UTC | Explicit.
    local HY = type(P._hydrogen) == "table" and P._hydrogen or {}
    local def_zone = type(def.defaultZone) == "string" and def.defaultZone ~= "" and def.defaultZone or nil
    local gen_now
    local function now()
        if gen_now == nil then
            local n = tonumber(P.now)
            if n == nil and H and H.system and H.system.now then n = tonumber(H.system.now()) end
            gen_now = math.floor(n or os.time())
        end
        return gen_now
    end
    local zone_logged = {}
    local function zone_log(level, message)
        if zone_logged[message] then return end
        zone_logged[message] = true
        add_log(level, message)
    end
    -- IANA zone at an instant -> offset seconds, or nil (caller prints UTC).
    local function iana_offset(zone, instant)
        local off = tonumber(HY.tzoffset)
        if zone == HY.tz and off then return math.floor(off * 60) end
        zone_log("info", "zone " .. zone .. " needs luatz (Phase 22a); printed UTC")
        return nil
    end
    local function zone_offset(item, instant)
        local mode = item.timezone or "User"
        if mode == "UTC" then return 0 end
        local zone
        if mode == "Explicit" then
            zone = type(item.zone) == "string" and item.zone ~= "" and item.zone or nil
            if not zone then
                zone_log("info", "item " .. tostring(item.id) .. " has Explicit timezone with no zone; printed UTC")
                return 0
            end
        else
            zone = type(HY.tz) == "string" and HY.tz ~= "" and HY.tz or def_zone
            if not zone then
                zone_log("info", "no user timezone or report default zone; printed UTC")
                return 0
            end
        end
        return iana_offset(zone, instant) or 0
    end
    local DT_DEFAULT = "yyyy-mm-dd hh:nn"
    local warned_user = false

    local declared = {}
    for _, ds in ipairs(def.datasources or {}) do
        if type(ds) == "table" and ds.name then declared[ds.name] = ds end
    end

    -- Sort bands into roles, keeping designer order inside each role.
    local rh, ph, gh, details, gf, pf, rf = {}, {}, {}, {}, {}, {}, {}
    local roles = { [12] = rh, [13] = ph, [14] = gh, [15] = details, [16] = gf, [17] = pf, [18] = rf }
    for _, b in ipairs(def.bands or {}) do
        local list = type(b) == "table" and roles[b.typeKey]
        if list then
            list[#list + 1] = b
        else
            add_log("warn", "band " .. tostring(type(b) == "table" and b.id) .. " has unknown typeKey "
                .. tostring(type(b) == "table" and b.typeKey) .. "; skipped")
        end
    end

    -- Pair each group header with its footer: same suffix (gh2/gf2), else nesting order.
    local used = {}
    local function suffix(id) return tostring(id or ""):match("^g[hf](.*)$") end
    local levels = {}
    for i, h in ipairs(gh) do
        local foot
        local hs = suffix(h.id)
        if hs then
            for j, f in ipairs(gf) do
                if not used[j] and suffix(f.id) == hs then foot = j break end
            end
        end
        if not foot then
            local j = #gf - i + 1
            if gf[j] and not used[j] then foot = j end
        end
        if foot then used[foot] = true end
        if h.field == nil or h.field == "" then
            add_log("warn", "group " .. tostring(h.id) .. " has no field; it wraps every row once")
        end
        levels[i] = { header = h, footer = foot and gf[foot] or nil }
    end
    for j, f in ipairs(gf) do
        if not used[j] then
            add_log("warn", "group footer " .. tostring(f.id) .. " has no group header; skipped")
        end
    end

    -- Rows: the first detail band with a datasource drives the report.
    local rows, driver
    local function rows_for(name, band_id)
        if not declared[name] then
            add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
                .. " which is not declared; no rows")
            return {}
        end
        local r = sources[name]
        if type(r) ~= "table" then
            add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
            return {}
        end
        return r
    end
    for _, d in ipairs(details) do
        if d.datasource ~= nil and d.datasource ~= "" then
            if not driver then
                driver = d.datasource
                rows = rows_for(driver, d.id)
            elseif d.datasource ~= driver then
                add_log("warn", "detail " .. tostring(d.id) .. " uses datasource " .. tostring(d.datasource)
                    .. "; rows come from " .. driver .. " in this version")
            end
        end
    end
    if not driver then
        rows = { {} } -- static report: each band once
    end

    -- Parameter items (63): "NAME" (first datasource with it) or "ds.NAME".
    local warned_param = {}
    local function param_value(ref)
        if ref == nil or ref == "" then return nil end
        ref = tostring(ref)
        local ds_name, pname = ref:match("^([^%.]+)%.(.+)$")
        for _, ds in ipairs(type(def.datasources) == "table" and def.datasources or {}) do
            if type(ds) == "table" and type(ds.params) == "table"
                and (ds_name == nil or ds.name == ds_name) then
                local v = ds.params[pname or ref]
                if v ~= nil then return v end
            end
        end
        if not warned_param[ref] then
            warned_param[ref] = true
            add_log("warn", "parameter " .. ref .. " is not set on any datasource; printed blank")
        end
        return nil
    end

    local warned_field = {}
    local function item_out(item, row, band_id)
        local tk = item.typeKey
        local text
        if tk == 0 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif FIELD_KEYS[tk] then
            if item.field == nil or item.field == "" then
                local k = tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[k] then
                    warned_field[k] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " has no field; printed blank")
                end
                text = ""
            else
                local v = row and row[item.field]
                -- Unix-number datetimes are instants and take the item timezone when set;
                -- civil strings never shift.
                if tk == 24 and item.timezone and type(v) == "number" then v = v + zone_offset(item, v) end
                text = format_value(v, tk, item.mask, item.id, band_id)
            end
        elseif tk == 5 then
            local s = now()
            local m = item.mask
            if m == nil or m == "" then m = DT_DEFAULT end
            text = format_value(s + zone_offset(item, s), 24, m, item.id, band_id)
        elseif tk == 6 then
            local u = HY.username
            if u == nil or u == "" then
                if not warned_user then
                    warned_user = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " is System user but the caller has no username; printed blank")
                end
                text = ""
            else
                text = plain(u)
            end
        elseif tk == 63 then
            local v = param_value(item.param)
            local ptk = type(v) == "number" and 21 or 20
            text = format_value(v, ptk, item.mask, item.id, band_id)
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        return {
            id = item.id,
            typeKey = tk,
            text = text,
            align = item.align or "Left",
            font = {
                family = f.family or rfont.family,
                size = f.size or rfont.size,
                style = f.style or rfont.style,
            },
            box = item.box,
        }
    end

    -- Paging (Phase 21): stored band heights against the page content box.
    local UNIT_PT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }
    local function to_pt(len)
        if type(len) == "number" then return len end
        if type(len) ~= "string" then return nil end
        local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
        n = tonumber(n)
        if not n or not UNIT_PT[u] then return nil end
        return n * UNIT_PT[u]
    end
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local box_h = to_pt(pg.height)
    if box_h then box_h = box_h - (to_pt(mg.top) or 0) - (to_pt(mg.bottom) or 0) end
    if not box_h or box_h <= 0 then
        add_log("warn", "page height " .. tostring(pg.height) .. " leaves no content box; no page breaks")
        box_h = math.huge
    end
    local warned_height = {}
    local function h_of(band)
        local h = to_pt(band.height)
        if h then return h end
        if not warned_height[band] then
            warned_height[band] = true
            add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
                .. " does not parse; counted as 0")
        end
        return 0
    end
    local EPS = 1e-6

    local pages, out, filled, body_start = {}, nil, 0, 0
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
        filled = filled + h_of(band)
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end
    local function sum(list)
        local s = 0
        for _, b in ipairs(list) do s = s + h_of(b) end
        return s
    end
    local pf_h = sum(pf)

    local first = rows[1]
    local function open_page(row)
        out, filled = {}, 0
        pages[#pages + 1] = { bands = out }
        if #pages == 1 then emit_all(rh, first) end
        emit_all(ph, row)
        body_start = filled
    end
    -- Make room for `need` before placing; break the page if the body already has bands.
    local function ensure(need, row, last, what)
        if filled + need + pf_h <= box_h + EPS then return end
        if filled > body_start + EPS then
            emit_all(pf, last)
            open_page(row)
        end
        if filled + need + pf_h > box_h + EPS then
            add_log("warn", what .. " does not fit on an empty page; placed anyway")
        end
    end

    open_page(first)

    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" then return "" end
        return plain(row[f])
    end

    local prev
    for _, row in ipairs(rows) do
        local brk = #levels + 1
        if prev == nil then
            brk = 1
        else
            for lv = 1, #levels do
                if key(row, lv) ~= key(prev, lv) then brk = lv break end
            end
        end
        if prev ~= nil then
            for lv = #levels, brk, -1 do
                local f = levels[lv].footer
                if f then
                    ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                    emit(f, prev)
                end
            end
        end
        if brk <= #levels then
            -- Group headers travel with the first detail row of the group.
            local need, ids = sum(details), {}
            for lv = brk, #levels do
                need = need + h_of(levels[lv].header)
                ids[#ids + 1] = tostring(levels[lv].header.id)
            end
            ensure(need, row, prev or first, "group header " .. table.concat(ids, ", ") .. " with its first detail row")
            for lv = brk, #levels do emit(levels[lv].header, row) end
            emit_all(details, row)
        else
            for _, d in ipairs(details) do
                ensure(h_of(d), row, prev, "detail " .. tostring(d.id))
                emit(d, row)
            end
        end
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            local f = levels[lv].footer
            if f then
                ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                emit(f, prev)
            end
        end
    end

    -- Report footer sits on the last page; it prints after the page footer as before.
    local last = prev or first
    local rf_h = sum(rf)
    if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
    emit_all(pf, last)
    emit_all(rf, last)

    H.set_result_json({ pages = pages, log = log })
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
            ]==]
local sql_new = [==[
-- Reports.Render (Reporting Phase 22a: compose with value format, Parameter items, paging, system datetime and user)
-- params: { stage = "compose", definition = {...}, sources = { name = [rows] } }
-- Returns the formatted intermediate { pages = [ { bands = [...] }, ... ], log = [...] }.
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Field values go through the item mask (Value format); null prints blank.
-- Paging: band heights against the page content box (page height less top/bottom margins).
-- A group header moves with its first detail row; headers do not repeat.
-- System datetime (5) prints one generation instant (params.now or H.system.now()) in the item timezone
-- (User | UTC | Explicit; report default zone definition.defaultZone). System user (6) prints _hydrogen.username.
-- IANA zones resolve through luatz on host tzdata (/usr/share/zoneinfo); unknown zone -> UTC + warn.

local P = params or {}
local log = {}

local function add_log(level, message)
    log[#log + 1] = { level = level, message = message }
end

local function fail(message)
    add_log("error", message)
    H.set_result_json({ error = message, log = log })
    return 0
end

local FIELD_KEYS = { [20] = true, [21] = true, [24] = true }
local LATER_KEYS = {
    [42] = true, [43] = true, [44] = true, [45] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [35] = true, [38] = true,
}

local function plain(v)
    if v == nil then return "" end
    if math.type(v) == "float" then
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", math.floor(v))
        end
        local s = string.format("%.15g", v)
        if tonumber(s) ~= v then s = string.format("%.17g", v) end
        return s
    end
    return tostring(v)
end

-- Value format (Phase 20). Masks are locale-free; see REPORTING_PLAN Value format.
local function decimal_string(a)
    -- a >= 0. Shortest round-trip digits, never scientific.
    local s = string.format("%.15g", a)
    if tonumber(s) ~= a then s = string.format("%.17g", a) end
    local mant, exp = s:match("^([%d%.]+)e([-+]%d+)$")
    if not mant then return s end
    local ip, fp = mant:match("^(%d*)%.?(%d*)$")
    local digits, point = ip .. fp, #ip + tonumber(exp)
    if point <= 0 then return "0." .. string.rep("0", -point) .. digits end
    if point >= #digits then return digits .. string.rep("0", point - #digits) end
    return digits:sub(1, point) .. "." .. digits:sub(point + 1)
end

local function plain_number(v)
    if v ~= v or v == math.huge or v == -math.huge then return tostring(v) end
    if math.type(v) == "integer" then return tostring(v) end
    local s = decimal_string(math.abs(v))
    if s:find("%.") then s = s:gsub("0+$", ""):gsub("%.$", "") end
    if v < 0 and s:match("[1-9]") then s = "-" .. s end
    return s
end

local function round_digits(int, frac, places)
    -- Half away from zero on the decimal string (sign handled by caller).
    if #frac <= places then return int, frac .. string.rep("0", places - #frac) end
    local up = frac:byte(places + 1) >= 53
    local all = int .. frac:sub(1, places)
    if up then
        local d, i = {}, #all
        for k = 1, #all do d[k] = all:byte(k) - 48 end
        while i >= 1 do
            if d[i] == 9 then d[i] = 0 i = i - 1 else d[i] = d[i] + 1 break end
        end
        for k = 1, #all do d[k] = string.char(d[k] + 48) end
        all = (i == 0 and "1" or "") .. table.concat(d)
    end
    local n = #all - places
    return all:sub(1, n), all:sub(n + 1)
end

local function number_mask(mask)
    local m, paren = mask, false
    local inner = m:match("^%((.*)%)$")
    if inner then m, paren = inner, true end
    local prefix = ""
    if m:sub(1, 1) == "$" then prefix, m = "$", m:sub(2) end
    local pct = false
    if m:sub(-1) == "%" then pct, m = true, m:sub(1, -2) end
    local ip, fp = m:match("^([#0,]*)%.([#0]*)$")
    if not ip then ip, fp = m:match("^([#0,]*)$"), "" end
    if not ip or (ip .. fp) == "" or not (ip .. fp):find("[#0]") then return nil end
    if fp:find("#0") then return nil end -- zeros must lead the fraction
    local _, minint = ip:gsub("0", "")
    local _, minfrac = fp:gsub("0", "")
    return {
        paren = paren, prefix = prefix, pct = pct, comma = ip:find(",") ~= nil,
        minint = minint, minfrac = minfrac, maxfrac = #fp,
    }
end

local function apply_number(v, spec)
    local x = spec.pct and v * 100 or v
    local neg = x < 0
    local s = decimal_string(math.abs(x))
    local int, frac = s:match("^(%d*)%.?(%d*)$")
    int, frac = round_digits(int, frac, spec.maxfrac)
    int = int:gsub("^0+", "")
    if #int < spec.minint then int = string.rep("0", spec.minint - #int) .. int end
    while #frac > spec.minfrac and frac:sub(-1) == "0" do frac = frac:sub(1, -2) end
    if spec.comma and #int > 3 then
        int = int:reverse():gsub("(%d%d%d)", "%1,"):reverse():gsub("^,", "")
    end
    local body = spec.prefix .. int .. (frac ~= "" and ("." .. frac) or "") .. (spec.pct and "%" or "")
    if not (int .. frac):find("[1-9]") then neg = false end
    if neg then return spec.paren and ("(" .. body .. ")") or ("-" .. body) end
    return body
end

local MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }

local function civil_from_days(z)
    z = z + 719468
    local era = (z >= 0 and z or z - 146096) // 146097
    local doe = z - era * 146097
    local yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365
    local doy = doe - (365 * yoe + yoe // 4 - yoe // 100)
    local mp = (5 * doy + 2) // 153
    local d = doy - (153 * mp + 2) // 5 + 1
    local m = mp < 10 and mp + 3 or mp - 9
    return yoe + era * 400 + (m <= 2 and 1 or 0), m, d
end

local function parse_datetime(v)
    if type(v) == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return nil end
        local secs = math.floor(v)
        local days, rem = secs // 86400, secs % 86400
        local y, m, d = civil_from_days(days)
        return { y = y, m = m, d = d, hh = rem // 3600, nn = rem % 3600 // 60, ss = rem % 60, time = rem ~= 0 }
    end
    if type(v) ~= "string" then return nil end
    local y, m, d = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)$")
    local hh, nn, ss
    if not y then
        y, m, d, hh, nn, ss = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)$")
        if not y then return nil end
    end
    local t = { y = tonumber(y), m = tonumber(m), d = tonumber(d),
        hh = tonumber(hh) or 0, nn = tonumber(nn) or 0, ss = tonumber(ss) or 0, time = hh ~= nil }
    if t.m < 1 or t.m > 12 or t.d < 1 or t.d > 31 or t.hh > 23 or t.nn > 59 or t.ss > 59 then return nil end
    return t
end

local DATE_TOKENS = { "yyyy", "mmm", "yy", "mm", "dd", "hh", "nn", "ss", "d" }

local function apply_date(t, mask)
    local out, i, found = {}, 1, false
    while i <= #mask do
        local hit
        for _, tok in ipairs(DATE_TOKENS) do
            if mask:sub(i, i + #tok - 1) == tok then hit = tok break end
        end
        if hit == "yyyy" then out[#out + 1] = string.format("%04d", t.y)
        elseif hit == "yy" then out[#out + 1] = string.format("%02d", t.y % 100)
        elseif hit == "mmm" then out[#out + 1] = MONTHS[t.m]
        elseif hit == "mm" then out[#out + 1] = string.format("%02d", t.m)
        elseif hit == "dd" then out[#out + 1] = string.format("%02d", t.d)
        elseif hit == "d" then out[#out + 1] = tostring(t.d)
        elseif hit == "hh" then out[#out + 1] = string.format("%02d", t.hh)
        elseif hit == "nn" then out[#out + 1] = string.format("%02d", t.nn)
        elseif hit == "ss" then out[#out + 1] = string.format("%02d", t.ss)
        else out[#out + 1] = mask:sub(i, i) end
        if hit then found = true end
        i = i + (hit and #hit or 1)
    end
    return found and table.concat(out) or nil
end

local function raw(v)
    if type(v) == "number" then return plain_number(v) end
    return plain(v)
end

local warned_format = {}
local function format_warn(item_id, band_id, message)
    local k = tostring(band_id) .. "/" .. tostring(item_id)
    if warned_format[k] then return end
    warned_format[k] = true
    add_log("warn", "item " .. tostring(item_id) .. " in band " .. tostring(band_id) .. " " .. message)
end

-- typeKey 21 number, 24 datetime, 20 text. Null prints blank and skips the mask.
local function format_value(v, tk, mask, item_id, band_id)
    if v == nil then return "" end
    if mask == "" then mask = nil end
    if tk == 21 then
        local n = type(v) == "number" and v or (type(v) == "string" and tonumber(v)) or nil
        if mask == nil then return n and plain_number(n) or raw(v) end
        local spec = number_mask(mask)
        if not spec then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        if not n or n ~= n or n == math.huge or n == -math.huge then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a number; printed raw")
            return raw(v)
        end
        return apply_number(n, spec)
    elseif tk == 24 then
        local t = parse_datetime(v)
        if not t then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a date; printed raw")
            return raw(v)
        end
        if mask == nil then
            return t.time and apply_date(t, "yyyy-mm-dd hh:nn") or apply_date(t, "yyyy-mm-dd")
        end
        local s = apply_date(t, mask)
        if not s then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        return s
    end
    if type(v) == "number" then return plain_number(v) end
    return raw(v)
end

local function compose()
    local stage = P.stage or "compose"
    if stage ~= "compose" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose")
    end
    local def = P.definition
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

    -- Phase 22: one generation instant for every item; timezone User | UTC | Explicit.
    local HY = type(P._hydrogen) == "table" and P._hydrogen or {}
    local def_zone = type(def.defaultZone) == "string" and def.defaultZone ~= "" and def.defaultZone or nil
    local gen_now
    local function now()
        if gen_now == nil then
            local n = tonumber(P.now)
            if n == nil and H and H.system and H.system.now then n = tonumber(H.system.now()) end
            gen_now = math.floor(n or os.time())
        end
        return gen_now
    end
    local zone_logged = {}
    local function zone_log(level, message)
        if zone_logged[message] then return end
        zone_logged[message] = true
        add_log(level, message)
    end
    -- Phase 22a: IANA zones resolve through luatz (group luatz) on host tzdata.
    -- Config hooks (env, defaults hardcoded): REPORTS_TZ_ZONEINFO_PATH (reports.tz.zoneinfo_path),
    -- REPORTS_TZ_DEFAULT_ZONE (reports.tz.default_zone, used when the report has no defaultZone).
    local function cfg_env(name)
        local ok, v = pcall(function() return os.getenv and os.getenv(name) end)
        return ok and type(v) == "string" and v ~= "" and v or nil
    end
    local TZ_PATH = cfg_env("REPORTS_TZ_ZONEINFO_PATH") or "/usr/share/zoneinfo"
    def_zone = def_zone or cfg_env("REPORTS_TZ_DEFAULT_ZONE")
    local tzc
    local function iana_offset(zone, instant)
        if not zone:match("^[%w_+%-/]+$") or zone:find("..", 1, true) or zone:sub(1, 1) == "/" then
            zone_log("warn", "zone " .. zone .. " is not an IANA name; printed UTC")
            return nil
        end
        if tzc == nil then
            local ok, m = pcall(require, "luatz.tzcache")
            tzc = ok and type(m) == "table" and m or false
            if not tzc then
                zone_log("warn", "luatz is not available (" .. tostring(m) .. "); printed UTC")
            elseif tzc.set_base_path then
                tzc.set_base_path(TZ_PATH)
            end
        end
        if not tzc then return nil end
        if io == nil then
            zone_log("warn", "zone " .. zone .. " needs read access to " .. TZ_PATH .. " (io is sandboxed); printed UTC")
            return nil
        end
        local ok, tz = pcall(tzc.get_tz, zone)
        if not ok or type(tz) ~= "table" then
            zone_log("warn", "zone " .. zone .. " is unknown or unreadable under " .. TZ_PATH .. "; printed UTC")
            return nil
        end
        local ok2, info = pcall(tz.find_current, tz, instant)
        if not ok2 or type(info) ~= "table" then
            zone_log("warn", "zone " .. zone .. " has no offset at " .. tostring(instant) .. "; printed UTC")
            return nil
        end
        return info.gmtoff
    end
    local function zone_offset(item, instant)
        local mode = item.timezone or "User"
        if mode == "UTC" then return 0 end
        local zone
        if mode == "Explicit" then
            zone = type(item.zone) == "string" and item.zone ~= "" and item.zone or nil
            if not zone then
                zone_log("info", "item " .. tostring(item.id) .. " has Explicit timezone with no zone; printed UTC")
                return 0
            end
        else
            -- The caller's own claim keeps its login offset (tzoffset drift accepted).
            local off = tonumber(HY.tzoffset)
            if type(HY.tz) == "string" and HY.tz ~= "" and off then return math.floor(off * 60) end
            zone = type(HY.tz) == "string" and HY.tz ~= "" and HY.tz or def_zone
            if not zone then
                zone_log("info", "no user timezone or report default zone; printed UTC")
                return 0
            end
        end
        return iana_offset(zone, instant) or 0
    end
    local DT_DEFAULT = "yyyy-mm-dd hh:nn"
    local warned_user = false

    local declared = {}
    for _, ds in ipairs(def.datasources or {}) do
        if type(ds) == "table" and ds.name then declared[ds.name] = ds end
    end

    -- Sort bands into roles, keeping designer order inside each role.
    local rh, ph, gh, details, gf, pf, rf = {}, {}, {}, {}, {}, {}, {}
    local roles = { [12] = rh, [13] = ph, [14] = gh, [15] = details, [16] = gf, [17] = pf, [18] = rf }
    for _, b in ipairs(def.bands or {}) do
        local list = type(b) == "table" and roles[b.typeKey]
        if list then
            list[#list + 1] = b
        else
            add_log("warn", "band " .. tostring(type(b) == "table" and b.id) .. " has unknown typeKey "
                .. tostring(type(b) == "table" and b.typeKey) .. "; skipped")
        end
    end

    -- Pair each group header with its footer: same suffix (gh2/gf2), else nesting order.
    local used = {}
    local function suffix(id) return tostring(id or ""):match("^g[hf](.*)$") end
    local levels = {}
    for i, h in ipairs(gh) do
        local foot
        local hs = suffix(h.id)
        if hs then
            for j, f in ipairs(gf) do
                if not used[j] and suffix(f.id) == hs then foot = j break end
            end
        end
        if not foot then
            local j = #gf - i + 1
            if gf[j] and not used[j] then foot = j end
        end
        if foot then used[foot] = true end
        if h.field == nil or h.field == "" then
            add_log("warn", "group " .. tostring(h.id) .. " has no field; it wraps every row once")
        end
        levels[i] = { header = h, footer = foot and gf[foot] or nil }
    end
    for j, f in ipairs(gf) do
        if not used[j] then
            add_log("warn", "group footer " .. tostring(f.id) .. " has no group header; skipped")
        end
    end

    -- Rows: the first detail band with a datasource drives the report.
    local rows, driver
    local function rows_for(name, band_id)
        if not declared[name] then
            add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
                .. " which is not declared; no rows")
            return {}
        end
        local r = sources[name]
        if type(r) ~= "table" then
            add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
            return {}
        end
        return r
    end
    for _, d in ipairs(details) do
        if d.datasource ~= nil and d.datasource ~= "" then
            if not driver then
                driver = d.datasource
                rows = rows_for(driver, d.id)
            elseif d.datasource ~= driver then
                add_log("warn", "detail " .. tostring(d.id) .. " uses datasource " .. tostring(d.datasource)
                    .. "; rows come from " .. driver .. " in this version")
            end
        end
    end
    if not driver then
        rows = { {} } -- static report: each band once
    end

    -- Parameter items (63): "NAME" (first datasource with it) or "ds.NAME".
    local warned_param = {}
    local function param_value(ref)
        if ref == nil or ref == "" then return nil end
        ref = tostring(ref)
        local ds_name, pname = ref:match("^([^%.]+)%.(.+)$")
        for _, ds in ipairs(type(def.datasources) == "table" and def.datasources or {}) do
            if type(ds) == "table" and type(ds.params) == "table"
                and (ds_name == nil or ds.name == ds_name) then
                local v = ds.params[pname or ref]
                if v ~= nil then return v end
            end
        end
        if not warned_param[ref] then
            warned_param[ref] = true
            add_log("warn", "parameter " .. ref .. " is not set on any datasource; printed blank")
        end
        return nil
    end

    local warned_field = {}
    local function item_out(item, row, band_id)
        local tk = item.typeKey
        local text
        if tk == 0 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif FIELD_KEYS[tk] then
            if item.field == nil or item.field == "" then
                local k = tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[k] then
                    warned_field[k] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " has no field; printed blank")
                end
                text = ""
            else
                local v = row and row[item.field]
                -- Unix-number datetimes are instants and take the item timezone when set;
                -- civil strings never shift.
                if tk == 24 and item.timezone and type(v) == "number" then v = v + zone_offset(item, v) end
                text = format_value(v, tk, item.mask, item.id, band_id)
            end
        elseif tk == 5 then
            local s = now()
            local m = item.mask
            if m == nil or m == "" then m = DT_DEFAULT end
            text = format_value(s + zone_offset(item, s), 24, m, item.id, band_id)
        elseif tk == 6 then
            local u = HY.username
            if u == nil or u == "" then
                if not warned_user then
                    warned_user = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " is System user but the caller has no username; printed blank")
                end
                text = ""
            else
                text = plain(u)
            end
        elseif tk == 63 then
            local v = param_value(item.param)
            local ptk = type(v) == "number" and 21 or 20
            text = format_value(v, ptk, item.mask, item.id, band_id)
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        return {
            id = item.id,
            typeKey = tk,
            text = text,
            align = item.align or "Left",
            font = {
                family = f.family or rfont.family,
                size = f.size or rfont.size,
                style = f.style or rfont.style,
            },
            box = item.box,
        }
    end

    -- Paging (Phase 21): stored band heights against the page content box.
    local UNIT_PT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }
    local function to_pt(len)
        if type(len) == "number" then return len end
        if type(len) ~= "string" then return nil end
        local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
        n = tonumber(n)
        if not n or not UNIT_PT[u] then return nil end
        return n * UNIT_PT[u]
    end
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local box_h = to_pt(pg.height)
    if box_h then box_h = box_h - (to_pt(mg.top) or 0) - (to_pt(mg.bottom) or 0) end
    if not box_h or box_h <= 0 then
        add_log("warn", "page height " .. tostring(pg.height) .. " leaves no content box; no page breaks")
        box_h = math.huge
    end
    local warned_height = {}
    local function h_of(band)
        local h = to_pt(band.height)
        if h then return h end
        if not warned_height[band] then
            warned_height[band] = true
            add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
                .. " does not parse; counted as 0")
        end
        return 0
    end
    local EPS = 1e-6

    local pages, out, filled, body_start = {}, nil, 0, 0
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
        filled = filled + h_of(band)
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end
    local function sum(list)
        local s = 0
        for _, b in ipairs(list) do s = s + h_of(b) end
        return s
    end
    local pf_h = sum(pf)

    local first = rows[1]
    local function open_page(row)
        out, filled = {}, 0
        pages[#pages + 1] = { bands = out }
        if #pages == 1 then emit_all(rh, first) end
        emit_all(ph, row)
        body_start = filled
    end
    -- Make room for `need` before placing; break the page if the body already has bands.
    local function ensure(need, row, last, what)
        if filled + need + pf_h <= box_h + EPS then return end
        if filled > body_start + EPS then
            emit_all(pf, last)
            open_page(row)
        end
        if filled + need + pf_h > box_h + EPS then
            add_log("warn", what .. " does not fit on an empty page; placed anyway")
        end
    end

    open_page(first)

    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" then return "" end
        return plain(row[f])
    end

    local prev
    for _, row in ipairs(rows) do
        local brk = #levels + 1
        if prev == nil then
            brk = 1
        else
            for lv = 1, #levels do
                if key(row, lv) ~= key(prev, lv) then brk = lv break end
            end
        end
        if prev ~= nil then
            for lv = #levels, brk, -1 do
                local f = levels[lv].footer
                if f then
                    ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                    emit(f, prev)
                end
            end
        end
        if brk <= #levels then
            -- Group headers travel with the first detail row of the group.
            local need, ids = sum(details), {}
            for lv = brk, #levels do
                need = need + h_of(levels[lv].header)
                ids[#ids + 1] = tostring(levels[lv].header.id)
            end
            ensure(need, row, prev or first, "group header " .. table.concat(ids, ", ") .. " with its first detail row")
            for lv = brk, #levels do emit(levels[lv].header, row) end
            emit_all(details, row)
        else
            for _, d in ipairs(details) do
                ensure(h_of(d), row, prev, "detail " .. tostring(d.id))
                emit(d, row)
            end
        end
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            local f = levels[lv].footer
            if f then
                ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                emit(f, prev)
            end
        end
    end

    -- Report footer sits on the last page; it prints after the page footer as before.
    local last = prev or first
    local rf_h = sum(rf)
    if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
    emit_all(pf, last)
    emit_all(rf, last)

    H.set_result_json({ pages = pages, log = log })
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
            ]==]

local seed, unseed = {}, {}
for _, m in ipairs(LUATZ) do
    seed[#seed + 1] = [[
            INSERT INTO ${SCHEMA}scripts (
                group_name,
                script_name,
                script_type,
                schedule,
                next_run,
                last_run_start,
                last_run_end,
                status,
                code,
                summary,
                invokable,
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                'luatz',
                ']] .. m[1] .. [[',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
]] .. m[2] .. [[
                ]==],
                'luatz.]] .. m[1] .. [[ (luatz v0.4-4-ga7c0310, library pack)',
                0,
                0,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}
]]
    unseed[#unseed + 1] = "'" .. m[1] .. "'"
end
local seed_sql = table.concat(seed)
local unseed_list = table.concat(unseed, ", ")

-- ----------------------------------------------------------------------------
-- Forward: seed luatz, then the Render body
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
]] .. seed_sql .. [[

            UPDATE ${SCHEMA}scripts
            SET code = [==[
]] .. sql_new .. [[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed luatz; Reports.Render IANA zones'                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed luatz and resolve IANA zones

            Seeds luatz v0.4-4-ga7c0310 (commit a7c0310, MIT) as group
            `luatz` scripts, invokable=0, mcp_access=0. zoneinfo comes
            from host tzdata (/usr/share/zoneinfo), not this migration.
            Reports.Render resolves Explicit and report-default zones
            through luatz at the generation instant; unknown zone ->
            UTC + warn. Refresh: tzdata with image rebuild; luatz by a
            later migration bumping luatz.version. Config hooks
            REPORTS_TZ_ZONEINFO_PATH / REPORTS_TZ_DEFAULT_ZONE (env,
            home TBD). Render needs io available to read tzfiles.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: restore the 1451 body, delete the luatz rows
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            UPDATE ${SCHEMA}scripts
            SET code = [==[
]] .. sql_old .. [[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}scripts
            WHERE group_name = 'luatz'
              AND script_name IN (]] .. unseed_list .. [[);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore 1451 Reports.Render; remove luatz'                         AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1451 Render, remove luatz

            Puts the acuranzo_1451.lua body back and deletes the group
            `luatz` script rows this migration seeded.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
