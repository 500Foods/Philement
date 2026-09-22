/*
 * Firebird UDR for JSON Value Extraction
 *
 * Provides JSON_VALUE(sql_string, json_path) as a Firebird UDR function,
 * since Firebird 4 does not have native JSON_VALUE (Firebird 6+).
 *
 * This UDR uses the jansson library for JSON parsing, matching the approach
 * used by the other engine extras (DB2 JSON UDF, etc.).
 *
 * Build: See Makefile
 * Install: make install  (copies .so to Firebird UDR plugin directory)
 * Register in SQL:
 *   CREATE FUNCTION JSON_VALUE(json_document BLOB SUB_TYPE TEXT, json_path VARCHAR(255))
 *   RETURNS BLOB SUB_TYPE TEXT
 *   EXTERNAL NAME 'json_udfn!json_value'
 *   ENGINE UDR;
 *
 * The macro ${JRS}/${JRE} in database_firebird.lua expands to:
 *   JSON_VALUE(col, '$.path' DEFAULT NULL ON ERROR)
 * which calls this UDR. Missing paths return NULL.
 *
 * The json_ingest PSQL function (also defined in database_firebird.lua)
 * calls JSON_VALUE internally for validity checking — so the JSON_VALUE
 * UDR must be created before json_ingest in migration 1000.
 */

#define FB_UDR_STATUS_TYPE ::Firebird::ThrowStatusWrapper

#include "ibase.h"
#include "firebird/UdrCppEngine.h"

#include <jansson.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <string>

using namespace Firebird;

// ---------------------------------------------------------------------------
// Helper: read an entire BLOB into a malloc'd null-terminated buffer.
// Returns the buffer (caller frees) and sets *outLen.
// Throws FbException on error.
// ---------------------------------------------------------------------------
static char* read_blob_text(ThrowStatusWrapper* status, IExternalContext* context,
                            const ISC_QUAD& blobId, size_t* outLen)
{
    *outLen = 0;

    IAttachment* attachment = context->getAttachment(status);
    if (!attachment)
        throw FbException(status, "Cannot get attachment for BLOB read");

    AutoRelease<ITransaction> transaction(context->getTransaction(status));
    AutoRelease<IBlob> blob(attachment->getBlob(status, transaction, blobId));

    unsigned length = blob->getLength(status);
    *outLen = length;

    char* buffer = (char*)malloc(length + 1);
    if (!buffer)
    {
        transaction->release();
        attachment->release();
        throw FbException(status, "Memory allocation failed for BLOB read");
    }

    unsigned offset = 0;
    unsigned actual = 0;
    blob->read(status, length, (uint8_t*)buffer, &actual);
    offset += actual;

    while (offset < length)
    {
        actual = 0;
        blob->read(status, length - offset, (uint8_t*)(buffer + offset), &actual);
        offset += actual;
        if (actual == 0)
            break;
    }

    buffer[length] = '\0';
    transaction->release();
    attachment->release();
    return buffer;
}

// ---------------------------------------------------------------------------
// Helper: write text to a BLOB, returning the ISC_QUAD identifier.
// ---------------------------------------------------------------------------
static ISC_QUAD write_blob_text(ThrowStatusWrapper* status, IExternalContext* context,
                                const char* data, size_t length)
{
    ISC_QUAD blobId;
    memset(&blobId, 0, sizeof(blobId));

    IAttachment* attachment = context->getAttachment(status);
    if (!attachment)
        throw FbException(status, "Cannot get attachment for BLOB write");

    AutoRelease<ITransaction> transaction(context->getTransaction(status));
    AutoRelease<IBlob> blob(attachment->createBlob(status, transaction, &blobId));

    if (length > 0 && data)
    {
        unsigned written = 0;
        blob->write(status, (unsigned)length, (const uint8_t*)data, &written);
        unsigned offset = written;
        while (offset < length)
        {
            written = 0;
            blob->write(status, (unsigned)(length - offset),
                        (const uint8_t*)(data + offset), &written);
            offset += written;
            if (written == 0)
                break;
        }
    }

    blob->close(status);
    transaction->release();
    attachment->release();
    return blobId;
}

// ---------------------------------------------------------------------------
// json_value  —  scalar UDR function
//
//   SQL:  CREATE FUNCTION JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, path VARCHAR)
//         RETURNS BLOB SUB_TYPE TEXT
//         EXTERNAL NAME 'json_udfn!json_value'
//         ENGINE UDR;
//
// Extracts a value from a JSON document at the given path.
// Missing path → returns NULL.
//
// Path syntax supports simple dot notation:  '$.key'  '$.key.subkey'
//   The leading "$. " is required (SQL/JSON compatible).
// ---------------------------------------------------------------------------

