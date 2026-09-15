# Payload Subsystem

## Overview

The Payload subsystem is a critical component of the Hydrogen project that manages embedded resources within the executable. It serves as a built-in storage system that holds static files and resources needed by the server to deliver web pages and APIs.

## Purpose

The Payload subsystem provides a secure and efficient way to bundle and access static assets directly within the application binary. This approach eliminates the need for external file dependencies and ensures that all necessary resources are available at runtime, regardless of the deployment environment.

## Key Features

- **Embedded Storage**: Stores static files and resources directly in the executable
- **Secure Access**: Uses encryption to protect embedded content
- **On-Demand Loading**: Loads resources into memory only when needed
- **File Organization**: Maintains structured access to different types of assets
- **Resource Caching**: Provides efficient access to frequently used files

## How It Works

The subsystem extracts and decrypts embedded payloads from the executable during startup. These payloads contain compressed archives of static files that are then made available to other subsystems through a caching mechanism. This allows web servers, APIs, and other components to access their required assets without external file system dependencies.

## Importance

By embedding resources directly in the executable, the Payload subsystem ensures:

- **Self-Contained Deployments**: No additional files needed for operation
- **Security**: Resources are protected through encryption
- **Reliability**: Eliminates file system access issues
- **Performance**: Fast access to cached resources
- **Portability**: Works across different environments without configuration

## Binary Layout

The release binary embeds the encrypted payload at the end of the executable using a
marker-based layout. The format is:

```layout
[ executable code/data ] [ encrypted payload ] [ payload marker ] [ payload size ]
```

### Fields

| Offset | Field | Description |
| --- | --- | --- |
| End of binary | Executable | The original ELF/PE/Mach-O binary contents |
| After executable | Encrypted payload | AES-encrypted, Brotli-compressed archive of static files |
| After payload | Marker | The literal string `<<< HERE BE ME TREASURE >>>` (27 bytes, no null terminator) |
| After marker | Payload size | 8-byte big-endian unsigned integer giving the payload length in bytes |

### Extraction

At startup, `extract_payload()` in
[`src/payload/payload.c`](/elements/001-hydrogen/hydrogen/src/payload/payload.c)
(scanned from the end of the file) locates the **last** occurrence of the marker,
reads the 8 subsequent bytes as a big-endian `size_t`, and treats the `payload_size`
bytes immediately preceding the marker as the encrypted payload. The marker and size
footer allow appending a payload to any pre-existing executable without rewriting it.

The embedding step is performed by
[`cmake/scripts/embed_payload.sh`](/elements/001-hydrogen/hydrogen/cmake/scripts/embed_payload.sh),
which concatenates the binary and payload, appends the marker string, then appends
the 8-byte big-endian size.

### Security

The payload is encrypted with ChaCha20-Poly1305 using a key from
`Server.PayloadKey` (resolved from `${env.PAYLOAD_KEY}` at runtime). It is never
written in plaintext to disk. Only the marker + size footer are appended in cleartext;
no file contents or metadata appear in the marker region.