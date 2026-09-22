# Brotli Decompression UDR for Firebird 4

Helium migrations store large `code` / `summary` / diagram blobs as
`BROTLI_DECOMPRESS(BASE64_DECODE('…'))`. This UDR provides `BROTLI_DECOMPRESS`.

## Build artifact

Firebird loads `EXTERNAL NAME 'brotli_decfn!brotli_decompress'` from:

```text
/usr/lib64/firebird/plugins/udr/libbrotli_decfn.so
```

## Build / install

```bash
cd extras/brotli_udf_firebird
make clean && make
sudo make install
```

Requires: `firebird-devel`, `brotli-devel` (or `libbrotli-dev`), `g++`.
