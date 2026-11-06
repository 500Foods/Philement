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
