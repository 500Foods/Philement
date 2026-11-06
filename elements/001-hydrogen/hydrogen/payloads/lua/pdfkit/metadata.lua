-- metadata.lua -- port of pdfkit/lib/metadata.js
--
-- The XMP packet: an XML document describing the file, accumulated as a string
-- and closed off at the end. It only reaches the output for PDF 1.4 and later;
-- at 1.3 it is built and then discarded, which is why a default document has no
-- metadata stream even though this ran.
--
-- The indentation below is not decorative. The packet is compared byte for byte
-- against the original's, so every space in these literals is significant.

local PDFMetadata = {}
PDFMetadata.__index = PDFMetadata

function PDFMetadata.new()
    local self = setmetatable({}, PDFMetadata)

    -- \239\187\191 is the UTF-8 encoding of U+FEFF, which the JS writes as the
    -- escape ﻿ inside the packet header.
    self._metadata = table.concat({
        "\n",
        '        <?xpacket begin="\239\187\191" id="W5M0MpCehiHzreSzNTczkc9d"?>\n',
        '            <x:xmpmeta xmlns:x="adobe:ns:meta/">\n',
        '                <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">\n',
        "        ",
    })

    return self
end

function PDFMetadata:_closeTags()
    self._metadata = self._metadata .. table.concat({
        "\n",
        "                </rdf:RDF>\n",
        "            </x:xmpmeta>\n",
        '        <?xpacket end="w"?>\n',
        "        ",
    })
end

function PDFMetadata:append(xml, newline)
    if newline == nil then newline = true end
    self._metadata = self._metadata .. xml
    if newline then
        self._metadata = self._metadata .. "\n"
    end
end

function PDFMetadata:getXML()
    return self._metadata
end

-- The JS reads String.length here, which counts UTF-16 code units, NOT bytes.
-- The packet already contains a BOM -- one UTF-16 unit, three UTF-8 bytes -- so
-- the two counts differ from the very first character, and the value ends up in
-- the output. Counting bytes would be the more sensible thing to report and the
-- wrong thing to port.
function PDFMetadata:getLength()
    local units = 0
    for _, code in utf8.codes(self._metadata) do
        -- Anything outside the basic plane is a surrogate pair in UTF-16.
        units = units + (code >= 0x10000 and 2 or 1)
    end
    return units
end

function PDFMetadata:finish()
    self:_closeTags()
    self._metadata = self._metadata:match("^%s*(.-)%s*$")
end

return PDFMetadata
