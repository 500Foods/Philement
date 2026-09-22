/*
 * Firebird UDR for Brotli Decompression
 *
 * This UDR decompresses Brotli-compressed data for use in the Helium
 * migration system. It is loaded into Firebird's UDR engine and
 * registered via the firebird_udr_plugin entry point.
 *
 * Build: See Makefile
 * Install: make install  (copies .so to Firebird UDR plugin directory)
 * Register in SQL:
 *   CREATE FUNCTION BROTLI_DECOMPRESS(compressed BLOB)
 *   RETURNS BLOB
 *   EXTERNAL NAME 'brotli_decfn!brotli_decompress'
 *   ENGINE UDR;
 *
 * External name format: '<module>!<routine>[!<misc>]'
 *   module  = shared library basename (without .so), e.g. "brotli_decfn"
 *   routine = registered function name,              e.g. "brotli_decompress"
 *
 * The macro ${BROTLI_DECOMPRESS_FUNCTION} in database_firebird.lua
 * emits the CREATE FUNCTION DDL referencing this UDR.
 */

#define FB_UDR_STATUS_TYPE ::Firebird::ThrowStatusWrapper

#include "ibase.h"
#include "firebird/UdrCppEngine.h"

#include <brotli/decode.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>

using namespace Firebird;

// ---------------------------------------------------------------------------
// Helper: read an entire BLOB into a malloc'd buffer.
// Returns the buffer (caller frees) and sets *outLen.
// Throws FbException on error or allocation failure.
// ---------------------------------------------------------------------------
static uint8_t* read_blob(ThrowStatusWrapper* status, IExternalContext* context,
                          const ISC_QUAD& blobId, size_t* outLen)
{
    *outLen = 0;

    IAttachment* attachment = context->getAttachment(status);
    if (!attachment)
        throw FbException(status, "Cannot get attachment for BLOB read");

    AutoRelease<ITransaction> transaction(context->getTransaction(status));
    AutoRelease<IBlob> blob(attachment->getBlob(status, transaction, blobId));

    // Get total size
    unsigned length = blob->getLength(status);
    *outLen = length;

    if (length == 0)
    {
        transaction->release();
        attachment->release();
        uint8_t* empty = (uint8_t*)malloc(1);
        if (!empty)
            throw FbException(status, "Memory allocation failed");
        empty[0] = '\0';
        return empty;
    }

    uint8_t* buffer = (uint8_t*)malloc(length + 1);
    if (!buffer)
    {
        transaction->release();
        attachment->release();
        throw FbException(status, "Memory allocation failed for BLOB read");
    }

    unsigned offset = 0;
    unsigned actual = 0;
    blob->read(status, length, buffer, &actual);
    offset += actual;

    // If blob didn't return everything in one call, loop
    while (offset < length)
    {
        actual = 0;
        blob->read(status, length - offset, buffer + offset, &actual);
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
// Helper: write a buffer to a BLOB, returning the ISC_QUAD identifier.
// ---------------------------------------------------------------------------
static ISC_QUAD write_blob(ThrowStatusWrapper* status, IExternalContext* context,
                           const uint8_t* data, size_t length)
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
        blob->write(status, (unsigned)length, data, &written);
        // If partial write, loop (same pattern as read)
        unsigned offset = written;
        while (offset < length)
        {
            written = 0;
            blob->write(status, (unsigned)(length - offset), data + offset, &written);
            offset += written;
            if (written == 0)
                break;
        }
    }

    blob->close(status);        // close and save the blob
    transaction->release();
    attachment->release();

    return blobId;
}

// ---------------------------------------------------------------------------
// brotli_decompress  —  scalar UDR function
//
//   SQL:  CREATE FUNCTION BROTLI_DECOMPRESS(compressed BLOB)
//         RETURNS BLOB
//         EXTERNAL NAME 'brotli_decfn!brotli_decompress'
//         ENGINE UDR;
//
// Input  : BLOB  — brotli-compressed binary data
// Output : BLOB  — decompressed text
// ---------------------------------------------------------------------------

