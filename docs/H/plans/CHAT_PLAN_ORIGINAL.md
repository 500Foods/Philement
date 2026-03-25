# Chat Service - Implementation Plan

## Overview

This document outlines the implementation of a **Chat Proxy Service** within the Conduit API. The service acts as an intermediary between REST API clients and AI chatbot engines (OpenAI, Anthropic, Grok, Ollama, Gradient AI, etc.). Clients send their conversation content to our API, we forward it to the configured chat engine, and return the AI's response.

### Key Design Principles

- **Mirrors Query API Structure**: Chat endpoints follow the same pattern as query endpoints (`chat`/`chats`, `auth_chat`/`auth_chats`, `alt_chat`/`alt_chats`)
- **JWT Authentication Required**: Like `auth_query`, the `auth_chat` endpoint requires a valid JWT token
- **Database from JWT**: The database name is extracted from JWT claims (same pattern as `auth_query`)
- **Engine Selection**: Clients specify which chat engine to use from the database's configured engines
- **Proxy Pattern**: We proxy requests to external AI APIs and return responses
- **Streaming Support**: Design should accommodate future streaming response support
- **OpenAI-First**: Most providers support OpenAI-compatible APIs; we prioritize this format

### Design Decisions

1. **Cross-Database Access**: The `alt_chat`/`alt_chats` endpoints support cross-database chat access for admin users, following the same `alt_query`/`alt_queries` pattern.

2. **Endpoint Naming Convention**: Uses underscores (`auth_chat`, `alt_chat`) to match the existing conduit endpoint naming convention (`auth_query`, `alt_query`, `auth_queries`, `alt_queries`).

