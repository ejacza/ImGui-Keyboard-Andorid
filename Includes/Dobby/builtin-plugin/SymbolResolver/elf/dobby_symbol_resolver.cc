// DobbySymbolResolver (ELF/Android) — rewritten using xDL's algorithms.
// Ported from xDL (https://github.com/hexhacking/xDL) xdl.c:
//   - .dynsym from memory via PT_DYNAMIC, O(1) GNU hash + SysV hash fallback
//   - .symtab from disk via section headers (linear scan)
//   - .gnu_debugdata fallback (LZMA-compressed mini-ELF .symtab)
//   - clean load_bias = base - min_vaddr from PT_LOAD phdrs
// No dependency on xDL library; all logic is self-contained here.

#include "SymbolResolver/dobby_symbol_resolver.h"
#include "common_header.h"

#include <elf.h>
#include <dlfcn.h>
#include <link.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <string.h>
#include <stdlib.h>

#include "PlatformUtil/ProcessRuntimeUtility.h"
#include "dobby_lzma.h"

#include <vector>

#undef LOG_TAG
#define LOG_TAG "DobbySymbolResolver"

// ======================================================================
// file I/O helpers (mmap disk file for .symtab / .gnu_debugdata)
// ======================================================================

static void file_mmap(const char *file_path, uint8_t **data_ptr, size_t *data_size_ptr) {
  uint8_t *mmap_data = NULL;
  size_t file_size = 0;

  int fd = open(file_path, O_RDONLY, 0);
  if (fd < 0) {
    ERROR_LOG("%s open failed", file_path);
    goto finished;
  }

  {
    struct stat s;
    int rt = fstat(fd, &s);
    if (rt != 0) {
      ERROR_LOG("fstat failed");
      goto finished;
    }
    file_size = s.st_size;
  }

  mmap_data = (uint8_t *)mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_FILE | MAP_PRIVATE, fd, 0);
  if (mmap_data == MAP_FAILED) {
    ERROR_LOG("mmap failed");
    mmap_data = NULL;
  }

finished:
  close(fd);

  if (data_size_ptr)
    *data_size_ptr = file_size;
  if (data_ptr)
    *data_ptr = mmap_data;
}

static void file_unmap(void *data, size_t data_size) {
  int ret = munmap(data, data_size);
  if (ret != 0) {
    ERROR_LOG("munmap failed");
  }
}

// ======================================================================
// ELF symbol export checks (ported from xDL macros)
// ======================================================================

static inline bool dynsym_is_export(ElfW(Half) shndx) {
  return SHN_UNDEF != shndx;
}

static inline bool symtab_is_export(ElfW(Half) shndx) {
  return SHN_UNDEF != shndx &&
         !(shndx >= SHN_LORESERVE && shndx <= SHN_HIRESERVE);
}

// ======================================================================
// hash functions (ported from xDL)
// ======================================================================

static uint32_t elf_sysv_hash(const char *name) {
  uint32_t h = 0, g;
  const uint8_t *p = (const uint8_t *)name;
  while (*p) {
    h = (h << 4) + *p++;
    g = h & 0xf0000000;
    h ^= g;
    h ^= g >> 24;
  }
  return h;
}

static uint32_t elf_gnu_hash(const char *name) {
  uint32_t h = 5381;
  const uint8_t *p = (const uint8_t *)name;
  while (*p) {
    h += (h << 5) + *p++;
  }
  return h;
}

// ======================================================================
// .dynsym lookup from memory (O(1) via GNU hash / SysV hash)
// Ported from xDL xdl_dynsym_load + xdl_dynsym_find_symbol_use_*_hash.
// Reads PT_DYNAMIC segment, locates .dynsym/.dynstr/.hash/.gnu.hash.
// ======================================================================