FB_UDR_BEGIN_FUNCTION(brotli_decompress)
    FB_UDR_MESSAGE(InMessage,
        (FB_BLOB, compressed)
    );

    FB_UDR_MESSAGE(OutMessage,
        (FB_BLOB, result)
    );

    FB_UDR_CONSTRUCTOR
    {
        AutoRelease<IMessageMetadata> inMetadata(metadata->getInputMetadata(status));
        compressedOffset = inMetadata->getOffset(status, 0);
        compressedNullOffset = inMetadata->getNullOffset(status, 0);
    }

    FB_UDR_EXECUTE_FUNCTION
    {
        // Handle NULL input: return NULL
        if (*(ISC_SHORT*)(in + compressedNullOffset))
        {
            *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_TRUE;
            return;
        }

        size_t compressedSize = 0;
        uint8_t* compressedData = read_blob(status, context,
                                            *(const ISC_QUAD*)(in + compressedOffset),
                                            &compressedSize);

        if (compressedSize == 0)
        {
            free(compressedData);
            *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_FALSE;
            ISC_QUAD emptyBlob;
            memset(&emptyBlob, 0, sizeof(emptyBlob));
            *(ISC_QUAD*)(out + outMetadata->getOffset(status, 0)) = emptyBlob;
            return;
        }

        // Create Brotli decoder
        BrotliDecoderState* decoder = BrotliDecoderCreateInstance(NULL, NULL, NULL);
        if (!decoder)
        {
            free(compressedData);
            throw FbException(status, "Failed to create Brotli decoder");
        }

        // Initial buffer size: 4x compressed size, min 256, max 1 GB
        size_t bufferSize = compressedSize * 4;
        if (bufferSize < 256) bufferSize = 256;
        if (bufferSize > 1073741824) bufferSize = 1073741824;

        uint8_t* output = (uint8_t*)malloc(bufferSize);
        if (!output)
        {
            free(compressedData);
            BrotliDecoderDestroyInstance(decoder);
            throw FbException(status, "Memory allocation failed");
        }

        size_t totalOut = 0;
        const uint8_t* nextIn = compressedData;
        size_t availableIn = compressedSize;
        uint8_t* nextOut = output;
        size_t availableOut = bufferSize;

        BrotliDecoderResult decodeResult;
        do
        {
            decodeResult = BrotliDecoderDecompressStream(
                decoder,
                &availableIn, &nextIn,
                &availableOut, &nextOut,
                &totalOut
            );

            if (decodeResult == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
            {
                size_t currentPos = (size_t)(nextOut - output);
                size_t newSize = bufferSize * 2;
                if (newSize > 1073741824) newSize = 1073741824;

                if (newSize == bufferSize)
                {
                    free(output);
                    free(compressedData);
                    BrotliDecoderDestroyInstance(decoder);
                    throw FbException(status, "Decompressed data exceeds 1 GB limit");
                }

                uint8_t* newOutput = (uint8_t*)realloc(output, newSize);
                if (!newOutput)
                {
                    free(output);
                    free(compressedData);
                    BrotliDecoderDestroyInstance(decoder);
                    throw FbException(status, "Memory reallocation failed");
                }

                output = newOutput;
                bufferSize = newSize;
                nextOut = output + currentPos;
                availableOut = bufferSize - currentPos;
            }
            else if (decodeResult == BROTLI_DECODER_RESULT_ERROR)
            {
                BrotliDecoderErrorCode errCode = BrotliDecoderGetErrorCode(decoder);
                const char* errStr = BrotliDecoderErrorString(errCode);
                free(output);
                free(compressedData);
                BrotliDecoderDestroyInstance(decoder);
                throw FbException(status, errStr);
            }
            else if (decodeResult == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
            {
                free(output);
                free(compressedData);
                BrotliDecoderDestroyInstance(decoder);
                throw FbException(status, "Incomplete compressed data");
            }
        } while (decodeResult != BROTLI_DECODER_RESULT_SUCCESS);

        BrotliDecoderDestroyInstance(decoder);
        free(compressedData);

        // Write decompressed data to output BLOB
        ISC_QUAD outBlobId = write_blob(status, context, output, totalOut);
        free(output);

        *(ISC_SHORT*)(out + outMetadata->getNullOffset(status, 0)) = FB_FALSE;
        *(ISC_QUAD*)(out + outMetadata->getOffset(status, 0)) = outBlobId;
    }

    unsigned compressedOffset;
    unsigned compressedNullOffset;

FB_UDR_END_FUNCTION

// ---------------------------------------------------------------------------
// Entry point — registers functions with the Firebird UDR engine.
// This must be the last thing in the file.
// ---------------------------------------------------------------------------
FB_UDR_IMPLEMENT_ENTRY_POINT
