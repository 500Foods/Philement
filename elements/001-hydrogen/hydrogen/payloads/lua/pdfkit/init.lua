-- init.lua -- the front door of pdfkit
--
-- PDFKit's entry point is the PDFDocument class and nothing else, and that is
-- the shape this keeps:
--
--     local PDFDocument = require("pdfkit")
--
--     local doc = PDFDocument.new({ size = "LETTER", compress = false })
--     doc:fontSize(16)
--     doc:text("hello, world", 72, 72)
--
--     local out = assert(io.open("out.pdf", "wb"))
--     doc:pipe(out)
--     doc:finish()
--     out:close()
--
-- TWO NAMES DIFFER FROM THE ORIGINAL, both because Lua reserves the word:
--
--     doc.end()   ->  doc:finish()
--     ref.end()   ->  ref:finish()
--
-- Everything else keeps PDFKit's own spelling, camelCase included, so that a
-- JavaScript example transliterates line for line.
--
-- The submodules below are exported for callers who want a piece of the machine
-- rather than the whole document -- reading a PNG's dimensions without embedding
-- it, say, or formatting a number the way the PDF writer does. They are not
-- needed for ordinary use, and PDFKit does not expose equivalents.

local Document = require("pdfkit.document")

local M = {}

--- Create a document. See document.lua for the option list.
M.new = Document.new

-- The class itself, for anyone wanting to add a method of their own -- the same
-- thing the mixins do.
M.Document = Document

-- Building blocks.
M.object = require("pdfkit.object")
M.utils = require("pdfkit.utils")
M.Reference = require("pdfkit.reference")
M.Page = require("pdfkit.page")

-- Fonts, images and compression, usable on their own.
M.Font = require("pdfkit.font")
M.Image = require("pdfkit.image")
M.PNGReader = require("pdfkit.png_reader")
M.flate = require("pdfkit.flate")

-- Text layout.
M.LineBreaker = require("pdfkit.line_breaker")
M.LineWrapper = require("pdfkit.line_wrapper")

-- SVG path parsing and playback.
M.SVGPath = require("pdfkit.path")

return M