static void *dynsym_lookup_from_memory(uintptr_t load_bias, const ElfW(Phdr) *phdr,
                                       ElfW(Half) phnum, const char *symbol) {
  // find PT_DYNAMIC segment
  ElfW(Dyn) *dynamic = NULL;
  for (size_t i = 0; i < phnum; i++) {
    if (phdr[i].p_type == PT_DYNAMIC) {
      dynamic = (ElfW(Dyn) *)(load_bias + phdr[i].p_vaddr);
      break;
    }
  }
  if (NULL == dynamic) return NULL;

  // iterate dynamic entries to locate .dynsym, .dynstr, .hash, .gnu.hash
  const ElfW(Sym) *dynsym = NULL;
  const char *dynstr = NULL;

  // SysV hash (.hash)
  const uint32_t *sysv_buckets = NULL;
  uint32_t sysv_buckets_cnt = 0;
  const uint32_t *sysv_chains = NULL;
  uint32_t sysv_chains_cnt = 0;

  // GNU hash (.gnu.hash)
  const uint32_t *gnu_buckets = NULL;
  uint32_t gnu_buckets_cnt = 0;
  const uint32_t *gnu_chains = NULL;
  uint32_t gnu_symoffset = 0;
  const ElfW(Addr) *gnu_bloom = NULL;
  uint32_t gnu_bloom_cnt = 0;
  uint32_t gnu_bloom_shift = 0;

  for (ElfW(Dyn) *entry = dynamic; entry->d_tag != DT_NULL; entry++) {
    switch (entry->d_tag) {
      case DT_SYMTAB:
        dynsym = (const ElfW(Sym) *)(load_bias + entry->d_un.d_ptr);
        break;
      case DT_STRTAB:
        dynstr = (const char *)(load_bias + entry->d_un.d_ptr);
        break;
      case DT_HASH: {
        const uint32_t *hash = (const uint32_t *)(load_bias + entry->d_un.d_ptr);
        sysv_buckets_cnt = hash[0];
        sysv_chains_cnt = hash[1];
        sysv_buckets = &hash[2];
        sysv_chains = &sysv_buckets[sysv_buckets_cnt];
        break;
      }
      case DT_GNU_HASH: {
        const uint32_t *ghash = (const uint32_t *)(load_bias + entry->d_un.d_ptr);
        gnu_buckets_cnt = ghash[0];
        gnu_symoffset = ghash[1];
        gnu_bloom_cnt = ghash[2];
        gnu_bloom_shift = ghash[3];
        gnu_bloom = (const ElfW(Addr) *)&ghash[4];
        gnu_buckets = (const uint32_t *)&gnu_bloom[gnu_bloom_cnt];
        gnu_chains = &gnu_buckets[gnu_buckets_cnt];
        break;
      }
      default:
        break;
    }
  }

  if (NULL == dynsym || NULL == dynstr) return NULL;
  if (0 == sysv_buckets_cnt && 0 == gnu_buckets_cnt) return NULL;

  // try GNU hash first (bloom filter + bucket/chain), O(1) expected
  if (gnu_buckets_cnt > 0) {
    uint32_t hash = elf_gnu_hash(symbol);
    uint32_t elfclass_bits = sizeof(ElfW(Addr)) * 8;

    size_t word = gnu_bloom[(hash / elfclass_bits) % gnu_bloom_cnt];
    size_t mask = (size_t)1 << (hash % elfclass_bits) |
                  (size_t)1 << ((hash >> gnu_bloom_shift) % elfclass_bits);

    // if at least one bit is not set, this symbol is surely missing
    if ((word & mask) == mask) {
      uint32_t i = gnu_buckets[hash % gnu_buckets_cnt];
      if (i >= gnu_symoffset) {
        while (1) {
          const ElfW(Sym) *sym = &dynsym[i];
          uint32_t sym_hash = gnu_chains[i - gnu_symoffset];

          if ((hash | (uint32_t)1) == (sym_hash | (uint32_t)1)) {
            if (0 == strcmp(dynstr + sym->st_name, symbol)) {
              if (dynsym_is_export(sym->st_shndx))
                return (void *)(load_bias + sym->st_value);
            }
          }

          // chain ends with an element whose lowest bit is 1
          if (sym_hash & (uint32_t)1) break;
          i++;
        }
      }
    }
  }

  // fallback: SysV hash, O(1) expected
  if (sysv_buckets_cnt > 0) {
    uint32_t hash = elf_sysv_hash(symbol);
    for (uint32_t i = sysv_buckets[hash % sysv_buckets_cnt]; 0 != i;
         i = sysv_chains[i]) {
      const ElfW(Sym) *sym = &dynsym[i];
      if (0 == strcmp(dynstr + sym->st_name, symbol)) {
        if (dynsym_is_export(sym->st_shndx))
          return (void *)(load_bias + sym->st_value);
      }
    }
  }

  return NULL;
}

