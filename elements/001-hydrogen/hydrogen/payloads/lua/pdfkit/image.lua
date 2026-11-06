-- image.lua -- port of pdfkit/lib/image.js
--
-- Works out what kind of image it has been handed and returns the right reader.
-- Format detection is by magic bytes, not by file extension: a .png that is
-- really a JPEG embeds correctly.
--
-- ONE THING IS DIFFERENT HERE. JavaScript can tell a Buffer from a path string
-- by its type; Lua cannot, because both are strings. So the order of tests
-- below is the disambiguation:
--
--   1. does it start with a JPEG or PNG signature?   -> image data
--   2. does it look like a data: URI?                -> base64 image data
--   3. otherwise                                     -> a filename
--
-- Image data is never mistaken for a path, since no filename begins with 0xFF
-- 0xD8. A path is never mistaken for image data for the same reason. The only
-- casualty is a file whose *name* happens to start with a PNG signature, which
-- is not a thing.

local base64 = require("pdfkit.base64")
local JPEG = require("pdfkit.image.jpeg")
local PNG = require("pdfkit.image.png")

local byte = string.byte

local M = {}

local function read_file(path)
    local f, err = io.open(path, "rb")
    if not f then
        error("pdfkit: cannot open image " .. tostring(path) .. ": " .. tostring(err))
    end
    local data = f:read("a")
    f:close()
    return data
end

local function is_jpeg(data)
    return byte(data, 1) == 0xFF and byte(data, 2) == 0xD8
end

local function is_png(data)
    return byte(data, 1) == 0x89 and byte(data, 2) == 0x50
        and byte(data, 3) == 0x4E and byte(data, 4) == 0x47
end

--- Open `src` and return a JPEG or PNG reader labelled `label`.
function M.open(src, label)
    local data

    if type(src) ~= "string" then
        error("pdfkit: image source must be a string (data or filename)")
    end

    if is_jpeg(src) or is_png(src) then
        data = src
    else
        local b64 = src:match("^data:.-;base64,(.*)$")
        if b64 then
            data = base64.decode(b64)
        else
            data = read_file(src)
        end
    end

    if is_jpeg(data) then
        return JPEG.new(data, label)
    elseif is_png(data) then
        return PNG.new(data, label)
    else
        error("Unknown image format.")
    end
end

return M
