// BrotliUdf.cs
// SQL CLR assembly for Brotli decompression in SQL Server 2022 Linux.
// This wraps libbrotlidec to provide a T-SQL scalar function for decompressing
// Brotli-compressed data stored as base64 in the queries table.
//
// Requirements:
// - SQL Server Developer Edition on Linux container (CLR enabled)
// - libbrotlidec.so must be loadable by the CLR host
// - Build with: dotnet build (requires .NET SDK with SqlServer CLR support)
//
// Usage in T-SQL:
//   CREATE ASSEMBLY brotli_assembly FROM 'BrotliUdf.dll' WITH PERMISSION_SET = UNSAFE;
//   CREATE FUNCTION testms.brotli_decompress(@data VARBINARY(MAX))
//   RETURNS VARBINARY(MAX) AS BrotliUdf.BrotliDecompress;

using System;
using System.Data.SqlTypes;
using System.Runtime.InteropServices;

public static class BrotliUdf
{
    // P/Invoke to libbrotlidec for decompression
    [DllImport("libbrotlidec.so.1",
        CallingConvention = CallingConvention.Cdecl,
        SetLastError = true)]
    private static extern int BrotliDecoderDecompress(
        UIntPtr encoded_size,
        byte[] encoded_buffer,
        byte[] output_buffer,
        out UIntPtr decoded_size);

    // SQL CLR entry point: decompress Brotli-compressed VARBINARY data
    [Microsoft.SqlServer.Server.SqlFunction]
    public static SqlBytes BrotliDecompress(SqlBytes input)
    {
        if (input.IsNull || input.Value == null || input.Value.Length == 0)
            return new SqlBytes(new byte[0]);

        byte[] encoded = input.Value;

        // Start with a reasonable buffer size; Brotli typically expands 2-10x
        UIntPtr outputCapacity = (UIntPtr)(encoded.Length * 10);
        if (outputCapacity.ToUInt64() < 1024)
            outputCapacity = (UIntPtr)1024;

        byte[] output = new byte[outputCapacity.ToUInt64()];
        UIntPtr decodedSize;

        int result = (int)BrotliDecoderDecompress(
            (UIntPtr)encoded.Length,
            encoded,
            output,
            out decodedSize);

        if (result == 0)
        {
            // Success - trim to actual decoded size
            byte[] trimmed = new byte[decodedSize.ToUInt64()];
            Array.Copy(output, 0, trimmed, 0, decodedSize.ToUInt64());
            return new SqlBytes(trimmed);
        }

        // Decompression failed - return NULL
        return SqlBytes.Null;
    }

    // SQL CLR entry point: decompress and return as NVARCHAR(MAX)
    [Microsoft.SqlServer.Server.SqlFunction]
    public static SqlString BrotliDecompressString(SqlBytes input)
    {
        SqlBytes result = BrotliDecompress(input);
        if (result.IsNull)
            return SqlString.Null;

        return new SqlString(
            System.Text.Encoding.UTF8.GetString(result.Value));
    }
}
