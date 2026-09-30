# Brotli UDF for MSSQL

SQL CLR assembly for Brotli decompression in SQL Server 2022 Linux.

## Overview

SQL Server does not have a built-in Brotli decompression function, and Linux
SQL Server cannot load C-based native UDFs. This project provides a SQL CLR
assembly (`BrotliUdf.dll`) that wraps `libbrotlidec` for the
`${COMPRESS_START}` macro in `database_mssql.lua`.

## Prerequisites

- SQL Server 2022 Linux Developer Edition (Developer EULA; test-only)
- `dotnet` SDK 6.0+ on the build machine (not Fedora; use a scratch container)
- `libbrotlidec.so.1` available inside the SQL Server container at runtime

## Build

```bash
dotnet build -c Release
```

Output: `bin/Release/netstandard2.0/BrotliUdf.dll`

## Deploy to SQL Server

Inside the SQL Server container or via `sqlcmd`:

```sql
-- Enable CLR (one-time)
EXEC sp_configure 'clr enabled', 1; RECONFIGURE;

-- Trust the assembly (Developer edition; UNSAFE for P/Invoke)
ALTER DATABASE CURRENT SET TRUSTWORTHY ON;

-- Create the assembly from the DLL bytes
CREATE ASSEMBLY brotli_assembly FROM 'BrotliUdf.dll' WITH PERMISSION_SET = UNSAFE;

-- Create the scalar function
CREATE FUNCTION testms.brotli_decompress(@data VARBINARY(MAX))
RETURNS VARBINARY(MAX)
AS BrotliUdf.BrotliDecompress;

CREATE FUNCTION testms.brotli_decompress_string(@data VARBINARY(MAX))
RETURNS NVARCHAR(MAX)
AS BrotliUdf.BrotliDecompressString;
```

## Security notes

- `PERMISSION_SET = UNSAFE` is required because of P/Invoke to `libbrotlidec`.
- Only deploy in a trusted dev/test environment. Never deploy on production
  without a security review.
- The Developer edition EULA is test-only; see `docs/H/SECRETS.md`.
