/*
 * Firebird UDR for Brotli Decompression (Firebird 4)
 *
 * EXTERNAL NAME 'brotli_decfn!brotli_decompress' → plugins/udr/libbrotli_decfn.so
 */

#define FB_UDR_STATUS_TYPE ::Firebird::ThrowStatusWrapper

#include "ibase.h"
#include "firebird/UdrCppEngine.h"

#include <brotli/decode.h>

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <vector>

using namespace Firebird;

template <typename T>
class AutoRelease
{
public:
	explicit AutoRelease(T* ptr = nullptr) : ptr_(ptr) {}
	~AutoRelease() { if (ptr_) ptr_->release(); }
	T* operator->() const { return ptr_; }
	operator T*() const { return ptr_; }
private:
	T* ptr_;
	AutoRelease(const AutoRelease&);
	AutoRelease& operator=(const AutoRelease&);
};

static void raise_msg(ThrowStatusWrapper* status, const char* msg)
{
	ISC_STATUS_ARRAY vector = {
		isc_arg_gds, isc_random,
		isc_arg_string, (ISC_STATUS)(intptr_t)msg,
		isc_arg_end
	};
	throw FbException(status, vector);
}

static uint8_t* read_blob(ThrowStatusWrapper* status, IExternalContext* context,
                          const ISC_QUAD& blobId, size_t* outLen)
{
	*outLen = 0;
	IAttachment* attachment = context->getAttachment(status);
	if (!attachment)
		raise_msg(status, "brotli_decompress: no attachment");

	AutoRelease<ITransaction> transaction(context->getTransaction(status));
	AutoRelease<IBlob> blob(attachment->openBlob(status, transaction,
	                                             const_cast<ISC_QUAD*>(&blobId), 0, nullptr));

	std::vector<uint8_t> buf;
	buf.reserve(4096);
	uint8_t segment[65535];

	for (;;)
	{
		unsigned segLen = 0;
		int rc = blob->getSegment(status, sizeof(segment), segment, &segLen);
		if (segLen > 0)
			buf.insert(buf.end(), segment, segment + segLen);
		if (rc == IStatus::RESULT_NO_DATA)
			break;
		if (rc != IStatus::RESULT_OK && rc != IStatus::RESULT_SEGMENT)
			raise_msg(status, "brotli_decompress: blob read failed");
		if (segLen == 0 && rc == IStatus::RESULT_OK)
			break;
	}
	blob->close(status);

	uint8_t* out = (uint8_t*)malloc(buf.size() + 1);
	if (!out)
		raise_msg(status, "brotli_decompress: out of memory reading blob");
	if (!buf.empty())
		memcpy(out, buf.data(), buf.size());
	out[buf.size()] = '\0';
	*outLen = buf.size();
	return out;
}

static ISC_QUAD write_blob(ThrowStatusWrapper* status, IExternalContext* context,
                           const uint8_t* data, size_t length)
{
	ISC_QUAD blobId;
	memset(&blobId, 0, sizeof(blobId));

	IAttachment* attachment = context->getAttachment(status);
	if (!attachment)
		raise_msg(status, "brotli_decompress: no attachment for write");

	AutoRelease<ITransaction> transaction(context->getTransaction(status));
	AutoRelease<IBlob> blob(attachment->createBlob(status, transaction, &blobId, 0, nullptr));

	const size_t maxSeg = 65535;
	size_t offset = 0;
	while (offset < length)
	{
		size_t chunk = length - offset;
		if (chunk > maxSeg)
			chunk = maxSeg;
		blob->putSegment(status, (unsigned)chunk, data + offset);
		offset += chunk;
	}
	blob->close(status);
	return blobId;
}

FB_UDR_BEGIN_FUNCTION(brotli_decompress)
	FB_UDR_MESSAGE(InMessage,
		(FB_BLOB, compressed)
	);

	FB_UDR_MESSAGE(OutMessage,
		(FB_BLOB, result)
	);

	FB_UDR_EXECUTE_FUNCTION
	{
		if (in->compressedNull)
		{
			out->resultNull = FB_TRUE;
			return;
		}

		size_t compressedSize = 0;
		uint8_t* compressedData = read_blob(status, context, in->compressed, &compressedSize);

		if (compressedSize == 0)
		{
			free(compressedData);
			out->result = write_blob(status, context, nullptr, 0);
			out->resultNull = FB_FALSE;
			return;
		}

		BrotliDecoderState* decoder = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
		if (!decoder)
		{
			free(compressedData);
			raise_msg(status, "brotli_decompress: failed to create decoder");
		}

		size_t bufferSize = compressedSize * 4;
		if (bufferSize < 256)
			bufferSize = 256;
		if (bufferSize > 1073741824u)
			bufferSize = 1073741824u;

		uint8_t* output = (uint8_t*)malloc(bufferSize);
		if (!output)
		{
			free(compressedData);
			BrotliDecoderDestroyInstance(decoder);
			raise_msg(status, "brotli_decompress: out of memory");
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
				&totalOut);

			if (decodeResult == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
			{
				size_t currentPos = (size_t)(nextOut - output);
				size_t newSize = bufferSize * 2;
				if (newSize > 1073741824u)
					newSize = 1073741824u;
				if (newSize == bufferSize)
				{
					free(output);
					free(compressedData);
					BrotliDecoderDestroyInstance(decoder);
					raise_msg(status, "brotli_decompress: output exceeds 1 GB");
				}
				uint8_t* newOutput = (uint8_t*)realloc(output, newSize);
				if (!newOutput)
				{
					free(output);
					free(compressedData);
					BrotliDecoderDestroyInstance(decoder);
					raise_msg(status, "brotli_decompress: realloc failed");
				}
				output = newOutput;
				bufferSize = newSize;
				nextOut = output + currentPos;
				availableOut = bufferSize - currentPos;
			}
			else if (decodeResult == BROTLI_DECODER_RESULT_ERROR)
			{
				const char* errStr = BrotliDecoderErrorString(BrotliDecoderGetErrorCode(decoder));
				free(output);
				free(compressedData);
				BrotliDecoderDestroyInstance(decoder);
				raise_msg(status, errStr ? errStr : "brotli_decompress: decode error");
			}
			else if (decodeResult == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
			{
				free(output);
				free(compressedData);
				BrotliDecoderDestroyInstance(decoder);
				raise_msg(status, "brotli_decompress: incomplete compressed data");
			}
		} while (decodeResult != BROTLI_DECODER_RESULT_SUCCESS);

		BrotliDecoderDestroyInstance(decoder);
		free(compressedData);

		out->result = write_blob(status, context, output, totalOut);
		free(output);
		out->resultNull = FB_FALSE;
	}
FB_UDR_END_FUNCTION

FB_UDR_IMPLEMENT_ENTRY_POINT
