/*
 * Firebird UDR for JSON Value Extraction (Firebird 4)
 *
 * Provides JSON_VALUE(json_doc BLOB SUB_TYPE TEXT, json_path VARCHAR(255))
 * RETURNS BLOB SUB_TYPE TEXT via EXTERNAL NAME 'json_udfn!json_value' ENGINE UDR.
 *
 * Firebird loads plugins/udr/libjson_udfn.so for module name json_udfn.
 *
 * Path:
 *   '$'           — whole document (validity / identity)
 *   '$.key'       — object member
 *   '$.key.sub'   — nested
 *   '$.arr[0]'    — array index
 * Missing path → SQL NULL.
 */

#define FB_UDR_STATUS_TYPE ::Firebird::ThrowStatusWrapper

#include "ibase.h"
#include "firebird/UdrCppEngine.h"

#include <jansson.h>

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

using namespace Firebird;

/* Minimal AutoRelease — samples put this in UdrCppExample.h; keep self-contained. */
template <typename T>
class AutoRelease
{
public:
	explicit AutoRelease(T* ptr = nullptr) : ptr_(ptr) {}
	~AutoRelease() { if (ptr_) ptr_->release(); }
	T* operator->() const { return ptr_; }
	T* get() const { return ptr_; }
	T* operator=(T* p) {
		if (ptr_ && ptr_ != p) ptr_->release();
		ptr_ = p;
		return ptr_;
	}
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

/* Read TEXT blob into a NUL-terminated buffer (caller free()). */
static char* read_blob_text(ThrowStatusWrapper* status, IExternalContext* context,
                            const ISC_QUAD& blobId, size_t* outLen)
{
	*outLen = 0;

	IAttachment* attachment = context->getAttachment(status);
	if (!attachment)
		raise_msg(status, "json_value: no attachment");

	AutoRelease<ITransaction> transaction(context->getTransaction(status));
	AutoRelease<IBlob> blob(attachment->openBlob(status, transaction, const_cast<ISC_QUAD*>(&blobId), 0, nullptr));

	std::vector<char> buf;
	buf.reserve(4096);
	char segment[65535];

	for (;;)
	{
		unsigned segLen = 0;
		int rc = blob->getSegment(status, sizeof(segment), segment, &segLen);
		if (segLen > 0)
			buf.insert(buf.end(), segment, segment + segLen);
		/* IStatus::RESULT_OK = 0, RESULT_SEGMENT = 1, RESULT_NO_DATA = 100 */
		if (rc == IStatus::RESULT_NO_DATA)
			break;
		if (rc != IStatus::RESULT_OK && rc != IStatus::RESULT_SEGMENT)
			raise_msg(status, "json_value: blob read failed");
		if (segLen == 0 && rc == IStatus::RESULT_OK)
			break;
	}

	blob->close(status);

	char* out = (char*)malloc(buf.size() + 1);
	if (!out)
		raise_msg(status, "json_value: out of memory reading blob");
	if (!buf.empty())
		memcpy(out, buf.data(), buf.size());
	out[buf.size()] = '\0';
	*outLen = buf.size();
	return out;
}

static ISC_QUAD write_blob_text(ThrowStatusWrapper* status, IExternalContext* context,
                                const char* data, size_t length)
{
	ISC_QUAD blobId;
	memset(&blobId, 0, sizeof(blobId));

	IAttachment* attachment = context->getAttachment(status);
	if (!attachment)
		raise_msg(status, "json_value: no attachment for write");

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

/*
 * Navigate jansson value by SQL/JSON-ish path.
 * Returns borrowed pointer into the tree, or nullptr if missing.
 */
static json_t* navigate_path(json_t* root, const char* pathBuf)
{
	if (!root || !pathBuf)
		return nullptr;

	const char* p = pathBuf;
	if (p[0] != '$')
		return nullptr;
	++p;
	if (*p == '\0')
		return root; /* bare '$' → whole document */
	if (*p == '.')
		++p;
	else if (*p != '[')
		return nullptr;

	json_t* current = root;
	while (current && *p)
	{
		if (*p == '[')
		{
			++p;
			char* endptr = nullptr;
			long idx = strtol(p, &endptr, 10);
			if (endptr == p || *endptr != ']')
				return nullptr;
			p = endptr + 1;
			if (!json_is_array(current))
				return nullptr;
			current = json_array_get(current, (size_t)idx);
			if (*p == '.')
				++p;
			continue;
		}

		char keyBuf[256];
		size_t keyLen = 0;
		while (*p && *p != '.' && *p != '[')
		{
			if (keyLen + 1 < sizeof(keyBuf))
				keyBuf[keyLen++] = *p;
			++p;
		}
		keyBuf[keyLen] = '\0';
		if (keyLen == 0)
			return nullptr;

		if (json_is_object(current))
		{
			current = json_object_get(current, keyBuf);
		}
		else if (json_is_array(current))
		{
			char* endptr = nullptr;
			long idx = strtol(keyBuf, &endptr, 10);
			if (*endptr != '\0')
				return nullptr;
			current = json_array_get(current, (size_t)idx);
		}
		else
		{
			return nullptr;
		}

		if (*p == '.')
			++p;
	}
	return current;
}

FB_UDR_BEGIN_FUNCTION(json_value)
	FB_UDR_MESSAGE(InMessage,
		(FB_BLOB, json_document)
		(FB_VARCHAR(255), json_path)
	);

	FB_UDR_MESSAGE(OutMessage,
		(FB_BLOB, result)
	);

	FB_UDR_EXECUTE_FUNCTION
	{
		if (in->json_documentNull || in->json_pathNull)
		{
			out->resultNull = FB_TRUE;
			return;
		}

		size_t jsonLen = 0;
		char* jsonStr = read_blob_text(status, context, in->json_document, &jsonLen);

		char pathBuf[256];
		unsigned pathLen = in->json_path.length;
		if (pathLen >= sizeof(pathBuf))
			pathLen = sizeof(pathBuf) - 1;
		memcpy(pathBuf, in->json_path.str, pathLen);
		pathBuf[pathLen] = '\0';

		json_error_t jerr;
		json_t* root = json_loads(jsonStr, JSON_DECODE_ANY, &jerr);
		free(jsonStr);

		if (!root)
		{
			/* Invalid JSON → NULL (matches DEFAULT NULL ON ERROR semantics for callers). */
			out->resultNull = FB_TRUE;
			return;
		}

		json_t* current = navigate_path(root, pathBuf);
		if (!current)
		{
			json_decref(root);
			out->resultNull = FB_TRUE;
			return;
		}

		char* resultStr = nullptr;
		if (json_is_string(current))
		{
			/* Return the raw string content (not JSON-quoted), common for JSON_VALUE. */
			const char* s = json_string_value(current);
			resultStr = strdup(s ? s : "");
		}
		else if (json_is_null(current))
		{
			json_decref(root);
			out->resultNull = FB_TRUE;
			return;
		}
		else
		{
			resultStr = json_dumps(current, JSON_COMPACT | JSON_ENCODE_ANY);
		}
		json_decref(root);

		if (!resultStr)
			raise_msg(status, "json_value: failed to serialize result");

		size_t resultLen = strlen(resultStr);
		out->result = write_blob_text(status, context, resultStr, resultLen);
		free(resultStr);
		out->resultNull = FB_FALSE;
	}
FB_UDR_END_FUNCTION

FB_UDR_IMPLEMENT_ENTRY_POINT
