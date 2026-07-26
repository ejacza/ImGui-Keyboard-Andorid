// LZMA (.gnu_debugdata) decompressor, ported from xDL xdl_lzma.c.
// Resolves liblzma.so via dlopen/dlsym (no xDL dependency).

#include "dobby_lzma.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <android/api-level.h>

#ifndef __LP64__
#define LZMA_PATHNAME "/system/lib/liblzma.so"
#else
#define LZMA_PATHNAME "/system/lib64/liblzma.so"
#endif

#define LZMA_SYM_CRCGEN     "CrcGenerateTable"
#define LZMA_SYM_CRC64GEN   "Crc64GenerateTable"
#define LZMA_SYM_CONSTRUCT  "XzUnpacker_Construct"
#define LZMA_SYM_ISFINISHED "XzUnpacker_IsStreamWasFinished"
#define LZMA_SYM_FREE       "XzUnpacker_Free"
#define LZMA_SYM_CODE       "XzUnpacker_Code"

#define SZ_OK 0

typedef struct ISzAlloc ISzAlloc;
typedef const ISzAlloc *ISzAllocPtr;
struct ISzAlloc {
  void *(*Alloc)(ISzAllocPtr p, size_t size);
  void (*Free)(ISzAllocPtr p, void *address);
};
typedef enum {
  CODER_STATUS_NOT_SPECIFIED,
  CODER_STATUS_FINISHED_WITH_MARK,
  CODER_STATUS_NOT_FINISHED,
  CODER_STATUS_NEEDS_MORE_INPUT
} ECoderStatus;
typedef enum {
  CODER_FINISH_ANY,
  CODER_FINISH_END
} ECoderFinishMode;

typedef void (*lzma_crcgen_t)(void);
typedef void (*lzma_crc64gen_t)(void);
typedef void (*lzma_construct_t)(void *, ISzAllocPtr);
typedef int (*lzma_isfinished_t)(const void *);
typedef void (*lzma_free_t)(void *);
typedef int (*lzma_code_t)(void *, uint8_t *, size_t *, const uint8_t *, size_t *, ECoderFinishMode,
                           ECoderStatus *);
typedef int (*lzma_code_q_t)(void *, uint8_t *, size_t *, const uint8_t *, size_t *, int,
                             ECoderFinishMode, ECoderStatus *);

static lzma_construct_t lzma_construct = NULL;
static lzma_isfinished_t lzma_isfinished = NULL;
static lzma_free_t lzma_free = NULL;
static void *lzma_code = NULL;

static void dobby_lzma_init(void) {
  void *lzma = dlopen(LZMA_PATHNAME, RTLD_NOW);
  if (NULL == lzma) return;

  lzma_crcgen_t crcgen = (lzma_crcgen_t)dlsym(lzma, LZMA_SYM_CRCGEN);
  lzma_crc64gen_t crc64gen = (lzma_crc64gen_t)dlsym(lzma, LZMA_SYM_CRC64GEN);
  lzma_construct = (lzma_construct_t)dlsym(lzma, LZMA_SYM_CONSTRUCT);
  lzma_isfinished = (lzma_isfinished_t)dlsym(lzma, LZMA_SYM_ISFINISHED);
  lzma_free = (lzma_free_t)dlsym(lzma, LZMA_SYM_FREE);
  lzma_code = dlsym(lzma, LZMA_SYM_CODE);

  if (crcgen) crcgen();
  if (crc64gen) crc64gen();

  dlclose(lzma);
}

static void *dobby_lzma_internal_alloc(ISzAllocPtr p, size_t size) {
  (void)p;
  return malloc(size);
}
static void dobby_lzma_internal_free(ISzAllocPtr p, void *address) {
  (void)p;
  free(address);
}

int dobby_lzma_decompress(uint8_t *src, size_t src_size, uint8_t **dst, size_t *dst_size) {
  size_t src_offset = 0;
  size_t dst_offset = 0;
  size_t src_remaining;
  size_t dst_remaining;
  ISzAlloc alloc = {dobby_lzma_internal_alloc, dobby_lzma_internal_free};
  long long state[4096 / sizeof(long long)];  // 8-byte aligned, large enough for XzUnpacker state
  ECoderStatus status;

  static bool inited = false;
  if (!inited) {
    dobby_lzma_init();
    inited = true;
  }
  if (NULL == lzma_code) return -1;

  lzma_construct(&state, &alloc);

  *dst_size = 2 * src_size;
  *dst = NULL;
  do {
    *dst_size *= 2;
    if (NULL == (*dst = (uint8_t *)realloc(*dst, *dst_size))) {
      lzma_free(&state);
      return -1;
    }

    src_remaining = src_size - src_offset;
    dst_remaining = *dst_size - dst_offset;

    int result;
    if (android_get_device_api_level() >= __ANDROID_API_Q__) {
      lzma_code_q_t lzma_code_q = (lzma_code_q_t)lzma_code;
      result = lzma_code_q(&state, *dst + dst_offset, &dst_remaining, src + src_offset, &src_remaining, 1,
                           CODER_FINISH_ANY, &status);
    } else {
      lzma_code_t lzma_code_fn = (lzma_code_t)lzma_code;
      result = lzma_code_fn(&state, *dst + dst_offset, &dst_remaining, src + src_offset, &src_remaining,
                            CODER_FINISH_ANY, &status);
    }
    if (SZ_OK != result) {
      free(*dst);
      lzma_free(&state);
      return -1;
    }

    src_offset += src_remaining;
    dst_offset += dst_remaining;
  } while (status == CODER_STATUS_NOT_FINISHED);

  lzma_free(&state);

  if (!lzma_isfinished(&state)) {
    free(*dst);
    return -1;
  }

  *dst_size = dst_offset;
  *dst = (uint8_t *)realloc(*dst, *dst_size);
  return 0;
}