3. **CEC Bootstrap via Internal QueryRef**: The Chat Engine Cache is populated at database bootstrap using an internal QueryRef (query_type 0 = `internal_sql`). This query is stored in the database like all other QueryRefs and is loaded into the QTC, but is never callable from any conduit endpoint. See [Internal Query Enforcement](#internal-query-enforcement-helium--hydrogen) for details.

### Supported AI Providers

| Provider | API Format | Base URL | Authentication |
|----------|------------|----------|----------------|
| **OpenAI** | Native OpenAI | `https://api.openai.com/v1` | Bearer token |
| **Anthropic** | Native (similar) | `https://api.anthropic.com/v1` | Bearer token + `anthropic-version` header |
| **xAI (Grok)** | OpenAI-compatible | `https://api.x.ai/v1` | Bearer token |
| **Ollama** | Native + OpenAI-compat | `http://localhost:11434` | None (local) or Bearer |
| **Gradient AI** | OpenAI-compatible | `https://inference.do-ai.run` | Bearer token (model access key) |

All providers above support the `/v1/chat/completions` endpoint with the OpenAI request/response format.

## Database Schema

### Schema vs Code Boundary

The `convos` and `lookups` tables referenced throughout this plan are **database-side schema managed by Acuranzo/Helium migrations**, not by Hydrogen C code. Hydrogen interacts with these tables exclusively through QueryRef SQL templates that are loaded into the QTC at bootstrap. The CEC bootstrap query follows this same pattern — it is a QueryRef stored in the database and executed during the bootstrap process.

### Chat Engine Data Structure

Chat engines are stored in the database as JSON objects in the `lookups` table (Lookup 038 - AI Chat Engines, Migration 1063). Each engine is a lookup entry with JSON configuration in the `collection` field:

```json
{
    "icon": "<img src='images/ai_openai.png'>",
    "name": "ChatGPT 4o",
    "limit": 32768,
    "model": "gpt-4o",
    "engine": "OpenAI",
    "api_key": "<api key>",
    "banner": {
        "default": ["Ask me anything."]
    },
    "country": "Demo",
    "default": true,
    "endpoint": "https://api.openai.com/v1/chat/completions",
    "location": "Demo",
    "authority": "Demo",
    "cost_prompt": 0,
    "organization": "<org>",
    "cost_completion": 0
}
```

### Field Descriptions

| Field | Type | Description |
|-------|------|-------------|
| `icon` | string | HTML img tag for UI display |
| `name` | string | Human-readable engine name |
| `limit` | integer | Maximum token limit for context window |
| `model` | string | Model identifier (e.g., "gpt-4o", "claude-3-opus") |
| `engine` | string | Engine provider ("OpenAI", "Anthropic", "Google", etc.) |
| `api_key` | string | API key for the engine (plaintext in DB) |
| `banner` | object | Welcome messages by locale |
| `country` | string | Geographic region |
| `default` | boolean | Whether this is the default engine for the database |
| `endpoint` | string | API endpoint URL |
| `location` | string | Data center location |
| `authority` | string | Regulatory authority |
| `cost_prompt` | number | Cost per prompt token |
| `organization` | string | Organization identifier |
| `cost_completion` | number | Cost per completion token |

### Chat Engines Storage (Existing)

Chat engines are stored in the existing **`lookups` table** under **Lookup 038 - AI Chat Engines**. Each engine is a lookup entry with JSON configuration in the `collection` field.

#### CEC Bootstrap Query (New QueryRef - `internal_sql` type)

```sql
-- New QueryRef: Get AI Chat Engines (Server-side with API keys)
-- query_type = 0 (internal_sql) - NOT callable from any conduit endpoint
SELECT
    key_idx AS engine_key,
    value_txt AS engine_name,
    collection AS engine_config
FROM lookups
WHERE lookup_id = 38
  AND status_a1 = 1
  AND (valid_after IS NULL OR valid_after <= NOW())
  AND (valid_until IS NULL OR valid_until >= NOW())
ORDER BY sort_seq;
```

**Note**: The existing QueryRef #044 (Get Filtered Lookup: AI Models) remains for clients — it strips API keys from the response. The new CEC bootstrap QueryRef is `internal_sql` (type 0) and includes API keys for server-side use only.

### Chat Conversations Table (Content-Addressable Storage)

Transform the existing **`convos` table** (Migration 1008, managed by Acuranzo) to use content-addressable storage with hash references:

```sql
-- Extend existing convos table for hash-based storage (new Helium migration)
ALTER TABLE convos ADD COLUMN IF NOT EXISTS segment_refs JSONB;
ALTER TABLE convos ADD COLUMN IF NOT EXISTS engine_name VARCHAR(100);
ALTER TABLE convos ADD COLUMN IF NOT EXISTS model VARCHAR(100);
ALTER TABLE convos ADD COLUMN IF NOT EXISTS tokens_prompt INTEGER DEFAULT 0;
ALTER TABLE convos ADD COLUMN IF NOT EXISTS tokens_completion INTEGER DEFAULT 0;
ALTER TABLE convos ADD COLUMN IF NOT EXISTS cost_total DECIMAL(10,6) DEFAULT 0;
ALTER TABLE convos ADD COLUMN IF NOT EXISTS session_id VARCHAR(50);
ALTER TABLE convos ADD COLUMN IF NOT EXISTS user_id INTEGER;
ALTER TABLE convos ADD COLUMN IF NOT EXISTS duration_ms INTEGER;

-- Index for session-based queries
CREATE INDEX idx_convos_session ON convos(session_id, created_at);
```

#### Storage Model Change

- **Old**: Store full JSON array in `history` column (duplicates data, O(n^2) growth)
- **New**: Store array of SHA-256 hashes in `segment_refs` (references content, O(n) growth)

### Conversation Segments Table (NEW)

Content-addressable storage for individual messages with Brotli compression:

```sql
-- New Helium migration
CREATE TABLE conversation_segments (
    segment_hash VARCHAR(64) PRIMARY KEY,  -- SHA-256 hash of UNCOMPRESSED content
    segment_content BLOB NOT NULL,          -- Brotli-compressed JSON message
    uncompressed_size INTEGER NOT NULL,     -- Size before compression (bytes)
    compressed_size INTEGER NOT NULL,       -- Size after compression (bytes)
    compression_ratio DECIMAL(4,2),         -- uncompressed/compressed (e.g., 3.50)
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_accessed TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    access_count INTEGER DEFAULT 1          -- For cache analytics
);

-- Indexes for efficient retrieval
CREATE INDEX idx_segments_accessed ON conversation_segments(last_accessed);
CREATE INDEX idx_segments_created ON conversation_segments(created_at);
```

#### Storage with Brotli Compression

```c
// Pseudocode for storing a segment with compression
void store_conversation_segment(const char* json_content) {
    // 1. Compute SHA-256 hash of ORIGINAL content
    char* hash = sha256_hex(json_content);

    // 2. Check if already exists (content-addressable dedup)
    if (segment_exists(hash)) {
        increment_access_count(hash);
        return;  // Already stored, don't duplicate
    }

    // 3. Compress with Brotli (using existing utils_compression functions)
    size_t uncompressed_len = strlen(json_content);
    uint8_t* compressed = brotli_compress((uint8_t*)json_content, uncompressed_len, &compressed_len);

    // 4. Calculate compression ratio
    float ratio = (float)uncompressed_len / (float)compressed_len;

    // 5. Store compressed blob
    db_execute(
        "INSERT INTO conversation_segments (segment_hash, segment_content, "
        "uncompressed_size, compressed_size, compression_ratio) "
        "VALUES (?, ?, ?, ?, ?)",
        hash, compressed, compressed_len, uncompressed_len, ratio
    );

    log_this("Stored segment %s: %zu -> %zu bytes (%.2fx compression)",
             hash, uncompressed_len, compressed_len, ratio);
}

// Pseudocode for retrieving and decompressing
char* retrieve_conversation_segment(const char* hash) {
    // 1. Fetch compressed blob from DB
    uint8_t* compressed = db_query_blob(
        "SELECT segment_content FROM conversation_segments WHERE segment_hash = ?", hash
    );

    // 2. Decompress with Brotli (using existing utils_compression functions)
    size_t compressed_len = get_compressed_size(hash);
    size_t uncompressed_len = get_uncompressed_size(hash);
    uint8_t* uncompressed = brotli_decompress(compressed, compressed_len, uncompressed_len);

    // 3. Update access metadata
    db_execute(
        "UPDATE conversation_segments SET last_accessed = NOW(), "
        "access_count = access_count + 1 WHERE segment_hash = ?",
        hash
    );

    return (char*)uncompressed;
}
```

#### Example Storage

```sql
-- A typical chat message:
-- Uncompressed: {"role":"assistant","content":"Python is a versatile programming language..."}
-- Size: 245 bytes
-- Brotli compressed: 67 bytes (3.65x compression ratio)

INSERT INTO conversation_segments
(segment_hash, segment_content, uncompressed_size, compressed_size, compression_ratio)
VALUES (
    'a3f5c8e2d4b1...',  -- SHA-256 of uncompressed JSON
    X'1b3f...',         -- Brotli-compressed bytes
    245,                -- Original size
    67,                 -- Compressed size
    3.65                -- Compression ratio
);

-- Referenced in convos table (just the hash, tiny!):
INSERT INTO convos (session_id, segment_refs, engine_name, tokens_prompt, tokens_completion)
VALUES ('sess-123', '["a3f5c8...", "b7e2d1..."]', 'gpt-4o', 10, 25);
```

#### Brotli Compression Benefits

- **3-5x compression** on typical chat messages (text-heavy JSON)
- **Faster than storage I/O**: Compression/decompression is CPU-bound, faster than disk
- **Reduces DB size**: 10GB of conversations -> ~2-3GB storage
- **Already in Hydrogen**: Brotli is fully integrated (`src/utils/utils_compression.c`, `src/webserver/web_server_compression.h`, `src/payload/payload.c`) with tiered quality levels in `src/globals.h`

### Integration with Existing Chat Queries

The existing chat queries need updates to work with hash-based storage:

#### Existing Queries (Require Updates)

| QueryRef | Current | Modification |
|----------|---------|--------------|
| #036 | Store Chat | **Update**: Instead of storing full `prompt`/`response`/`context`/`history` text columns, store hashes and insert segments into `conversation_segments` |
| #037 | Chats Missing Icon+Keywords | No change (metadata query) |
| #038 | Update Chat Icon+Keywords | No change (metadata update) |
| #039 | Get Chats List | Add segment count, total storage size per conversation |
| #040 | Get Chats List + Search | Search now queries `conversation_segments` via hash joins |
| #041 | Get Chat | **Major update**: Return reconstructed conversation from hash references + segment lookup |
| #044 | Get Filtered Lookup: AI Models | No change (engine lookup, strips API keys) |

#### New Queries Needed

**Note**: QueryRef numbers below are provisional. Actual numbers will be assigned during Helium migration creation based on what's available. The existing codebase already uses QueryRefs up through #052 for account registration.

#### New QueryRef A: Get Conversation Segments by Hash

```sql
-- Retrieve multiple compressed segments for reconstruction
SELECT segment_hash, segment_content, uncompressed_size, compression_ratio
FROM conversation_segments
WHERE segment_hash = ANY(:HASH_ARRAY);

-- Returns: Array of compressed blobs that need Brotli decompression
```

#### New QueryRef B: Store Conversation Segment

```sql
-- Insert compressed segment (called by chat service after Brotli compression)
INSERT INTO conversation_segments
(segment_hash, segment_content, uncompressed_size, compressed_size, compression_ratio)
VALUES (:HASH, :COMPRESSED_BLOB, :UNCOMPRESSED_SIZE, :COMPRESSED_SIZE, :RATIO)
ON CONFLICT (segment_hash) DO UPDATE SET
    last_accessed = NOW(),
    access_count = conversation_segments.access_count + 1;
```

#### New QueryRef C: Store Chat with Hashes

```sql
-- Updated version of #036 for hash-based storage
-- Stores only hash references in convos table
INSERT INTO convos (
    convos_id,
    convos_ref,
    segment_refs,        -- JSON array of SHA-256 hashes instead of full text
    engine_name,
    model,
    tokens_prompt,
    tokens_completion,
    cost_total,
    session_id,
    user_id,
    duration_ms,
    created_id,
    created_at
)
VALUES (
    next_convos_id,
    :CONVOREF,
    :SEGMENT_REFS,       -- e.g., '["a3f5...","b7e2..."]'
    :ENGINE_NAME,
    :MODEL,
    :TOKENS_PROMPT,
    :TOKENS_COMPLETION,
    :COST_TOTAL,
    :SESSION_ID,
    :USER_ID,
    :DURATION_MS,
    :ACCOUNTID,
    NOW()
);

-- Note: Actual segment content stored via QueryRef B
```

#### New QueryRef D: Reconstruct Conversation

```sql
-- Get all segment hashes for a conversation, then client fetches segments
SELECT
    c.convos_id,
    c.convos_ref,
    c.segment_refs,      -- Array of hashes to fetch
    c.engine_name,
    c.tokens_prompt,
    c.tokens_completion,
    c.cost_total,
    c.created_at,
    -- Calculate total uncompressed size
    (SELECT SUM(uncompressed_size)
     FROM conversation_segments
     WHERE segment_hash = ANY(c.segment_refs)) as total_size
FROM convos c
WHERE c.convos_id = :CONVOSID;

-- Client then calls QueryRef A to get actual segment content
```

#### New QueryRef E: Find Conversations by Segment Content

```sql
-- Audit query: Find all conversations containing a specific message
-- (Find by hash after computing hash of search content)
SELECT c.convos_id, c.convos_ref, c.session_id, c.created_at
FROM convos c
WHERE EXISTS (
    SELECT 1 FROM jsonb_array_elements_text(c.segment_refs) as hash
    WHERE hash = :SEGMENT_HASH
);
```

#### New QueryRef F: Get Conversation Statistics

```sql
-- Analytics query for storage metrics
SELECT
    COUNT(DISTINCT c.convos_id) as total_conversations,
    COUNT(DISTINCT s.segment_hash) as unique_segments,
    SUM(s.uncompressed_size) as total_uncompressed_bytes,
    SUM(s.compressed_size) as total_compressed_bytes,
    AVG(s.compression_ratio) as avg_compression_ratio,
    (SUM(s.uncompressed_size) - SUM(s.compressed_size)) as space_saved_bytes
FROM convos c
LEFT JOIN conversation_segments s ON s.segment_hash = ANY(
    SELECT jsonb_array_elements_text(c.segment_refs)
);
```

#### Query Migration Strategy

**Phase 1:** Create new queries (A-F) alongside existing ones in Helium migrations
**Phase 2:** Update Hydrogen application to use hash-based queries
**Phase 3:** Migrate historical data (optional background job)
**Phase 4:** Deprecate old text-column storage (mark QueryRef #036 as deprecated)

#### Backwards Compatibility

Old conversations with full `prompt`/`response` columns remain readable:

```sql
-- Hybrid read: Check if segment_refs exists, fallback to legacy columns
SELECT
    CASE
        WHEN segment_refs IS NOT NULL THEN 'HASH_BASED'
        ELSE 'LEGACY_TEXT'
    END as storage_type,
    convos_id, convos_ref, created_at
FROM convos
WHERE session_id = :SESSIONID;
```

## Internal Query Enforcement (Helium + Hydrogen)

### The Problem - Internal Query Security Gap

The CEC bootstrap query retrieves AI engine configurations **including plaintext API keys**. This query must be executable by the Hydrogen server at bootstrap time but must **never** be callable by any client through conduit endpoints.

### Current Query Type System

The existing `query_type` field in the database controls access:

| Type | Category | Loaded into QTC | Callable from Public Conduit | Callable from Auth Conduit |
|------|----------|-----------------|------------------------------|----------------------------|
| 0 | `internal_sql` | Yes | No | **Yes (gap)** |
| 1 | `system_sql` | Yes | No | **Yes (gap)** |
| 2 | `system_ddl` | Yes | No | **Yes (gap)** |
| 3 | `reporting_sql` | Yes | No | **Yes (gap)** |
| 10 | Public queries | Yes | Yes | Yes |
| 1000 | Forward migration | Yes | No | No (never matched) |
| 1001 | Reverse migration | Yes | No | No (never matched) |

**The gap**: Authenticated conduit endpoints (`auth_query`, `auth_queries`, `alt_query`, `alt_queries`) use `lookup_database_and_query()` which performs a type-agnostic lookup. An authenticated client with a valid JWT can currently call any QueryRef with type 0-3, including the CEC bootstrap query that returns API keys.

### Proposed Solution

Add enforcement in **both** Helium (database) and Hydrogen (server) to block conduit access to internal queries:

#### Hydrogen Change: Filter Internal Types in Authenticated Lookups

In `src/api/conduit/helpers/database_operations.c`, update `lookup_database_and_query()` to reject queries with `query_type` 0-3 when called from a conduit endpoint:

```c
// Updated lookup for authenticated conduit endpoints
// Allows type 10 (public) and any future conduit-safe types
// Blocks type 0-3 (internal/system) from being called via conduit
bool lookup_database_and_query(DatabaseQueue** db_queue,
                               QueryCacheEntry** cache_entry,
                               const char* database,
                               int query_ref) {
    // ... existing database lookup ...

    *cache_entry = query_cache_lookup((*db_queue)->query_cache, query_ref, SR_API);

    // NEW: Block internal queries from conduit access
    if (*cache_entry && (*cache_entry)->query_type >= 0 && (*cache_entry)->query_type <= 3) {
        log_this(0, "Blocked conduit access to internal query_ref %d (type %d)",
                 LOG_LEVEL_WARN, 2, query_ref, (*cache_entry)->query_type);
        *cache_entry = NULL;
        return false;
    }

    return (*cache_entry != NULL);
}
```

**Server-side code** (auth service, CEC bootstrap) continues to use `query_cache_lookup()` directly, bypassing this restriction.

#### Helium Change: Add Internal Query Flag (Optional Enhancement)

Optionally add an `is_internal` boolean column to the `queries` table in a future migration:

```sql
ALTER TABLE queries ADD COLUMN IF NOT EXISTS is_internal BOOLEAN DEFAULT false;
UPDATE queries SET is_internal = true WHERE query_type_a28 IN (0, 1, 2, 3);
```

This provides defense-in-depth at the database level, but the Hydrogen-side enforcement is the primary control.

### Impact on Existing Auth Queries

The auth service queries (refs 1, 4, 5, 7, 8, 12, 13, 18, 19, 50, 51) are already type 0-3 (`internal_sql`/`system_sql`). These are called server-side via `execute_auth_query()` in `auth_service_database.c`, which uses `query_cache_lookup()` directly — **not** through the conduit helper functions. The Hydrogen change above will not affect them.

However, **if any authenticated clients are currently calling type 0-3 queries through conduit endpoints**, this change would break them. An audit of client usage should be done before deploying this change. If any legitimate client use cases exist, those queries should be reclassified to type 10 (public) in Helium.

## API Endpoints

The Chat API mirrors the Query API structure with single and batch endpoints, plus public and authenticated variants.

### Public Endpoints (No Authentication)

#### 1. `POST /api/conduit/chat` - Single Chat

Executes a single chat completion. Requires explicit `database` and `engine` parameters.

**Request Format:**

**Text-only conversation:**

```json
{
    "database": "DemoDB",
    "engine": "ChatGPT 4o",
    "messages": [
        {"role": "system", "content": "You are a helpful assistant."},
        {"role": "user", "content": "What is the capital of France?"}
    ],
    "temperature": 0.7,
    "max_tokens": 500
}
```

**Multimodal conversation (images + text) - Supported from Day One:**

```json
{
    "database": "DemoDB",
    "engine": "ChatGPT 4o",
    "messages": [
        {"role": "system", "content": "You are a helpful assistant."},
        {
            "role": "user",
            "content": [
                {"type": "text", "text": "What's in this image?"},
                {"type": "image_url", "image_url": {"url": "data:image/jpeg;base64,/9j/4AAQSkZJRg..."}}
            ]
        }
    ],
    "temperature": 0.7,
    "max_tokens": 500
}
```

**Note**: `content` can be a **string** (simple text) OR an **array of parts** (multimodal). This follows the OpenAI vision API format and works with OpenAI, xAI Grok, and Ollama out of the box.

**Response Format (Success):**

```json
{
    "success": true,
    "database": "DemoDB",
    "engine": "ChatGPT 4o",
    "model": "gpt-4o",
    "message": {
        "role": "assistant",
        "content": "The capital of France is Paris."
    },
    "usage": {
        "prompt_tokens": 25,
        "completion_tokens": 10,
        "total_tokens": 35
    },
    "response_time_ms": 850
}
```

#### 2. `POST /api/conduit/chats` - Batch Chat

Executes multiple chat completions in parallel (useful for multi-turn or multi-model comparisons).

**Request Format:**

```json
{
    "database": "DemoDB",
    "chats": [
        {
            "engine": "ChatGPT 4o",
            "messages": [
                {"role": "user", "content": "What is 2+2?"}
            ]
        },
        {
            "engine": "Grok 4",
            "messages": [
                {"role": "user", "content": "What is 2+2?"}
            ]
        }
    ]
}
```

**Response Format:**

```json
{
    "success": true,
    "database": "DemoDB",
    "results": [
        {
            "success": true,
            "engine": "ChatGPT 4o",
            "message": {"role": "assistant", "content": "2+2=4"}
        },
        {
            "success": true,
            "engine": "Grok 4",
            "message": {"role": "assistant", "content": "The answer is 4."}
        }
    ],
    "total_response_time_ms": 1200
}
```

### Authenticated Endpoints (JWT Required)

#### 3. `POST /api/conduit/auth_chat` - Authenticated Single Chat

Same as `/api/conduit/chat` but database is extracted from JWT claims.

**Request Format:**

```json
{
    "engine": "ChatGPT 4o",
    "messages": [
        {"role": "user", "content": "Hello!"}
    ]
}
```

**Database**: Extracted from `Authorization: Bearer <jwt>` token claims.

#### 4. `POST /api/conduit/auth_chats` - Authenticated Batch Chat

Same as `/api/conduit/chats` but database is extracted from JWT claims.

#### 5. `POST /api/conduit/alt_chat` - Cross-Database Single Chat

Authenticated endpoint allowing database override (for admin users).

**Request Format:**

```json
{
    "database": "OtherDB",
    "engine": "ChatGPT 4o",
    "messages": [
        {"role": "user", "content": "Hello!"}
    ]
}
```

**Database**: From request body (overrides JWT claims).

#### 6. `POST /api/conduit/alt_chats` - Cross-Database Batch Chat

Batch version of `alt_chat` with database override support.

### Error Response Format

All endpoints return consistent error responses:

```json
{
    "success": false,
    "error": "Engine not found",
    "engine": "NonExistentEngine",
    "message": "The specified chat engine 'NonExistentEngine' is not configured for this database"
}
```

### HTTP Status Codes

| HTTP Code | Error | Description |
|-----------|-------|-------------|
| 200 | Success | Request completed (check `success` field for business logic errors) |
| 400 | Invalid request | Missing required fields or malformed JSON |
| 401 | Authentication required | Missing or invalid JWT token |
| 404 | Engine not found | Specified engine doesn't exist for this database |
| 429 | Rate limited | Too many requests or engine quota exceeded |
| 502 | Engine error | Error from upstream AI service |
| 504 | Engine timeout | Upstream AI service timed out |

## Implementation Structure

### Directory Layout

Following the Query API pattern:

```directory
src/api/conduit/
├── chat/                       # Public single chat
│   ├── chat.h
│   └── chat.c
├── chats/                      # Public batch chat
│   ├── chats.h
│   └── chats.c
├── auth_chat/                  # Authenticated single chat
│   ├── auth_chat.h
│   └── auth_chat.c
├── auth_chats/                 # Authenticated batch chat
│   ├── auth_chats.h
│   └── auth_chats.c
├── alt_chat/                   # Cross-db single chat (admin)
│   ├── alt_chat.h
│   └── alt_chat.c
├── alt_chats/                  # Cross-db batch chat (admin)
│   ├── alt_chats.h
│   └── alt_chats.c
├── chat_common/                # Shared chat components
│   ├── chat_proxy.h            # HTTP proxy to AI APIs
│   ├── chat_proxy.c
│   ├── chat_engine_cache.h     # Engine cache (CEC)
│   ├── chat_engine_cache.c
│   ├── chat_request_builder.h  # Build provider-specific requests
│   ├── chat_request_builder.c
│   ├── chat_response_parser.h  # Parse provider responses
│   └── chat_response_parser.c
└── ... (existing conduit files)
```

### Key Components

#### 1. Chat Engine Cache (CEC)

Similar to the Query Table Cache (QTC), engines are cached in memory per database:

```c
typedef struct {
    char* name;                    // Engine name (e.g., "ChatGPT 4o")
    char* model;                   // Model identifier (e.g., "gpt-4o")
    char* provider;                // Provider type ("openai", "anthropic", "xai", "ollama", "gradient")
    char* endpoint_url;            // API endpoint URL
    char* api_key;                 // API key (plaintext from DB)
    int context_limit;             // Maximum context window
    bool is_default;               // Is this the default engine?
    json_t* metadata;              // Provider-specific metadata
} ChatEngineConfig;

typedef struct {
    ChatEngineConfig* engines;
    size_t count;
    pthread_rwlock_t lock;
} ChatEngineCache;

// Lookup functions (mirrors QTC pattern)
ChatEngineConfig* chat_engine_cache_lookup(
    const char* database,
    const char* engine_name
);

ChatEngineConfig* chat_engine_cache_get_default(
    const char* database
);

ChatEngineList* chat_engine_cache_list_all(
    const char* database
);
```

#### 2. Chat Proxy (`chat_proxy.c`)

Uses libcurl to send requests to external AI APIs:

```c
typedef struct {
    char* response_body;
    size_t response_size;
    long http_status;
    double response_time_ms;
    char* content_type;
} ChatProxyResult;

// Main proxy function - sends request to any provider
ChatProxyResult* chat_proxy_send(
    const ChatEngineConfig* engine,
    const json_t* request_body,
    int timeout_seconds
);

// Build request URL based on provider
typedef enum {
    PROVIDER_OPENAI,
    PROVIDER_ANTHROPIC,
    PROVIDER_XAI,
    PROVIDER_OLLAMA,
    PROVIDER_GRADIENT,
    PROVIDER_CUSTOM
} ChatProviderType;

char* chat_proxy_build_url(
    const ChatEngineConfig* engine,
    ChatProviderType provider
);

// Set provider-specific headers
struct curl_slist* chat_proxy_build_headers(
    const ChatEngineConfig* engine,
    ChatProviderType provider
);

void chat_proxy_result_free(ChatProxyResult* result);
```

#### 3. Request Builder (`chat_request_builder.c`)

Builds provider-specific request payloads:

```c
// Build request for OpenAI-compatible APIs (OpenAI, xAI, Gradient, Ollama)
json_t* chat_request_build_openai_format(
    const char* model,
    const json_t* messages,
    double temperature,
    int max_tokens,
    bool stream
);

// Build request for Anthropic (native format)
json_t* chat_request_build_anthropic_format(
    const char* model,
    const json_t* messages,
    double temperature,
    int max_tokens,
    bool stream
);

// Build request for Ollama native API
json_t* chat_request_build_ollama_native(
    const char* model,
    const json_t* messages,
    double temperature,
    int max_tokens,
    bool stream
);

// Auto-detect format based on provider
json_t* chat_request_build_auto(
    const ChatEngineConfig* engine,
    const json_t* messages,
    double temperature,
    int max_tokens,
    bool stream
);
```

#### 4. Response Parser (`chat_response_parser.c`)

Parses provider responses into standard format:

```c
// Standard response structure
typedef struct {
    bool success;
    char* error_message;
    char* content;              // Assistant's message content
    char* role;                 // "assistant"
    char* model;                // Actual model used
    int prompt_tokens;
    int completion_tokens;
    int total_tokens;
    char* finish_reason;        // "stop", "length", "error", etc.
} ParsedChatResponse;

// Parse OpenAI-compatible response
ParsedChatResponse* chat_response_parse_openai_format(
    const char* response_body
);

// Parse Anthropic native response
ParsedChatResponse* chat_response_parse_anthropic_format(
    const char* response_body
);

// Parse Ollama native response
ParsedChatResponse* chat_response_parse_ollama_native(
    const char* response_body
);

// Auto-detect format based on provider
ParsedChatResponse* chat_response_parse_auto(
    const char* response_body,
    ChatProviderType provider
);

void parsed_chat_response_free(ParsedChatResponse* response);
```

## Implementation Phases

### Phase 1: Prerequisites and Infrastructure

1. **Add libcurl as a core dependency**
   - Update `cmake/CMakeLists-init.cmake` to add libcurl to `HYDROGEN_BASE_LIBS`
   - Add curl initialization (`curl_global_init`/`curl_global_cleanup`) to Hydrogen startup/shutdown
   - Add libcurl to `tests/test_14_library_dependencies.sh`
2. **Create internal CEC bootstrap QueryRef in Helium**
   - Add new migration with `query_type = 0` (`internal_sql`) for engine retrieval with API keys
   - Ensure existing QueryRef #044 (client-facing, strips API keys) remains unchanged
3. **Implement internal query enforcement in Hydrogen**
   - Update `lookup_database_and_query()` in `database_operations.c` to block `query_type` 0-3 from conduit access
   - Audit existing client usage to verify no breakage (see [Internal Query Enforcement](#internal-query-enforcement-helium--hydrogen))
4. **Add chat configuration fields to `DatabaseConnection`**
   - Update `src/config/config_databases.h` with chat-specific fields
   - Update `src/config/config_databases.c` parsing logic
   - Update `src/config/config_defaults.c` with default values

### Phase 2: Chat Engine Cache (CEC) and Proxy Foundation

1. **Create `ChatEngineConfig` and `ChatEngineCache` structures**
2. **Implement CEC bootstrap** — execute the internal QueryRef during database bootstrap (same flow as QTC)
3. **CEC refresh**: periodic (1 hour) + on-demand admin endpoint
4. **Implement `chat_proxy.c`** with libcurl HTTP POST functionality
5. **Implement `chat_request_builder.c`** for OpenAI-compatible format
6. **Implement `chat_response_parser.c`** for OpenAI-compatible responses
7. **Token pre-counting**: quick estimation (`strlen/4`) to reject over-limit requests before proxying
8. **Retry with exponential backoff**: 1-2 retries on 429/5xx errors with jitter
9. Unit tests for cache operations, proxy, builder, and parser

### Phase 3: Public Endpoints (`chat`, `chats`) with Image Support

**Core chat functionality with multimodal support from day one:**

1. Create `chat/chat.h` and `chat/chat.c` (single chat)
2. Create `chats/chats.h` and `chats/chats.c` (batch chat with parallel execution via `curl_multi`)
3. Implement database parameter extraction
4. **Default engine fallback**: Make `"engine"` parameter optional; if omitted, use `chat_engine_cache_get_default(database)`
5. **Image/Multimodal Support (Day One)**:
   - Accept `content` as string OR array of parts (text + images)
   - Support OpenAI-compatible vision format:

     ```json
     {
       "messages": [{
         "role": "user",
         "content": [
           {"type": "text", "text": "What's in this image?"},
           {"type": "image_url", "image_url": {"url": "data:image/jpeg;base64,..."}}
         ]
       }]
     }
     ```

   - Pass through to provider unchanged (OpenAI, xAI, Ollama support this natively)
   - Add engine limits: `max_images_per_message`, `max_payload_mb`
6. Default params from engine config: use engine `default_params` when client omits `temperature`, `max_tokens`, etc.
7. Integrate with proxy, builder, and parser
8. **Register endpoints** in `src/api/api_service.c`:
   - Add to `endpoint_expects_json()`: `"conduit/chat"`, `"conduit/chats"`
   - Add routing `strcmp()` blocks for both paths
9. Unit and integration tests with mock server

### Phase 4: Authenticated Endpoints (`auth_chat`, `auth_chats`)

1. Create `auth_chat/auth_chat.h` and `auth_chat/auth_chat.c`
2. Create `auth_chats/auth_chats.h` and `auth_chats/auth_chats.c`
3. Implement JWT validation (reuse existing helpers from `conduit_helpers.h`)
4. Extract database from JWT claims
5. **Register endpoints** in `src/api/api_service.c`:
   - Add to `endpoint_requires_auth()`: `"conduit/auth_chat"`, `"conduit/auth_chats"`
   - Add to `endpoint_expects_json()`: `"conduit/auth_chat"`, `"conduit/auth_chats"`
   - Add routing `strcmp()` blocks
6. Unit and integration tests

### Phase 5: Cross-Database Endpoints (`alt_chat`, `alt_chats`)

1. Create `alt_chat/alt_chat.h` and `alt_chat/alt_chat.c`
2. Create `alt_chats/alt_chats.h` and `alt_chats/alt_chats.c`
3. Implement database override from request body
4. **Register endpoints** in `src/api/api_service.c`:
   - Add to `endpoint_requires_auth()`: `"conduit/alt_chat"`, `"conduit/alt_chats"`
   - Add to `endpoint_expects_json()`: `"conduit/alt_chat"`, `"conduit/alt_chats"`
   - Add routing `strcmp()` blocks
5. Unit and integration tests

### Phase 6: Additional Provider Support

1. Implement Anthropic native format (if needed beyond OpenAI-compatible)
2. Implement Ollama native format support
3. Add provider-specific configuration options
4. Test with each provider

### Phase 7: Conversation History with Content-Addressable Storage + Brotli

Instead of storing full JSON in `convos.history`, implement hash-based storage with Brotli compression:

1. **Create `conversation_segments` table** with Brotli-compressed content (new Helium migration)
2. **Extend `convos` table** with `segment_refs` JSONB column, usage tracking columns (new Helium migration)
3. **Add Brotli compression/decompression wrappers** (using existing `src/utils/utils_compression.c` functions)
4. Store each message as a compressed segment with SHA-256 hash
5. Store conversation turns as arrays of hash references in `convos.segment_refs`
6. Add usage tracking columns (tokens, cost, duration, session_id)
7. **Create new QueryRefs** (A: Get Segments, B: Store Segment, C: Store Chat with Hashes) in Helium
8. Unit tests for compression, hashing, and storage

**Storage Pipeline:**

```text
Message JSON -> SHA-256 Hash -> Brotli Compress -> Store in conversation_segments
                                     |
                               Store hash reference in convos.segment_refs
```

**Benefits from Day 1:**

- 95%+ storage savings vs traditional approach
- Additional 3-5x compression with Brotli (total 98%+ savings)
- Automatic deduplication of system prompts and common queries
- Audit-ready: immutable, content-addressed history

### Phase 8: Update Existing Chat Queries

Modify existing queries to work with new storage model:

1. **QueryRef #036** (Store Chat): Update to use hash-based storage
2. **QueryRef #041** (Get Chat): Update to reconstruct from hash references
3. **QueryRef #039** (Get Chats List): Add segment count and storage metrics
4. **New QueryRef D** (Reconstruct Conversation): New query for audit trails
5. **New QueryRef E** (Find by Segment): New query for content search
6. Maintain backwards compatibility with legacy text columns during transition

### Phase 9: Context Hashing for Client-Server Optimization

1. Implement SHA-256 hashing of message content
2. Add `context_hashes` field to chat requests
3. Client sends hashes instead of full context
4. Server reconstructs from cache/DB using QueryRef A (Get Segments)
5. Reduces bandwidth by ~50-90% for long conversations

### Phase 10: Local Disk Cache and LRU

1. Add LRU disk cache (1GB limit) for hot segments
2. Cache stores uncompressed segments (avoid repeated decompression)
3. Background sync from cache to DB
4. Optimized for conversations active within last hour

### Phase 11: Cross-Server Segment Recovery

1. Batch query for missing segments
2. Automatic fallback to database when local cache misses
3. Optimistic pre-fetching based on conversation patterns
4. Multi-server conversation continuity

### Phase 12: Streaming Support

1. Add streaming response support (SSE)
2. Implement `stream: true` parameter handling
3. Endpoint: `POST /api/conduit/auth_chat/stream`

### Phase 13: Advanced Multi-modal Features (Media Upload, Audio, Files)

**Key Insight**: The content-addressable + Brotli design is perfect for multi-modal. Basic image support is already in Phase 3 (day one).

#### Already Implemented (Phase 3 - Day One)

- **Content arrays** - `content` can be string OR array of parts
- **Base64 images** - Inline data URIs work out of the box
- **External image URLs** - Pass through to providers
- **Engine limits** - `max_images_per_message`, `max_payload_mb`

#### What's in This Phase (Advanced)

##### Step 1: Separate Media Upload Endpoint

New endpoint: `POST /api/conduit/upload` or `/auth_upload`

```text
Client -> Upload 10MB image -> Server stores in media_assets table
                      |
              Returns: media_hash: "abc123..."
                      |
Client -> Chat message: {"type": "image_url", "image_url": {"url": "media:abc123"}}
                      |
Server -> Injects real base64/URL before proxying to AI provider
```

**Benefits:**

- No repeated 10MB uploads in long conversations
- Works with load-balanced servers (fetch missing media from DB)
- Same content-addressable magic
- Store large binaries in S3/minio or `media_assets` table

##### media_assets Table

```sql
CREATE TABLE media_assets (
    media_hash VARCHAR(64) PRIMARY KEY,  -- SHA-256 of file content
    media_data BLOB,                     -- Binary data (or S3 URL)
    media_size INTEGER,
    mime_type VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_accessed TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    access_count INTEGER DEFAULT 1
);
```

##### Step 2: Extend Segments Table

Add optional columns to `conversation_segments`:

```sql
ALTER TABLE conversation_segments ADD COLUMN content_type VARCHAR(20) DEFAULT 'text';
-- text | image | pdf | audio | document

ALTER TABLE conversation_segments ADD COLUMN mime_type VARCHAR(100);
-- image/jpeg, application/pdf, etc.

ALTER TABLE conversation_segments ADD COLUMN metadata JSONB;
-- {"width":1024, "height":768, "detail":"high", "approx_tokens":450}
```

##### Step 3: Provider-Specific Handling

| Provider | Vision Support | Notes |
|----------|---------------|-------|
| **OpenAI** | Full | GPT-4V, GPT-4o - perfect |
| **xAI Grok** | Full | Same content array format |
| **Ollama** | Full | llava, bakllava models |
| **Anthropic** | Different format | Needs translation in request builder |
| **Gradient AI** | Limited | Text mostly, let it fail gracefully |

##### Anthropic Translation Example

```c
json_t* convert_openai_to_anthropic_vision(json_t* openai_message) {
    // OpenAI: [{"type": "image_url", "image_url": {"url": "..."}}]
    // Anthropic: [{"type": "image", "source": {"type": "base64", "media_type": "...", "data": "..."}}]

    json_t* anthropic_content = json_array();
    size_t index;
    json_t* part;

    json_array_foreach(openai_message, index, part) {
        const char* type = json_string_value(json_object_get(part, "type"));

        if (strcmp(type, "image_url") == 0) {
            json_t* image_url = json_object_get(part, "image_url");
            const char* url = json_string_value(json_object_get(image_url, "url"));

            // Convert to Anthropic format
            json_t* anthropic_image = json_pack("{s:s, s:{s:s, s:s, s:s}}",
                "type", "image",
                "source",
                    "type", "base64",
                    "media_type", extract_mime_type(url),
                    "data", extract_base64_data(url)
            );
            json_array_append_new(anthropic_content, anthropic_image);
        } else {
            // Text passes through
            json_array_append(anthropic_content, part);
        }
    }

    return anthropic_content;
}
```

##### Step 4: Add Engine Limits

```json
{
    "provider": "openai",
    "model": "gpt-4o",
    "max_images_per_message": 5,
    "max_payload_mb": 20,
    "supported_modalities": ["text", "image"]
}
```

#### Nice-to-Haves

##### Image Validation and Optimization

- Size/resolution validation (prevent 8K photo token explosion)
- Auto-resize before sending to provider
- Vision-specific token estimator (OpenAI charges by resolution/detail)

##### Rate Limiting per Media Type

```c
// Prevent abuse
if (request->image_count > engine->max_images_per_message) {
    return error("Too many images (max %d)", engine->max_images_per_message);
}
if (request->total_payload_mb > engine->max_payload_mb) {
    return error("Payload too large (max %d MB)", engine->max_payload_mb);
}
```

##### Moderation

- Optional image moderation scan on upload
- Easy hook in upload endpoint

### Phase 14: Advanced Features

- **Function Calling**: OpenAI function calling support, tool definitions, function results
- **Response Caching**: Cache frequent queries to reduce costs
- **Load Balancing**: Distribute across multiple API keys for same provider
- **Fallback Engines**: Auto-failover to backup engine on failure
- **Usage Analytics**: Dashboard for usage and costs
- **Prompt Templates**: Pre-defined prompt templates per engine
- **Conversation Management**: List, retrieve, delete conversation history
- **Real-time Cost Tracking**: Accurate per-request cost calculation
- **A/B Testing**: Compare responses from different engines

### Potential New Providers

- **Google Gemini** (Vertex AI)
- **Cohere**
- **Mistral AI**
- **AWS Bedrock**
- **Azure OpenAI**

## API Registration

Add to `src/api/api_service.c`:

### Protected Endpoints (JWT Required)

```c
// In endpoint_requires_auth() array:
"conduit/auth_chat",
"conduit/auth_chats",
"conduit/alt_chat",
"conduit/alt_chats",
```

### JSON Body Endpoints

```c
// In endpoint_expects_json() array:
"conduit/chat",
"conduit/chats",
"conduit/auth_chat",
"conduit/auth_chats",
"conduit/alt_chat",
"conduit/alt_chats",
```

### Route Handlers

```c
// Public endpoints
else if (strcmp(path, "conduit/chat") == 0) {
    return handle_conduit_chat_request(connection, url, method, upload_data,
                                       upload_data_size, con_cls);
}
else if (strcmp(path, "conduit/chats") == 0) {
    return handle_conduit_chats_request(connection, url, method, upload_data,
                                        upload_data_size, con_cls);
}

// Authenticated endpoints
else if (strcmp(path, "conduit/auth_chat") == 0) {
    return handle_conduit_auth_chat_request(connection, url, method, upload_data,
                                            upload_data_size, con_cls);
}
else if (strcmp(path, "conduit/auth_chats") == 0) {
    return handle_conduit_auth_chats_request(connection, url, method, upload_data,
                                             upload_data_size, con_cls);
}

// Cross-database endpoints (admin)
else if (strcmp(path, "conduit/alt_chat") == 0) {
    return handle_conduit_alt_chat_request(connection, url, method, upload_data,
                                           upload_data_size, con_cls);
}
else if (strcmp(path, "conduit/alt_chats") == 0) {
    return handle_conduit_alt_chats_request(connection, url, method, upload_data,
                                            upload_data_size, con_cls);
}
```

## Configuration

### Database-Level Configuration

Add to `src/config/config_databases.h`:

```c
typedef struct {
    // ... existing fields ...

    // Chat configuration
    bool chat_enabled;                    // Enable chat for this database
    int chat_max_tokens_default;          // Default max_tokens
    double chat_temperature_default;      // Default temperature
    int chat_timeout_seconds;             // Proxy timeout (default: 60)
    int chat_rate_limit_requests;         // Requests per minute per user
    int chat_max_chats_per_request;       // Batch limit (default: 10)
    bool chat_log_conversations;          // Whether to log conversations
} DatabaseConnection;
```

**Note**: `src/config/config_databases.c` must be updated with JSON parsing for these fields, and `src/config/config_defaults.c` must define default values for each.

### Provider-Specific Engine Configuration

Engines support provider-specific options in their JSON config:

#### OpenAI / xAI (Grok) / Gradient AI

All use OpenAI-compatible format:

```json
{
    "provider": "openai",
    "endpoint": "https://api.openai.com/v1",
    "api_key": "sk-...",
    "default_params": {
        "temperature": 0.7,
        "max_tokens": 2048,
        "top_p": 1.0,
        "frequency_penalty": 0,
        "presence_penalty": 0
    }
}
```

For **xAI (Grok)**:

```json
{
    "provider": "xai",
    "endpoint": "https://api.x.ai/v1",
    "api_key": "xai-..."
}
```

For **Gradient AI**:

```json
{
    "provider": "gradient",
    "endpoint": "https://inference.do-ai.run",
    "api_key": "model-access-key"
}
```

#### Anthropic (if using native format)

```json
{
    "provider": "anthropic",
    "endpoint": "https://api.anthropic.com/v1",
    "api_keys": ["sk-ant-..."],
    "api_version": "2023-06-01",
    "default_params": {
        "temperature": 0.7,
        "max_tokens": 2048,
        "top_k": null,
        "top_p": null
    }
}
```

**Note**: `api_keys` is an array supporting rotation (multiple keys round-robined).

#### Ollama (Local)

```json
{
    "provider": "ollama",
    "endpoint": "http://localhost:11434",
    "api_key": null,
    "use_native_api": false,
    "default_params": {
        "temperature": 0.7,
        "num_predict": 2048
    }
}
```

**Note**: Ollama can use either:

- OpenAI-compatible endpoint: `http://localhost:11434/v1/chat/completions` (when `use_native_api: false`)
- Native endpoint: `http://localhost:11434/api/chat` (when `use_native_api: true`)

## Chat Engine Cache (CEC) Strategy

Just as the Query Table Cache (QTC) preloads queries from the database at startup, the **Chat Engine Cache (CEC)** preloads AI engine configurations. This ensures fast engine lookups without database round-trips during chat processing.

### CEC Bootstrap via Internal QueryRef

The CEC is populated at database bootstrap time using an **internal QueryRef** (query_type 0 = `internal_sql`). This query is stored in the Helium database alongside all other QueryRefs but is never callable from any conduit endpoint:

```sql
-- Internal QueryRef: Get AI Chat Engines (with API keys)
-- query_type = 0 (internal_sql) - server-side only
SELECT
    key_idx AS engine_key,
    value_txt AS engine_name,
    collection AS engine_config
FROM lookups
WHERE lookup_id = 38  -- AI Chat Engines lookup
  AND status_a1 = 1
  AND (valid_after IS NULL OR valid_after <= NOW())
  AND (valid_until IS NULL OR valid_until >= NOW())
ORDER BY sort_seq;
```

The Hydrogen server executes this query during bootstrap (after QTC is populated) using `query_cache_lookup()` directly — the same code path used by `auth_service_database.c` for auth queries. The internal query enforcement in `lookup_database_and_query()` prevents any conduit client from executing this query.

### CEC Data Structure

```c
typedef struct {
    char* engine_key;           // Lookup key_idx (e.g., "0", "1", "2")
    char* name;                 // Human-readable name (e.g., "ChatGPT 4o")
    char* provider;             // Provider type: "openai", "xai", "ollama", "gradient"
    char* model;                // Model identifier (e.g., "gpt-4o")
    char* endpoint_base;        // Base URL (e.g., "https://api.openai.com")
    char* api_key;              // API key (decrypted in memory only)
    int context_limit;          // Maximum context window
    bool is_default;            // Is this the default engine?
    json_t* default_params;     // Default parameters (temperature, etc.)
    json_t* raw_config;         // Full JSON config for extensibility
    time_t loaded_at;           // When this was cached
} ChatEngineCacheEntry;

typedef struct {
    ChatEngineCacheEntry* engines;
    size_t count;
    size_t capacity;
    pthread_rwlock_t lock;      // Read-write lock for thread safety
    char* database_name;        // Which database this cache belongs to
    time_t last_refresh;        // Last time cache was refreshed
} ChatEngineCache;

// Global cache per database (stored in DatabaseQueue or global hash table)
extern ChatEngineCache** global_engine_caches;
extern size_t global_engine_cache_count;
```

### CEC Operations

```c
// Initialize cache at database bootstrap
bool chat_engine_cache_init(DatabaseQueue* db_queue);

// Load engines from database into cache
bool chat_engine_cache_load(const char* database);

// Lookup engine by name (case-insensitive)
ChatEngineCacheEntry* chat_engine_cache_lookup(
    const char* database,
    const char* engine_name
);

// Get default engine for database
ChatEngineCacheEntry* chat_engine_cache_get_default(const char* database);

// List all engines (sanitized - no API keys)
json_t* chat_engine_cache_list_all(const char* database);

// Refresh cache (periodic or on-demand)
bool chat_engine_cache_refresh(const char* database);

// Cleanup cache on shutdown
void chat_engine_cache_cleanup(const char* database);
```

### Integration with Database Bootstrap

Add to `database_bootstrap.c` (after QTC bootstrap):

```c
// After QTC bootstrap, populate Chat Engine Cache
bool bootstrap_chat_engine_cache(DatabaseQueue* db_queue) {
    log_this(0, "Bootstrapping Chat Engine Cache for %s", LOG_LEVEL_DEBUG, 1, db_queue->name);

    // Look up the CEC bootstrap QueryRef from the QTC (internal_sql type)
    // This uses query_cache_lookup() directly - same path as auth_service_database.c
    const QueryCacheEntry* cec_query = query_cache_lookup(
        db_queue->query_cache, CEC_BOOTSTRAP_QUERY_REF, "CEC Bootstrap"
    );

    if (!cec_query) {
        log_this(0, "CEC bootstrap query not found for %s", LOG_LEVEL_WARN, 1, db_queue->name);
        return false;
    }

    // Execute the bootstrap query
    QueryResult* result = execute_internal_query(db_queue, cec_query);
    if (!result || !result->success) {
        log_this(0, "Failed to bootstrap chat engines for %s", LOG_LEVEL_ERROR, 1, db_queue->name);
        return false;
    }

    // Parse results and populate cache
    ChatEngineCache* cache = calloc(1, sizeof(ChatEngineCache));
    cache->database_name = strdup(db_queue->name);
    pthread_rwlock_init(&cache->lock, NULL);

    // Parse each row into ChatEngineCacheEntry
    for (int i = 0; i < result->row_count; i++) {
        ChatEngineCacheEntry entry = parse_engine_from_row(result->rows[i]);

        // Add to cache (API keys are plaintext from database)
        add_engine_to_cache(cache, &entry);
    }

    // Store cache in DatabaseQueue
    db_queue->chat_engine_cache = cache;

    log_this(0, "Loaded %zu chat engines into cache for %s", LOG_LEVEL_INFO,
             2, cache->count, db_queue->name);

    free_query_result(result);
    return true;
}
```

### Cache Refresh Strategy

**Refresh Interval**: 1 hour (3600 seconds)

```c
#define CEC_REFRESH_INTERVAL_SECONDS 3600  // 1 hour

// Add to periodic maintenance tasks (like QTC stats refresh)
void chat_engine_cache_periodic_refresh(void) {
    time_t now = time(NULL);
    for (each database) {
        ChatEngineCache* cache = get_cache_for_database(db);
        if (now - cache->last_refresh > CEC_REFRESH_INTERVAL_SECONDS) {
            log_this(0, "Refreshing Chat Engine Cache for %s", LOG_LEVEL_DEBUG, 1, db);
            chat_engine_cache_refresh(db);
        }
    }
}
```

#### On-Demand Refresh (Admin Endpoint)

```c
// POST /api/conduit/admin/refresh_engines
// Forces immediate cache refresh for a database
// Useful when new engines are added or API keys are rotated
```

##### Future: Trigger-Based Refresh

```c
// Database trigger on lookups table updates
// Notifies Hydrogen to refresh cache for that database
// Eliminates the 1-hour lag when engines change
```

### Memory Management

```c
// Cleanup cache entry
void chat_engine_cache_entry_clear(ChatEngineCacheEntry* entry) {
    if (entry->api_key) {
        // Zero out API key memory before freeing (security best practice)
        memset(entry->api_key, 0, strlen(entry->api_key));
        free(entry->api_key);
        entry->api_key = NULL;
    }
    // ... free other fields
}
```

### CEC Benefits

1. **Fast Lookups**: O(1) hash table lookup vs database query
2. **Reduced Database Load**: No queries needed for engine config
3. **Consistent Performance**: Engine availability doesn't depend on DB latency
4. **Offline Resilience**: Can queue chat requests even if DB is temporarily unavailable
5. **API Key Isolation**: Server-side cache only — API keys never exposed to clients

## Conversation Context Hashing (Content-Addressable Storage)

To reduce bandwidth between client and server during long conversations, implement **content-addressable conversation segment caching**.

### The Problem - Bandwidth Waste in Multi-Turn Conversations

In a multi-turn conversation:

- Call 1: Client sends A -> Server -> AI, receives B
- Call 2: Client sends A+B+C -> Server -> AI, receives D
- Call 3: Client sends A+B+C+D+E -> Server -> AI, receives F

Each call repeats the entire growing context, wasting bandwidth.

### The Solution

Use **SHA-256 hashing** to create content-addressable segments:

```text
Client                     Server                  Database/Cache
  |                          |                           |
  |  POST /auth_chat         |                           |
  |  {                       |                           |
  |    "engine": "gpt-4o",   |                           |
  |    "messages": [          |                           |
  |      {"role": "user",     |                           |
  |       "content": "A"}     |                           |
  |    ]                      |                           |
  |  }                        |                           |
  |-------------------------->|                           |
  |                          |  Compute SHA256(A) = H1   |
  |                          |  Store in cache + DB      |
  |                          |  Send to AI provider      |
  |                          |  Receive B                |
  |                          |  Compute SHA256(B) = H2   |
  |  {"hash": "H2",           |  Store in cache + DB      |
  |   "response": "B"}        |                           |
  |<--------------------------|                           |
  |                          |                           |
  |  POST /auth_chat         |                           |
  |  {                       |                           |
  |    "engine": "gpt-4o",    |                           |
  |    "context_hashes": [    |                           |
  |      "H1", "H2"           |  Look up H1, H2 in cache  |
  |    ],                      |  Fall back to DB if miss  |
  |    "messages": [           |  Reconstruct full context |
  |      {"role": "user",       |                           |
  |       "content": "C"}      |                           |
  |    ]                       |                           |
  |  }                         |                           |
  |-------------------------->|                           |
  |                           |  Compute SHA256(A+B+C)=H3 |
  |                           |  Send A+B+C to AI         |
  |                           |  Receive D                |
  |  {"hash": "H4",            |  Compute SHA256(D) = H4   |
  |   "response": "D"}         |  Store H3, H4             |
  |<--------------------------|                           |
```

### Request/Response Format with Hashing

#### Initial Request (no context)

```json
{
    "engine": "ChatGPT 4o",
    "messages": [
        {"role": "user", "content": "Tell me about Python"}
    ]
}
```

##### Response

```json
{
    "success": true,
    "hash": "a1b2c3d4...",
    "segment_hash": "e5f6g7h8...",
    "message": {
        "role": "assistant",
        "content": "Python is a programming language..."
    }
}
```

##### Follow-up Request (with context hashes)

```json
{
    "engine": "ChatGPT 4o",
    "context_hashes": ["e5f6g7h8..."],
    "messages": [
        {"role": "user", "content": "What about data structures?"}
    ]
}
```

### Local Disk Cache (LRU)

```c
typedef struct {
    char* hash;                 // SHA-256 key
    char* content;              // JSON message content
    size_t size;                // Size in bytes
    time_t last_accessed;       // For LRU eviction
    uint32_t access_count;      // For LFU variant
} ConversationSegmentCacheEntry;

typedef struct {
    ConversationSegmentCacheEntry* entries;
    size_t count;
    size_t max_entries;         // Configurable (default: 10000)
    size_t max_size_bytes;      // Configurable (default: 1GB)
    char* cache_dir;            // "/var/cache/hydrogen/conv_segments"
    pthread_mutex_t lock;
} ConversationSegmentCache;

// Initialize cache
bool conv_segment_cache_init(const char* cache_dir, size_t max_size);

// Store segment (writes to disk + optionally DB)
bool conv_segment_cache_store(const char* hash, const char* content);

// Retrieve segment (checks disk cache, falls back to DB)
char* conv_segment_cache_retrieve(const char* hash);

// Multi-retrieve for batch context reconstruction
char** conv_segment_cache_retrieve_multi(const char** hashes, size_t count);

// Background sync to database
void conv_segment_cache_sync_to_db(void);
```

### Hashing Benefits

1. **Reduced Bandwidth**: Client only sends new content + hashes, not full history
2. **Cross-Server Mobility**: Load balancer can route to any server; missing segments fetched from DB
3. **Deduplication**: Identical message content (e.g., system prompts) stored once
4. **Persistence**: Conversation history survives server restarts
5. **LRU Eviction**: Local cache limited to 1GB, cold data purged, hot data retained

### Storage Efficiency and Audit Trail Advantages

The content-addressable approach transforms how we store conversation histories, making audit trails far more efficient:

#### Traditional Storage (Inefficient)

```sql
-- Each conversation turn stores the ENTIRE growing context:

-- Turn 1: 500 bytes (User: "Hello")
INSERT INTO convos (session_id, history)
VALUES ('sess1', '[{"role":"user","content":"Hello"}]');

-- Turn 2: 1,200 bytes (User+Bot messages duplicated!)
INSERT INTO convos (session_id, history)
VALUES ('sess1', '[{"role":"user","content":"Hello"},{"role":"assistant","content":"Hi there!"},{"role":"user","content":"How are you?"}]');

-- Turn 3: 2,500 bytes (All previous messages duplicated AGAIN!)
INSERT INTO convos (session_id, history)
VALUES ('sess1', '[{"role":"user","content":"Hello"},{"role":"assistant","content":"Hi there!"},{"role":"user","content":"How are you?"},{"role":"assistant","content":"I am doing well!"},{"role":"user","content":"Tell me more"}]');

-- Total storage for 3-turn conversation: ~4,200 bytes
-- With 10 turns: ~25,000 bytes (quadratic growth!)
```

**Problems:**

- Quadratic storage growth (O(n^2))
- Cannot reconstruct history without storing full snapshots
- Wasted space on duplicate data
- Difficult to audit: which parts of conversations share common prompts?

#### Content-Addressable Storage (Efficient)

```sql
-- Store each unique message ONCE in conversation_segments:
INSERT INTO conversation_segments (segment_hash, segment_content) VALUES
('a3f5...', compressed('{"role":"user","content":"Hello"}')),
('b7e2...', compressed('{"role":"assistant","content":"Hi there!"}')),
('c9d1...', compressed('{"role":"user","content":"How are you?"}')),
('d4a8...', compressed('{"role":"assistant","content":"I am doing well!"}')),
('e2b5...', compressed('{"role":"user","content":"Tell me more"}'));

-- convos table stores ONLY hash references:
INSERT INTO convos (session_id, segment_refs) VALUES
('sess1', '["a3f5..."]'),
('sess1', '["a3f5...","b7e2...","c9d1..."]'),
('sess1', '["a3f5...","b7e2...","c9d1...","d4a8...","e2b5..."]');

-- Total storage for 3-turn conversation: ~374 bytes (vs 4,200 traditional)
-- With 10 turns: ~600 bytes (vs 25,000 traditional) - 98% savings!
```

**Advantages:**

1. **Linear Storage Growth (O(n))**: Each new message adds only its own size + hash reference
2. **Automatic Deduplication**: Identical system prompts, common questions, shared context stored once across ALL conversations
3. **Efficient Audit Trails**:

   ```sql
   -- Find all conversations that used a specific system prompt:
   SELECT session_id FROM convos
   WHERE segment_refs @> '["a3f5..."]';

   -- Find conversations with similar context:
   SELECT session_id, COUNT(*) as shared_segments
   FROM convos, jsonb_array_elements_text(segment_refs) as ref
   WHERE ref IN (SELECT UNNEST(segment_refs) FROM convos WHERE session_id = 'sess1')
   GROUP BY session_id
   ORDER BY shared_segments DESC;
   ```

4. **Immutable History**: Once a segment is stored with hash H, it never changes. Perfect for audit compliance.
5. **Compression**: Hash references compress extremely well (repeated patterns in reference lists)
6. **Time Travel**: Can reconstruct conversation state at ANY point by truncating the hash list

#### Real-World Savings Example

Consider 1,000 users, each with 20-turn conversations:

| Metric | Traditional | Content-Addressable | Savings |
|--------|-------------|---------------------|---------|
| Avg convo size | 50 KB | 2 KB | 96% |
| Total storage | 50 MB | 2 MB + shared segments | ~95% |
| System prompt dedup | 0% | 100% (stored once) | - |
| Audit query time | Slow (full text search) | Fast (hash index) | 10x+ |

#### Audit Trail Reconstruction

```c
// Reconstruct complete conversation history for audit:
char** reconstruct_conversation(const char* session_id) {
    // 1. Get all segment refs for session
    char** refs = db_get_segment_refs(session_id);

    // 2. Fetch each segment by hash
    char** messages = malloc(sizeof(char*) * (refs_count + 1));
    for (int i = 0; refs[i]; i++) {
        messages[i] = conv_segment_cache_retrieve(refs[i]);
    }

    // 3. Return complete conversation
    return messages;
}
```

This approach is **exactly how Git works** — content-addressable storage with Merkle tree-like properties. It's proven at massive scale.

### Hash Calculation

```c
#include <openssl/sha.h>

// Compute SHA-256 of message content
char* compute_message_hash(const char* role, const char* content) {
    SHA256_CTX ctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, role, strlen(role));
    SHA256_Update(&ctx, ":", 1);  // Separator
    SHA256_Update(&ctx, content, strlen(content));
    SHA256_Final(hash, &ctx);

    // Convert to hex string
    char* hex_hash = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex_hash + (i * 2), "%02x", hash[i]);
    }
    hex_hash[SHA256_DIGEST_LENGTH * 2] = '\0';

    return hex_hash;
}

// Compute cumulative hash of conversation context
char* compute_context_hash(const char** message_hashes, size_t count) {
    SHA256_CTX ctx;
    unsigned char hash[SHA256_DIGEST_LENGTH];

    SHA256_Init(&ctx);
    for (size_t i = 0; i < count; i++) {
        SHA256_Update(&ctx, message_hashes[i], strlen(message_hashes[i]));
        SHA256_Update(&ctx, ";", 1);  // Separator between messages
    }
    SHA256_Final(hash, &ctx);

    // Convert to hex string
    char* hex_hash = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(hex_hash + (i * 2), "%02x", hash[i]);
    }
    hex_hash[SHA256_DIGEST_LENGTH * 2] = '\0';

    return hex_hash;
}
```

## Existing Acuranzo Schema Assessment

The Helium/Acuranzo project already has substantial chat-related infrastructure:

### What Already Exists

#### 1. Lookup 038 - AI Chat Engines (Migration 1063)

Already stores AI chat engine configurations in the `lookups` table:

```json
{
    "icon": "<img src='images/ai_openai.png'>",
    "name": "ChatGPT 4o",
    "limit": 32768,
    "model": "gpt-4o",
    "banner": {"default": ["Ask me anything."]},
    "engine": "OpenAI",
    "api_key": "<api key>",
    "country": "Demo",
    "default": true,
    "endpoint": "https://api.openai.com/v1/chat/completions",
    "location": "Demo",
    "authority": "Demo",
    "cost_prompt": 0.00000,
    "organization": "<org>",
    "cost_completion": 0.00000
}
```

**Status**: Workable with minor additions

- API keys are plaintext (as designed)
- Needs additional fields: `provider` (openai, xai, ollama, gradient)
- Should add more demo engines (Grok, Ollama, Gradient AI)

#### 2. convos Table (Migration 1008)

Already exists for storing conversations:

```sql
CREATE TABLE convos (
    convos_id       INTEGER NOT NULL PRIMARY KEY,
    convos_ref      VARCHAR(50) NOT NULL,
    convos_keywords TEXT,
    convos_icon     TEXT,
    prompt          TEXT,
    response        TEXT,
    context         TEXT,
    history         TEXT,
    collection      JSON,
    ...common fields...
);
```

**Status**: Workable but needs extension

- Missing: `engine_name`, `tokens_prompt`, `tokens_completion`, `cost_total`, `session_id`
- Current structure stores single messages; may need adjustment for conversation threads

#### 3. Existing Queries

| QueryRef | Purpose | Status |
|----------|---------|--------|
| #036 | Store Chat | Reusable |
| #037 | Chats Missing Icon+Keywords | Reusable |
| #038 | Update Chat Icon+Keywords | Reusable |
| #039 | Get Chats List | Reusable |
| #040 | Get Chats List + Search | Reusable |
| #041 | Get Chat | Reusable |
| #044 | Get Filtered Lookup: AI Models | **Critical** - Already filters API keys |

#### 4. Additional Lookups

- **Lookup 039**: AI Auditor Engines (Migration 1064)
- **Lookup 052**: AI Summary Engines (Migration 1085)

**Status**: Bonus - May be useful for future features

### Summary

| Component | Status | Work Required |
|-----------|--------|---------------|
| AI Chat Engines lookup | Exists | Add more engines (Grok, Ollama, Gradient), minor JSON field additions |
| convos table | Exists | Add `segment_refs` JSONB column, usage tracking columns (Phase 7) |
| conversation_segments table | **NEW** | Brotli-compressed content-addressable storage |
| Queries | Exists | Add new QueryRefs (A-F, provisional numbers assigned at migration time) |
| API key filtering | Exists | Already implemented in QueryRef #044 |
| Brotli compression | Exists | Already in Hydrogen - just need to integrate |
| Internal query enforcement | **NEW** | Hydrogen: block type 0-3 from conduit; Helium: optional `is_internal` column |

**Verdict**: The existing schema is **workable and well-designed**. We need:

1. **New Table**: `conversation_segments` with Brotli-compressed content (Helium migration)
2. **New QueryRefs** (provisional numbers assigned during migration):
   - **CEC Bootstrap**: Get AI Engines with API keys (`internal_sql` type 0) — server-side only
   - **QueryRef A**: Get Conversation Segments by Hash
   - **QueryRef B**: Store Conversation Segment (with Brotli compression)
   - **QueryRef C**: Store Chat with Hashes (updated #036)
   - **QueryRef D**: Reconstruct Conversation
   - **QueryRef E**: Find Conversations by Segment
   - **QueryRef F**: Get Conversation Statistics
3. **Update 3 Existing Queries**: #036, #039, #041 for hash-based storage
4. **Helium Migration**: Extend `convos` table (segment_refs, usage tracking)
5. **Hydrogen Change**: Internal query enforcement in `lookup_database_and_query()`
6. **Hydrogen Change**: Add libcurl to core dependencies
7. **Optional**: Add more demo engines to Lookup 038

## Low-Hanging Fruit Enhancements

These easy wins provide big ROI with minimal effort and are incorporated into the phase where they belong:

### Default Engine Fallback (Phase 3, ~30 min)

- Make `"engine"` parameter optional in all chat endpoints
- If omitted: use `chat_engine_cache_get_default(database)`
- Reduces "engine not found" errors, improves UX

```c
// In chat request handler:
if (!engine_name || strlen(engine_name) == 0) {
    engine = chat_engine_cache_get_default(database);
    if (!engine) {
        return error_response("No default engine configured");
    }
}
```

### Token Pre-Counting + Hard Enforcement (Phase 2)

- Quick estimation before proxying: `strlen(prompt) / 4 + system_tokens`
- If `(prompt_tokens + max_tokens) > engine->limit` -> immediate 400
- Prevents wasted API calls, instant feedback
- Store `estimated_tokens` vs `actual_tokens` for audit

```c
int estimate_tokens(const char* text) {
    // Rough estimate: 4 chars ~ 1 token (good enough for limit checking)
    return strlen(text) / 4 + 4;  // +4 for message overhead
}

bool check_token_limit(const ChatRequest* request, const ChatEngineConfig* engine) {
    int total_estimate = 0;
    for (each message in request->messages) {
        total_estimate += estimate_tokens(message->content);
    }
    total_estimate += request->max_tokens;

    if (total_estimate > engine->context_limit) {
        return false;  // Return 400 with clear error
    }
    return true;
}
```

### Parallel Batch Execution for `/chats` (Phase 3, ~2 hours)

- Use `curl_multi` interface instead of sequential `curl_easy_perform`
- True parallelism cuts batch latency from N x single to ~max(single)

```c
// Use curl_multi for parallel requests
CURLM* multi_handle = curl_multi_init();
for (each chat in batch) {
    CURL* easy = create_chat_request(chat);
    curl_multi_add_handle(multi_handle, easy);
}

// Process all simultaneously
int running;
do {
    curl_multi_perform(multi_handle, &running);
} while (running > 0);
```

### Retry + Exponential Backoff in `chat_proxy.c` (Phase 2, 1 hour)

- 1-2 retries on 429/5xx/network errors with jitter
- Per-engine failure counter in CEC (if >3 in 5 min -> mark unhealthy)
- Auto-fallback to default engine if configured

```c
#define MAX_RETRIES 2
#define BASE_RETRY_DELAY_MS 1000

ChatProxyResult* chat_proxy_send_with_retry(
    const ChatEngineConfig* engine,
    const json_t* request_body
) {
    for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
        ChatProxyResult* result = chat_proxy_send(engine, request_body);

        if (result->http_status == 200) {
            return result;  // Success
        }

        if (should_retry(result->http_status)) {
            int delay = BASE_RETRY_DELAY_MS * (1 << attempt);  // Exponential
            delay += random_jitter(0, 500);  // Add jitter
            usleep(delay * 1000);

            // Track failure
            engine->failure_count++;
            if (engine->failure_count > 3) {
                engine->is_healthy = false;
                // Try fallback to default engine
                engine = chat_engine_cache_get_default(database);
            }
        } else {
            return result;  // Don't retry client errors
        }
    }
}
```

### Default Params from Engine Config

- Engines have `"default_params"` in JSON (temperature, max_tokens, etc.)
- In `chat_request_build_auto`, use engine defaults if client omits them

```c
json_t* chat_request_build_auto(
    const ChatEngineConfig* engine,
    const json_t* client_params
) {
    json_t* request = json_object();

    // Use client params or fall back to engine defaults
    double temperature = json_real_value(
        json_object_get(client_params, "temperature") ?
        json_object_get(client_params, "temperature") :
        json_object_get(engine->default_params, "temperature")
    ) ?: 0.7;

    json_object_set_new(request, "temperature", json_real(temperature));
    // ... etc for max_tokens, top_p, etc.

    return request;
}
```

### Structured Observability Counters (Phase 1-2)

- Global counters per provider: requests, tokens, cost, latency, errors
- Log every 60s (like QTC stats)
- Future: Prometheus metrics endpoint

```c
typedef struct {
    atomic_uint requests_total;
    atomic_uint tokens_prompt;
    atomic_uint tokens_completion;
    atomic_double cost_usd;
    atomic_uint latency_ms_total;
    atomic_uint errors_total;
} ChatProviderMetrics;

ChatProviderMetrics g_metrics[PROVIDER_COUNT];

// Log every 60s
void log_chat_metrics(void) {
    for (each provider) {
        log_this(0, "Chat metrics %s: req=%u, tokens=%u/%u, cost=$%.4f, err=%u",
                 LOG_LEVEL_INFO, 5,
                 provider_name,
                 g_metrics[provider].requests_total,
                 g_metrics[provider].tokens_prompt,
                 g_metrics[provider].tokens_completion,
                 g_metrics[provider].cost_usd,
                 g_metrics[provider].errors_total);
    }
}
```

### API Key Rotation Support (CEC, Phase 1)

- Change `api_key` -> `api_keys` (JSON array) in engine config
- Round-robin or "try primary, fallback to secondary"
- Hot-reload works automatically on CEC refresh

```c
typedef struct {
    char** api_keys;
    size_t api_key_count;
    size_t current_key_index;
} ChatEngineConfig;

char* get_next_api_key(ChatEngineConfig* engine) {
    if (engine->api_key_count == 0) return NULL;
    if (engine->api_key_count == 1) return engine->api_keys[0];

    // Round-robin
    char* key = engine->api_keys[engine->current_key_index];
    engine->current_key_index = (engine->current_key_index + 1) % engine->api_key_count;
    return key;
}
```

### Quick Health-Check Endpoint (Bonus, Phase 1)

- `GET /api/conduit/engines/health?database=DemoDB` (admin only)
- Runs dummy 1-token request to each engine
- Returns status + latency for ops monitoring

```c
// Test each engine with minimal request
json_t* health_check(const char* database) {
    json_t* results = json_array();
    ChatEngineList* engines = chat_engine_cache_list_all(database);

    for (each engine in engines) {
        json_t* test_request = json_pack("{s:s, s:[{s:s, s:s}], s:i}",
            "model", engine->model,
            "messages", "role", "user", "content", "hi",
            "max_tokens", 1
        );

        double start = get_time_ms();
        ChatProxyResult* result = chat_proxy_send(engine, test_request);
        double latency = get_time_ms() - start;

        json_t* engine_health = json_pack("{s:s, s:b, s:f, s:i}",
            "engine", engine->name,
            "healthy", result->http_status == 200,
            "latency_ms", latency,
            "status", result->http_status
        );
        json_array_append_new(results, engine_health);

        chat_proxy_result_free(result);
        json_decref(test_request);
    }

    return results;
}
```

### CORS Headers (Verify Global)

- Ensure `Access-Control-Allow-Origin` is set (or specific origins)
- One-liner in MHD response setup

```c
MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
// Or: MHD_add_response_header(response, "Access-Control-Allow-Origin", "https://yourdomain.com");
```

### Client-Side Brotli Note

- Document that segment retrieval queries return Brotli-compressed blobs
- Provide JS snippet: `const decompressed = await brotliDecode(blob);`
- Modern browsers have native Brotli support via Compression Streams API

```javascript
// Client-side Brotli decompression (modern browsers)
async function decompressBrotli(compressedBlob) {
    const ds = new DecompressionStream('br');
    const decompressedStream = compressedBlob.stream().pipeThrough(ds);
    return await new Response(decompressedStream).text();
}
```

## Testing Strategy

### Unit Tests

#### Chat Engine Cache Tests

```c
// tests/unity/src/api/conduit/chat_common/chat_engine_cache_test.c
void test_chat_engine_cache_lookup_found(void);
void test_chat_engine_cache_lookup_not_found(void);
void test_chat_engine_cache_get_default(void);
void test_chat_engine_cache_list_all(void);
void test_chat_engine_cache_reload(void);
```

#### Chat Proxy Tests

```c
// tests/unity/src/api/conduit/chat_common/chat_proxy_test.c
void test_chat_proxy_send_success(void);
void test_chat_proxy_send_timeout(void);
void test_chat_proxy_send_http_error(void);
void test_chat_proxy_send_network_failure(void);
void test_chat_proxy_build_url_openai(void);
void test_chat_proxy_build_url_ollama(void);
void test_chat_proxy_build_headers_auth(void);
```

#### Request Builder Tests

```c
// tests/unity/src/api/conduit/chat_common/chat_request_builder_test.c
void test_chat_request_build_openai_format_basic(void);
void test_chat_request_build_openai_format_with_params(void);
void test_chat_request_build_anthropic_format(void);
void test_chat_request_build_ollama_native(void);
void test_chat_request_build_auto_detection(void);
```

#### Response Parser Tests

```c
// tests/unity/src/api/conduit/chat_common/chat_response_parser_test.c
void test_chat_response_parse_openai_format_success(void);
void test_chat_response_parse_openai_format_error(void);
void test_chat_response_parse_anthropic_format(void);
void test_chat_response_parse_malformed_json(void);
void test_chat_response_parse_empty_content(void);
```

### Integration Tests

#### Public Endpoints (`chat`, `chats`)

```bash
# tests/test_60_conduit_chat.sh

# Test 1: Single chat with OpenAI-compatible engine
curl -X POST http://localhost:8080/api/conduit/chat \
  -H "Content-Type: application/json" \
  -d '{"database":"DemoDB","engine":"MockEngine","messages":[{"role":"user","content":"Hello"}]}'

# Test 2: Batch chat (multiple engines)
curl -X POST http://localhost:8080/api/conduit/chats \
  -H "Content-Type: application/json" \
  -d '{"database":"DemoDB","chats":[...]}'

# Test 3: Missing database parameter (should fail)
# Test 4: Invalid engine (should fail)
# Test 5: Malformed messages (should fail)
# Test 6: Timeout handling
```

#### Authenticated Endpoints (`auth_chat`, `auth_chats`)

```bash
# tests/test_61_conduit_auth_chat.sh

# Test 1: Valid JWT, single chat
# Test 2: Valid JWT, batch chat
# Test 3: Missing JWT (should fail with 401)
# Test 4: Invalid JWT (should fail with 401)
# Test 5: Database from JWT claims (verify correct routing)
```

#### Cross-Database Endpoints (`alt_chat`, `alt_chats`)

```bash
# tests/test_62_conduit_alt_chat.sh

# Test 1: Admin JWT with database override
# Test 2: Access different database than JWT claims
# Test 3: Invalid database name (should fail)
```

#### Internal Query Enforcement Tests

```bash
# tests/test_63_internal_query_enforcement.sh

# Test 1: Authenticated client attempts to call CEC bootstrap QueryRef via auth_query -> should fail
# Test 2: Authenticated client attempts to call auth QueryRefs (1, 8, 12, etc.) via auth_query -> should fail
# Test 3: Public client attempts to call internal QueryRef via query -> should fail (already works)
# Test 4: Server-side CEC bootstrap still works (verified by chat endpoint functionality)
```

### Mock Server Tests

Create a mock AI API server for deterministic testing:

```python
# tests/mocks/mock_ai_server.py
# Flask/FastAPI server that mimics OpenAI, xAI, Anthropic responses

@app.post("/v1/chat/completions")
def mock_chat_completion(request: ChatRequest):
    return {
        "id": "mock-id",
        "model": request.model,
        "choices": [{
            "message": {"role": "assistant", "content": "Mock response"},
            "finish_reason": "stop"
        }],
        "usage": {
            "prompt_tokens": 10,
            "completion_tokens": 5,
            "total_tokens": 15
        }
    }
```

### Provider-Specific Tests

Test each supported provider:

1. **OpenAI** (if API key available in test environment)
2. **xAI (Grok)** (if API key available)
3. **Ollama** (run local Ollama in CI with tiny model)
4. **Gradient AI** (if API key available)
5. **Anthropic** (if API key available)

## Security and Reliability Considerations

### Security

1. **API Key Storage**: API keys stored as plaintext in database (access controlled at database level)
2. **API Key Exposure**: Never return API keys in any API response (QueryRef #044 filters them out)
3. **API Key Access**: Only server-side CEC has access to full engine configs with API keys
4. **Internal Query Enforcement**: CEC bootstrap QueryRef is `internal_sql` type, blocked from all conduit endpoints
5. **Request Logging**: Log conversation metadata but not content (configurable)
6. **Rate Limiting**: Per-user and per-engine rate limits to prevent abuse
7. **Timeout Handling**: Short timeouts to prevent resource exhaustion
8. **Input Validation**: Strict validation of message content length and format
9. **HTTPS Only**: All external API calls use HTTPS
10. **CORS**: Configurable CORS headers for browser clients

### Reliability

1. **Token Pre-Validation**: Estimate tokens before sending to provider (avoid wasted calls)
2. **Retry with Exponential Backoff**: 1-2 retries on 429/5xx errors with jitter
3. **Circuit Breaker**: Mark engines unhealthy after 3 failures in 5 min, fallback to default
4. **API Key Rotation**: Support multiple keys per engine (round-robin or primary/secondary)
5. **Health Checks**: Built-in endpoint to test all engines (ops monitoring)
6. **Observability**: Per-provider metrics (requests, tokens, cost, latency, errors)

## Provider API Differences

While most providers support OpenAI-compatible endpoints, there are some differences to note:

### OpenAI / xAI (Grok) / Gradient AI Providers

**Fully OpenAI-compatible** - Use the same request/response format:

```bash
# All use identical format
curl https://api.openai.com/v1/chat/completions \
  -H "Authorization: Bearer $OPENAI_API_KEY"

curl https://api.x.ai/v1/chat/completions \
  -H "Authorization: Bearer $XAI_API_KEY"

curl https://inference.do-ai.run/v1/chat/completions \
  -H "Authorization: Bearer $GRADIENT_KEY"
```

**Differences**:

- xAI adds some Grok-specific parameters (not required)
- Gradient AI uses model access keys instead of API keys

### Ollama

**Supports both native and OpenAI-compatible APIs**:

```bash
# OpenAI-compatible (recommended)
curl http://localhost:11434/v1/chat/completions \
  -d '{"model":"llama3.3","messages":[...]}'

# Native API (slightly different format)
curl http://localhost:11434/api/chat \
  -d '{"model":"llama3.3","messages":[...],"stream":false}'
```

**Implementation**: Use OpenAI-compatible endpoint by default.

### Anthropic

**Has native format, but also offers OpenAI compatibility**:

```bash
# Native format (different structure)
curl https://api.anthropic.com/v1/messages \
  -H "x-api-key: $ANTHROPIC_KEY" \
  -H "anthropic-version: 2023-06-01" \
  -d '{"model":"claude-3-opus","max_tokens":1024,"messages":[...]}'

# OpenAI-compatible (if available via proxy)
# Same format as OpenAI
```

**Implementation**: Use native format only if OpenAI-compatible endpoint unavailable.

## Dependencies

- **libcurl**: **NEW** - Required for HTTP requests to external AI APIs. Must be added to `cmake/CMakeLists-init.cmake` (`HYDROGEN_BASE_LIBS`) and `tests/test_14_library_dependencies.sh`
- **jansson**: Already used for JSON handling
- **libmicrohttpd**: Already used for webserver
- **brotli**: Already used for compression (`src/utils/utils_compression.c`, `src/globals.h`)
- **OpenSSL**: Already used (provides SHA-256 for content hashing)

## Open Questions

1. **API Key Rotation**: How should we handle API key rotation without downtime?
   - Option A: Support multiple API keys per engine (primary/secondary)
   - Option B: Hot-reload cache on config change
   - Option C: Both

2. **Content Filtering**: Should we implement content filtering/moderation?
   - Per-database setting?
   - Use provider's moderation API or implement our own?

3. **Conversation History Retention**: What's the retention policy?
   - Configurable per database?
   - Auto-purge after N days?
   - Archive to cold storage?

4. **Model Parameters**: Should we support custom model parameters per request?
   - Currently: temperature, max_tokens
   - Potential additions: top_p, frequency_penalty, presence_penalty, seed
   - Should these be restricted based on engine capabilities?

5. **Streaming Architecture**: How should streaming be implemented?
   - Use MHD's suspend/resume like queries endpoint?
   - Separate thread for SSE?
   - WebSocket support?

6. **Error Handling Strategy**: How much error detail should we expose?
   - Hide all upstream errors (generic "Engine error")?
   - Pass through sanitized error messages?
   - Log full errors server-side only?

7. **Cost Tracking**: How should we track costs?
   - Per-request tracking (immediate)
   - Async batch processing (delayed)
   - Real-time usage endpoint?

8. **Testing Strategy**: How do we test against real providers in CI?
   - Mock servers only?
   - Integration tests with real keys (costly)?
   - Hybrid approach?

9. **Authenticated Conduit Access Audit**: Before deploying internal query enforcement, are any authenticated clients currently calling type 0-3 queries through conduit endpoints? If so, those queries need to be reclassified to type 10 in Helium first.