// ======================================================================
// .symtab lookup from disk (linear scan)
// With .gnu_debugdata LZMA fallback for stripped binaries.
// Ported from xDL xdl_symtab_load + xdl_symtab_load_from_debugdata.
// ======================================================================

// linear scan a symtab/strtab pair for a named symbol
static void *scan_symtab(ElfW(Sym) *symtab, size_t count, const char *strtab,
                         const char *symbol, uintptr_t load_bias) {
  for (size_t i = 0; i < count; i++) {
    ElfW(Sym) *sym = &symtab[i];
    if (!symtab_is_export(sym->st_shndx)) continue;
    if (0 == strcmp(strtab + sym->st_name, symbol))
      return (void *)(load_bias + sym->st_value);
  }
  return NULL;
}

// search .symtab inside a decompressed .gnu_debugdata mini-ELF
static void *symtab_lookup_from_debugdata(uint8_t *debugdata, size_t debugdata_sz,
                                          uintptr_t load_bias, const char *symbol) {
  ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *)debugdata;
  if (0 == ehdr->e_shnum || ehdr->e_shentsize != sizeof(ElfW(Shdr))) return NULL;

  ElfW(Shdr) *shdrs = (ElfW(Shdr) *)(debugdata + ehdr->e_shoff);
  if (SHN_UNDEF == ehdr->e_shstrndx || ehdr->e_shstrndx >= ehdr->e_shnum) return NULL;
  char *shstrtab = (char *)(debugdata + shdrs[ehdr->e_shstrndx].sh_offset);

  for (size_t i = 0; i < ehdr->e_shnum; i++) {
    ElfW(Shdr) *shdr = &shdrs[i];
    const char *name = shstrtab + shdr->sh_name;

    if (SHT_SYMTAB == shdr->sh_type && 0 == strcmp(".symtab", name)) {
      if (shdr->sh_link >= ehdr->e_shnum) continue;
      ElfW(Shdr) *strtab_shdr = &shdrs[shdr->sh_link];
      if (SHT_STRTAB != strtab_shdr->sh_type) continue;

      ElfW(Sym) *symtab = (ElfW(Sym) *)(debugdata + shdr->sh_offset);
      size_t count = shdr->sh_size / shdr->sh_entsize;
      char *strtab = (char *)(debugdata + strtab_shdr->sh_offset);

      return scan_symtab(symtab, count, strtab, symbol, load_bias);
    }
  }
  return NULL;
}

