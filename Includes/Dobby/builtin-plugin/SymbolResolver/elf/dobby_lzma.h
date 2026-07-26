// LZMA (.gnu_debugdata) decompressor for DobbySymbolResolver.
// Ported from xDL (https://github.com/hexhacking/xDL) xdl_lzma.c.
// Self-contained: resolves liblzma.so via dlopen/dlsym, no xDL dependency.

#ifndef DOBBY_SYMBOL_RESOLVER_ELF_LZMA_H
#define DOBBY_SYMBOL_RESOLVER_ELF_LZMA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Decompress an XZ/LZMA stream (as used by ELF .gnu_debugdata).
// On success: *dst is malloc'd by this function and must be free()'d by caller.
// Returns 0 on success, non-zero on failure.
int dobby_lzma_decompress(uint8_t *src, size_t src_size, uint8_t **dst, size_t *dst_size);

#ifdef __cplusplus
}
#endif

#endif