FB_UDR_BEGIN_FUNCTION(json_value)
    FB_UDR_MESSAGE(InMessage,
        (FB_BLOB, json_document)
        (FB_VARCHAR(255), json_path)
    );

    FB_UDR_MESSAGE(OutMessage,
        (FB_BLOB, result)
    );

    FB_UDR_CONSTRUCTOR
    {
        AutoRelease<IMessageMetadata> inMetadata(metadata->getInputMetadata(status));
        jsonDocOffset = inMetadata->getOffset(status, 0);
        jsonDocNullOffset = inMetadata->getNullOffset(status, 0);
        jsonPathOffset = inMetadata->getOffset(status, 1);
        jsonPathNullOffset = inMetadata->getNullOffset(status, 1);
    }

    FB_UDR_EXECUTE_FUNCTION
    {
        // Handle NULL input: return NULL
        if (*(ISC_SHORT*)(in + jsonDocNullOffset) || *(ISC_SHORT*)(in + jsonPathNullOffset))
        {
            *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_TRUE;
            return;
        }

        // Read JSON document BLOB
        size_t jsonLen = 0;
        char* jsonStr = read_blob_text(status, context,
                                        *(const ISC_QUAD*)(in + jsonDocOffset),
                                        &jsonLen);

        // Read JSON path — it's in the second field of InMessage
        const char* pathStr = ((const FB_VARCHAR(255)*)(in + jsonPathOffset))->str;
        unsigned pathLen = ((const FB_VARCHAR(255)*)(in + jsonPathOffset))->length;

        // Build null-terminated path string
        char pathBuf[256];
        if (pathLen >= sizeof(pathBuf)) pathLen = sizeof(pathBuf) - 1;
        memcpy(pathBuf, pathStr, pathLen);
        pathBuf[pathLen] = '\0';

        // Parse JSON document with jansson
        json_t* root = json_loads(jsonStr, JSON_DISABLE_EI | JSON_DECODE_ANY, NULL);
        free(jsonStr);

        if (!root)
        {
            throw FbException(status, "Invalid JSON document in json_value UDR");
        }

        // Navigate the path: must start with "$."
        // Supported: $.key, $.key.subkey, $.key[N], etc.
        const char* p = pathBuf;
        if (p[0] != '$' || p[1] != '.')
        {
            json_decref(root);
            throw FbException(status, "JSON path must start with '$.' in json_value UDR");
        }
        p += 2; // skip "$."

        json_t* current = root;
        char keyBuf[128];
        size_t keyLen = 0;

        while (current && *p)
        {
            // Collect key characters until '.' or '[' or '\0'
            keyLen = 0;
            while (*p && *p != '.' && *p != '[')
            {
                if (keyLen < sizeof(keyBuf) - 1)
                {
                    keyBuf[keyLen++] = *p;
                    p++;
                }
                else
                {
                    p++;
                }
            }
            keyBuf[keyLen] = '\0';

            if (keyLen > 0)
            {
                // Try object lookup first, then array index
                json_t* next = json_object_get(current, keyBuf);
                if (!next)
                {
                    // Try as array index
                    char* endptr = NULL;
                    long idx = strtol(keyBuf, &endptr, 10);
                    if (*endptr == '\0' && next == NULL)
                    {
                        next = json_array_get(current, (size_t)idx);
                    }
                }

                if (!next)
                {
                    // Missing path → NULL result
                    json_decref(root);
                    free(jsonStr);
                    *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_TRUE;
                    return;
                }
                current = next;
            }

            // Handle array index: [N]
            if (*p == '[')
            {
                p++; // skip '['
                char idxBuf[32];
                size_t idxLen = 0;
                while (*p && *p != ']' && idxLen < sizeof(idxBuf) - 1)
                {
                    idxBuf[idxLen++] = *p;
                    p++;
                }
                idxBuf[idxLen] = '\0';
                if (*p == ']') p++; // skip ']'

                char* endptr = NULL;
                long idx = strtol(idxBuf, &endptr, 10);
                if (*endptr == '\0')
                {
                    json_t* next = json_array_get(current, (size_t)idx);
                    if (!next)
                    {
                        json_decref(root);
                        *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_TRUE;
                        return;
                    }
                    current = next;
                }
            }

            // Skip the dot separator
            if (*p == '.') p++;
        }

        // Serialize the result value to JSON text
        char* resultStr = json_dumps(current, JSON_COMPACT);
        json_decref(root);

        if (!resultStr)
        {
            throw FbException(status, "Failed to serialize JSON value in json_value UDR");
        }

        size_t resultLen = strlen(resultStr);

        ISC_QUAD outBlobId = write_blob_text(status, context, resultStr, resultLen);
        free(resultStr);

        *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_FALSE;
        *(ISC_QUAD*)(out + outMetadata->getOffset(status, 0)) = outBlobId;
    }

    unsigned jsonDocOffset;
    unsigned jsonDocNullOffset;
    unsigned jsonPathOffset;
    unsigned jsonPathNullOffset;

FB_UDR_END_FUNCTION

// ---------------------------------------------------------------------------
// Entry point — registers functions with the Firebird UDR engine.
// ---------------------------------------------------------------------------
FB_UDR_IMPLEMENT_ENTRY_POINT
