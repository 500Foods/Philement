-- structure_annotation.lua -- port of pdfkit/lib/structure_annotation.js
--
-- A wrapper around an annotation reference whose only job is to be a distinct
-- type. A structure element treats an annotation child differently from a
-- content child -- it becomes an /OBJR entry rather than an MCID -- and this
-- wrapper is how the two are told apart.

local PDFAnnotationReference = {}
PDFAnnotationReference.__index = PDFAnnotationReference

function PDFAnnotationReference.new(annotationRef)
    local self = setmetatable({}, PDFAnnotationReference)
    self.annotationRef = annotationRef
    return self
end

function PDFAnnotationReference.is(v)
    return type(v) == "table" and getmetatable(v) == PDFAnnotationReference
end

return PDFAnnotationReference
