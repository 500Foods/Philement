-- mixins/metadata.lua -- port of pdfkit/lib/mixins/metadata.js
--
-- Builds the XMP packet from the info dictionary, then attaches it -- but only
-- for PDF 1.4 and up. At 1.3 the packet is assembled and thrown away, on the
-- grounds that a reader of that vintage would ignore it anyway. That branch is
-- the reason a default document creates no object here, and therefore the
-- reason the object numbers that follow are what they are.
--
-- Every string below is interpolated straight into XML without escaping, which
-- is what the original does. A title containing "&" produces malformed XML in
-- both implementations; matching that is the point.

local object = require("pdfkit.object")
local PDFMetadata = require("pdfkit.metadata")

local M = {}

-- Date.prototype.toISOString(), cut at the seconds and re-terminated with Z.
local function iso_date(date)
    local d = os.date("!*t", date.value)
    return string.format(
        "%04d-%02d-%02dT%02d:%02d:%02dZ",
        d.year, d.month, d.day, d.hour, d.min, d.sec
    )
end

-- Info values reach the XML as text. A PDF literal string carries its contents
-- in .value; a plain Lua string is already the text.
local function text_of(v)
    if type(v) == "table" and getmetatable(v) == object.STR then
        return v.value
    end
    return tostring(v)
end

function M.initMetadata(doc)
    doc.metadata = PDFMetadata.new()
end

function M.appendXML(doc, xml, newline)
    doc.metadata:append(xml, newline)
end

function M._addInfo(doc)
    local info = doc.info

    doc:appendXML(table.concat({
        "\n",
        '        <rdf:Description rdf:about="" xmlns:xmp="http://ns.adobe.com/xap/1.0/">\n',
        "            <xmp:CreateDate>", iso_date(info.CreationDate), "</xmp:CreateDate>\n",
        "            <xmp:CreatorTool>", text_of(info.Creator), "</xmp:CreatorTool>\n",
        "        </rdf:Description>\n",
        "        ",
    }))

    if info.Title or info.Author or info.Subject then
        doc:appendXML(table.concat({
            "\n",
            '            <rdf:Description rdf:about="" xmlns:dc="http://purl.org/dc/elements/1.1/">\n',
            "            ",
        }))

        if info.Title then
            doc:appendXML(table.concat({
                "\n",
                "                <dc:title>\n",
                "                    <rdf:Alt>\n",
                '                        <rdf:li xml:lang="x-default">', text_of(info.Title), "</rdf:li>\n",
                "                    </rdf:Alt>\n",
                "                </dc:title>\n",
                "                ",
            }))
        end

        if info.Author then
            doc:appendXML(table.concat({
                "\n",
                "                <dc:creator>\n",
                "                    <rdf:Seq>\n",
                "                        <rdf:li>", text_of(info.Author), "</rdf:li>\n",
                "                    </rdf:Seq>\n",
                "                </dc:creator>\n",
                "                ",
            }))
        end

        if info.Subject then
            doc:appendXML(table.concat({
                "\n",
                "                <dc:description>\n",
                "                    <rdf:Alt>\n",
                '                        <rdf:li xml:lang="x-default">', text_of(info.Subject), "</rdf:li>\n",
                "                    </rdf:Alt>\n",
                "                </dc:description>\n",
                "                ",
            }))
        end

        doc:appendXML(table.concat({
            "\n",
            "            </rdf:Description>\n",
            "            ",
        }))
    end

    -- No trailing newline on this one and the next: the Producer element and
    -- the optional Keywords element run together on purpose.
    doc:appendXML(table.concat({
        "\n",
        '        <rdf:Description rdf:about="" xmlns:pdf="http://ns.adobe.com/pdf/1.3/">\n',
        "            <pdf:Producer>", text_of(info.Producer), "</pdf:Producer>",
    }), false)

    if info.Keywords then
        doc:appendXML(table.concat({
            "\n",
            "            <pdf:Keywords>", text_of(info.Keywords), "</pdf:Keywords>",
        }), false)
    end

    doc:appendXML(table.concat({
        "\n",
        "        </rdf:Description>\n",
        "        ",
    }))
end

function M.endMetadata(doc)
    doc:_addInfo()
    doc.metadata:finish()

    -- Metadata arrived in PDF 1.4. Adding it to a 1.3 file would only make the
    -- file bigger for no reader's benefit, so it is skipped -- and with it, the
    -- object that would otherwise have been created here.
    if doc.version ~= 1.3 then
        doc.metadataRef = doc:ref(object.dict({
            -- Lower-case "length" is not a typo on this side: the original
            -- writes it that way, so the key in the file is /length.
            { "length",  doc.metadata:getLength() },
            { "Type",    "Metadata" },
            { "Subtype", "XML" },
        }))
        doc.metadataRef.compress = false
        -- raw: the packet is written as-is, with no newline appended
        doc.metadataRef:write(doc.metadata:getXML(), true)
        doc.metadataRef:finish()
        doc._root.data.Metadata = doc.metadataRef
    end
end

return M