static void *symtab_lookup_from_disk(const char *pathname, uintptr_t load_bias,
                                     const char *symbol) {
  if (NULL == pathname || '[' == pathname[0]) return NULL;  // skip [vdso] etc.

  uint8_t *file_mem = NULL;
  size_t file_sz = 0;
  file_mmap(pathname, &file_mem, &file_sz);
  if (NULL == file_mem) return NULL;

  void *result = NULL;
  ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *)file_mem;
  if (0 == ehdr->e_shnum || ehdr->e_shentsize != sizeof(ElfW(Shdr))) goto end;

  {
    ElfW(Shdr) *shdrs = (ElfW(Shdr) *)(file_mem + ehdr->e_shoff);
    if (SHN_UNDEF == ehdr->e_shstrndx || ehdr->e_shstrndx >= ehdr->e_shnum) goto end;
    char *shstrtab = (char *)(file_mem + shdrs[ehdr->e_shstrndx].sh_offset);

    // iterate sections: try .symtab first, fall back to .gnu_debugdata
    for (size_t i = 0; i < ehdr->e_shnum; i++) {
      ElfW(Shdr) *shdr = &shdrs[i];
      const char *name = shstrtab + shdr->sh_name;

      if (SHT_SYMTAB == shdr->sh_type && 0 == strcmp(".symtab", name)) {
        if (shdr->sh_link >= ehdr->e_shnum) continue;
        ElfW(Shdr) *strtab_shdr = &shdrs[shdr->sh_link];
        if (SHT_STRTAB != strtab_shdr->sh_type) continue;

        ElfW(Sym) *symtab = (ElfW(Sym) *)(file_mem + shdr->sh_offset);
        size_t count = shdr->sh_size / shdr->sh_entsize;
        char *strtab = (char *)(file_mem + strtab_shdr->sh_offset);

        result = scan_symtab(symtab, count, strtab, symbol, load_bias);
        if (result) goto end;
      } else if (SHT_PROGBITS == shdr->sh_type && 0 == strcmp(".gnu_debugdata", name)) {
        // LZMA-compressed mini-ELF containing .symtab
        uint8_t *debugdata = NULL;
        size_t debugdata_sz = 0;
        if (0 != dobby_lzma_decompress(file_mem + shdr->sh_offset, shdr->sh_size,
                                       &debugdata, &debugdata_sz))
          continue;

        result = symtab_lookup_from_debugdata(debugdata, debugdata_sz, load_bias, symbol);
        free(debugdata);
        if (result) goto end;
      }
    }
  }

end:
  if (file_mem) file_unmap(file_mem, file_sz);
  return result;
}

// ======================================================================
// per-module resolve: dynsym (memory, O(1)) + symtab (disk, linear)
// ======================================================================

static void *resolve_in_module(const RuntimeModule &module, const char *symbol) {
  if (NULL == module.load_address) return NULL;

  uintptr_t base = (uintptr_t)module.load_address;
  ElfW(Ehdr) *ehdr = (ElfW(Ehdr) *)base;

  // verify ELF magic
  if (0 != memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) return NULL;

  const ElfW(Phdr) *phdr = (const ElfW(Phdr) *)(base + ehdr->e_phoff);
  ElfW(Half) phnum = ehdr->e_phnum;

  // compute load_bias = base - min_vaddr (from PT_LOAD segments)
  uintptr_t min_vaddr = UINTPTR_MAX;
  for (size_t i = 0; i < phnum; i++) {
    if (PT_LOAD == phdr[i].p_type && min_vaddr > phdr[i].p_vaddr)
      min_vaddr = phdr[i].p_vaddr;
  }
  if (UINTPTR_MAX == min_vaddr) return NULL;
  uintptr_t load_bias = base - min_vaddr;

  // 1. .dynsym from memory — O(1) GNU/SysV hash lookup
  void *result = dynsym_lookup_from_memory(load_bias, phdr, phnum, symbol);
  if (result) return result;

  // 2. .symtab from disk — linear scan with .gnu_debugdata fallback
  result = symtab_lookup_from_disk(module.path, load_bias, symbol);
  return result;
}

// ======================================================================
// public API
// ======================================================================

PUBLIC void *DobbySymbolResolver(const char *image_name, const char *symbol_name_pattern) {
  if (NULL == symbol_name_pattern) return NULL;

  // When image_name is specified, resolve ONLY in matching module(s) — do NOT
  // dlsym first, because dlsym(RTLD_DEFAULT) may return the symbol from a
  // different module than the one requested. This matches xdl_sym() semantics,
  // which resolves only within the opened handle.
  if (image_name) {
    RuntimeModule module = ProcessRuntimeUtility::GetProcessModule(image_name);
    void *result = resolve_in_module(module, symbol_name_pattern);
    if (result) return result;
    return NULL;
  }

  // image_name == NULL: dlsym fast path (exported symbols), then search all
  // loaded modules for hidden / non-exported symbols.
  void *result = dlsym(RTLD_DEFAULT, symbol_name_pattern);
  if (result) return result;

  auto ProcessModuleMap = ProcessRuntimeUtility::GetProcessModuleMap();
  for (auto module : ProcessModuleMap) {
    result = resolve_in_module(module, symbol_name_pattern);
    if (result) return result;
  }

  return NULL;
}
