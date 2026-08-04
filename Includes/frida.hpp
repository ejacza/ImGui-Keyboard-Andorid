#ifndef FRIDA_HPP_INCLUDED
#define FRIDA_HPP_INCLUDED

#include "frida-gum.h"
#include "gumpp.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <pthread.h>
#include <unistd.h>
#include "il2cpp/log.h"

namespace frida
{

using Address = GumAddress;

static inline void ensure (void)
{
  static bool inited = false;
  if (!inited)
  {
    gum_init_embedded ();
    inited = true;
  }
}

struct Region
{
  Address base;
  gsize size;
  GumPageProtection prot;
};

enum RegionType
{
  REGION_ALL = 0,
  REGION_JAVA_HEAP,
  REGION_C_HEAP,
  REGION_C_ALLOC,
  REGION_C_DATA,
  REGION_C_BSS,
  REGION_PPSSPP,
  REGION_ANONYMOUS,
  REGION_JAVA,
  REGION_STACK,
  REGION_ASHMEM,
  REGION_VIDEO,
  REGION_OTHER,
  REGION_BAD,
  REGION_CODE_APP,
  REGION_CODE_SYS
};

static inline const char * region_type_name (RegionType type)
{
  switch (type)
  {
    case REGION_ALL: return "ALL";
    case REGION_JAVA_HEAP: return "JAVA_HEAP";
    case REGION_C_HEAP: return "C_HEAP";
    case REGION_C_ALLOC: return "C_ALLOC";
    case REGION_C_DATA: return "C_DATA";
    case REGION_C_BSS: return "C_BSS";
    case REGION_PPSSPP: return "PPSSPP";
    case REGION_ANONYMOUS: return "ANONYMOUS";
    case REGION_JAVA: return "JAVA";
    case REGION_STACK: return "STACK";
    case REGION_ASHMEM: return "ASHMEM";
    case REGION_VIDEO: return "VIDEO";
    case REGION_OTHER: return "OTHER";
    case REGION_BAD: return "BAD";
    case REGION_CODE_APP: return "CODE_APP";
    case REGION_CODE_SYS: return "CODE_SYS";
  }
  return "ALL";
}

static inline RegionType classify_region (const char * name, GumPageProtection prot)
{
  if (prot == GUM_PAGE_NO_ACCESS)
    return REGION_BAD;
  if (name[0] == '[' || name[0] == '\0')
  {
    if (std::strcmp (name, "[heap]") == 0)
      return REGION_C_HEAP;
    if (std::strncmp (name, "[stack", 6) == 0)
      return REGION_STACK;
    if (std::strstr (name, "dalvik") != nullptr ||
        std::strstr (name, "art-") != nullptr)
      return REGION_JAVA_HEAP;
    if (std::strstr (name, "scudo") != nullptr ||
        std::strstr (name, "libc_malloc") != nullptr)
      return REGION_C_ALLOC;
    if (std::strstr (name, "ashmem") != nullptr)
      return REGION_ASHMEM;
    if (std::strstr (name, ".bss") != nullptr)
      return REGION_C_BSS;
    if (std::strstr (name, "stack_and_tls") != nullptr)
      return REGION_STACK;
    if (prot & GUM_PAGE_EXECUTE)
      return REGION_CODE_APP;
    return REGION_ANONYMOUS;
  }
  if (std::strstr (name, "ppsspp") != nullptr)
    return REGION_PPSSPP;
  if (std::strstr (name, "gralloc") != nullptr ||
      std::strstr (name, "ion") != nullptr ||
      std::strstr (name, "kgsl") != nullptr ||
      std::strstr (name, "dma_buf") != nullptr ||
      std::strstr (name, "hwcomposer") != nullptr ||
      std::strstr (name, "videocodec") != nullptr ||
      std::strstr (name, "/dev/video") != nullptr)
    return REGION_VIDEO;
  if (std::strstr (name, "ashmem") != nullptr)
    return REGION_ASHMEM;
  if (std::strstr (name, ".bss") != nullptr)
    return REGION_C_BSS;
  if (std::strstr (name, ".dex") != nullptr ||
      std::strstr (name, ".odex") != nullptr ||
      std::strstr (name, ".oat") != nullptr ||
      std::strstr (name, ".vdex") != nullptr ||
      std::strstr (name, ".art") != nullptr)
    return REGION_JAVA;
  if (prot & GUM_PAGE_EXECUTE)
    return (std::strstr (name, "/data/") != nullptr)
        ? REGION_CODE_APP : REGION_CODE_SYS;
  if (std::strstr (name, "/data/") != nullptr)
    return REGION_C_DATA;
  return REGION_OTHER;
}

static inline std::vector<Region> map_regions (RegionType only = REGION_ALL)
{
  std::vector<Region> out;
  FILE * f = std::fopen ("/proc/self/maps", "r");
  if (f == nullptr)
    return out;
  char line[512];
  while (std::fgets (line, sizeof (line), f) != nullptr)
  {
    unsigned long long start = 0, end = 0;
    char perms[5] = {0, 0, 0, 0, 0};
    unsigned long long offset = 0;
    char dev[16] = "";
    unsigned long inode = 0;
    char name[256] = "";
    int pos = 0;
    if (std::sscanf (line, "%llx-%llx %4s %llx %15s %lu %n",
        &start, &end, perms, &offset, dev, &inode, &pos) < 6)
      continue;
    std::snprintf (name, sizeof (name), "%s", line + pos);
    size_t nl = std::strlen (name);
    while (nl > 0 && (name[nl - 1] == '\n' || name[nl - 1] == '\r'))
      name[--nl] = '\0';
    GumPageProtection prot = GUM_PAGE_NO_ACCESS;
    if (perms[0] == 'r') prot = static_cast<GumPageProtection> (prot | GUM_PAGE_READ);
    if (perms[1] == 'w') prot = static_cast<GumPageProtection> (prot | GUM_PAGE_WRITE);
    if (perms[2] == 'x') prot = static_cast<GumPageProtection> (prot | GUM_PAGE_EXECUTE);
    RegionType t = classify_region (name, prot);
    if (only != REGION_ALL && t != only)
      continue;
    out.push_back ({ static_cast<Address> (start),
        static_cast<gsize> (end - start), prot });
  }
  std::fclose (f);
  return out;
}

static inline std::vector<Region> regions (GumPageProtection prot = GUM_PAGE_READ)
{
  ensure ();
  std::vector<Region> out;
  gum_process_enumerate_ranges (prot,
      +[] (const GumRangeDetails * details, gpointer user_data) -> gboolean
      {
        std::vector<Region> * v = static_cast<std::vector<Region> *> (user_data);
        v->push_back ({ details->range->base_address, details->range->size,
            details->protection });
        return TRUE;
      }, &out);
  return out;
}

static inline std::vector<Address> find_bytes_in (Address base, gsize size,
    const void * bytes, gsize len)
{
  ensure ();
  std::vector<Address> out;
  if (len == 0 || size < len)
    return out;
  const guint8 * pat = static_cast<const guint8 *> (bytes);
  const gsize chunk = 0x1000;
  gsize off = 0;
  while (off < size)
  {
    gsize want = chunk;
    if (off + want > size)
      want = size - off;
    gsize got = 0;
    guint8 * data = gum_memory_read (reinterpret_cast<gconstpointer> (base + off),
        want, &got);
    if (data == nullptr)
    {
      off += chunk;
      continue;
    }
    gsize scan = got;
    if (off + want < size)
    {
      gsize got2 = 0;
      guint8 * extra = gum_memory_read (reinterpret_cast<gconstpointer> (
          base + off + want), len - 1, &got2);
      if (extra != nullptr)
      {
        std::memcpy (data + want, extra, got2);
        scan += got2;
        g_free (extra);
      }
    }
    for (gsize i = 0; i + len <= scan; i++)
    {
      if (std::memcmp (data + i, pat, len) == 0)
        out.push_back (base + off + i);
    }
    g_free (data);
    off += chunk;
  }
  return out;
}

static inline std::string bytes_to_pattern (const guint8 * data, gsize len)
{
  std::string out;
  char buf[4];
  for (gsize i = 0; i < len; i++)
  {
    if (i > 0)
      out += ' ';
    std::snprintf (buf, sizeof (buf), "%02X", data[i]);
    out += buf;
  }
  return out;
}

struct ScanMatchData
{
  std::vector<Address> hits;
};

static inline gboolean scan_match_cb (GumAddress address, gsize size, gpointer user_data)
{
  static_cast<ScanMatchData *> (user_data)->hits.push_back (address);
  return TRUE;
}

static inline std::vector<Address> find_bytes (const void * bytes, gsize len,
    GumPageProtection prot = GUM_PAGE_READ)
{
  ensure ();
  std::vector<Address> out;
  std::string pattern = bytes_to_pattern (static_cast<const guint8 *> (bytes), len);
  GumMatchPattern * pat = gum_match_pattern_new_from_string (pattern.c_str ());
  if (pat == nullptr)
    return out;
  ScanMatchData data;
  for (const Region & r : regions (prot))
  {
    GumMemoryRange range = { r.base, r.size };
    gum_memory_scan (&range, pat, scan_match_cb, &data);
  }
  gum_match_pattern_unref (pat);
  out.swap (data.hits);
  return out;
}

template <class T>
static inline T read (Address address)
{
  T value = T ();
  gsize n = 0;
  guint8 * data = gum_memory_read (reinterpret_cast<gconstpointer> (address), sizeof (T), &n);
  if (data != nullptr)
  {
    if (n == sizeof (T))
      std::memcpy (&value, data, n);
    g_free (data);
  }
  return value;
}

static inline bool write (Address address, const void * bytes, gsize size)
{
  return gum_memory_write (reinterpret_cast<gpointer> (address),
      static_cast<const guint8 *> (bytes), size) != FALSE;
}

template <class T>
static inline bool write (Address address, const T & value)
{
  return write (address, &value, sizeof (T));
}

static inline guint8 read_byte (Address address) { return read<guint8> (address); }
static inline gint32 read_int (Address address) { return read<gint32> (address); }
static inline guint32 read_dword (Address address) { return read<guint32> (address); }
static inline guint64 read_qword (Address address) { return read<guint64> (address); }
static inline float read_float (Address address) { return read<float> (address); }
static inline double read_double (Address address) { return read<double> (address); }

static inline bool write_byte (Address address, guint8 value) { return write (address, value); }
static inline bool write_int (Address address, gint32 value) { return write (address, value); }
static inline bool write_dword (Address address, guint32 value) { return write (address, value); }
static inline bool write_qword (Address address, guint64 value) { return write (address, value); }
static inline bool write_float (Address address, float value) { return write (address, value); }
static inline bool write_double (Address address, double value) { return write (address, value); }

enum ScanType
{
  SCAN_BYTE = 0,
  SCAN_INT,
  SCAN_DWORD,
  SCAN_QWORD,
  SCAN_FLOAT,
  SCAN_DOUBLE
};

static inline gsize scan_type_size (ScanType type)
{
  switch (type)
  {
    case SCAN_BYTE: return 1;
    case SCAN_INT:
    case SCAN_DWORD:
    case SCAN_FLOAT: return 4;
    case SCAN_QWORD:
    case SCAN_DOUBLE: return 8;
  }
  return 4;
}

static inline const char * scan_type_name (ScanType type)
{
  switch (type)
  {
    case SCAN_BYTE: return "Byte";
    case SCAN_INT: return "Int";
    case SCAN_DWORD: return "Dword";
    case SCAN_QWORD: return "Qword";
    case SCAN_FLOAT: return "Float";
    case SCAN_DOUBLE: return "Double";
  }
  return "Dword";
}

static inline bool parse_value (ScanType type, const char * text, guint8 * out)
{
  if (text == nullptr || text[0] == '\0')
    return false;
  char * end = nullptr;
  switch (type)
  {
    case SCAN_BYTE:
    {
      unsigned long v = std::strtoul (text, &end, 0);
      if (end == text)
        return false;
      guint8 x = static_cast<guint8> (v);
      std::memcpy (out, &x, 1);
      return true;
    }
    case SCAN_INT:
    {
      long v = std::strtol (text, &end, 0);
      if (end == text)
        return false;
      gint32 x = static_cast<gint32> (v);
      std::memcpy (out, &x, 4);
      return true;
    }
    case SCAN_DWORD:
    {
      unsigned long v = std::strtoul (text, &end, 0);
      if (end == text)
        return false;
      guint32 x = static_cast<guint32> (v);
      std::memcpy (out, &x, 4);
      return true;
    }
    case SCAN_QWORD:
    {
      unsigned long long v = std::strtoull (text, &end, 0);
      if (end == text)
        return false;
      guint64 x = static_cast<guint64> (v);
      std::memcpy (out, &x, 8);
      return true;
    }
    case SCAN_FLOAT:
    {
      float v = std::strtof (text, &end);
      if (end == text)
        return false;
      std::memcpy (out, &v, 4);
      return true;
    }
    case SCAN_DOUBLE:
    {
      double v = std::strtod (text, &end);
      if (end == text)
        return false;
      std::memcpy (out, &v, 8);
      return true;
    }
  }
  return false;
}

static inline bool write_value (ScanType type, Address address, const char * text)
{
  guint8 raw[8];
  if (!parse_value (type, text, raw))
    return false;
  return write (address, raw, scan_type_size (type));
}

static inline bool read_raw (Address address, guint8 * out, gsize len)
{
  try
  {
    gsize got = 0;
    guint8 * data = gum_memory_read (reinterpret_cast<gconstpointer> (address), len, &got);
    if (data == nullptr || got != len)
    {
      if (data != nullptr)
        g_free (data);
      LOGE("read_raw failed: addr=0x%llx len=%zu", (unsigned long long)address, len);
      return false;
    }
    std::memcpy (out, data, len);
    g_free (data);
    return true;
  }
  catch (const std::exception & e)
  {
    LOGE("read_raw exception: %s", e.what());
    return false;
  }
  catch (...)
  {
    LOGE("read_raw unknown exception");
    return false;
  }
}

struct FreezeEntry
{
  ScanType type;
  guint8 bytes[8];
};

struct ScanResult
{
  Address address;
  guint8 prev[8];
};

struct FreezeData
{
  pthread_mutex_t * mutex;
  std::vector<std::pair<Address, FreezeEntry>> * frozen;
  volatile bool * running;
};

struct ScanContext
{
  ScanType type;
  guint8 search_bytes[8];
  RegionType region_type;
  std::vector<ScanResult> results;
  std::vector<ScanResult> prev_results;
  bool done;
};

static void * scan_thread_fn (void * arg)
{
  ScanContext * ctx = static_cast<ScanContext *> (arg);
  try
  {
    ensure ();
    gsize len = scan_type_size (ctx->type);

    if (ctx->prev_results.empty ())
    {
      std::vector<Address> hits;
      if (ctx->region_type == REGION_ALL)
      {
        hits = find_bytes (ctx->search_bytes, len);
      }
      else
      {
        for (const Region & r : map_regions (ctx->region_type))
        {
          std::vector<Address> h = find_bytes_in (r.base, r.size, ctx->search_bytes, len);
          hits.insert (hits.end (), h.begin (), h.end ());
        }
      }
      ctx->results.clear ();
      ctx->results.reserve (hits.size ());
      for (Address a : hits)
      {
        ScanResult sr;
        sr.address = a;
        read_raw (a, sr.prev, len);
        ctx->results.push_back (sr);
      }
    }
    else
    {
      std::vector<ScanResult> kept;
      kept.reserve (ctx->prev_results.size ());
      for (const ScanResult & sr : ctx->prev_results)
      {
        guint8 cur[8] = {};
        if (!read_raw (sr.address, cur, len))
          continue;
        if (std::memcmp (cur, ctx->search_bytes, len) == 0)
        {
          ScanResult nsr;
          nsr.address = sr.address;
          std::memcpy (nsr.prev, cur, len);
          kept.push_back (nsr);
        }
      }
      ctx->results.swap (kept);
    }
    ctx->done = true;
  }
  catch (const std::exception & e)
  {
    LOGE("scan_thread_fn exception: %s", e.what());
    ctx->done = true;
  }
  catch (...)
  {
    LOGE("scan_thread_fn unknown exception");
    ctx->done = true;
  }
  return nullptr;
}

static void * freeze_thread_fn (void * arg)
{
  FreezeData * fd = static_cast<FreezeData *> (arg);
  try
  {
    while (*fd->running)
    {
      pthread_mutex_lock (fd->mutex);
      for (auto & pair : *fd->frozen)
      {
        Address addr = pair.first;
        const FreezeEntry & fe = pair.second;
        gsize len = scan_type_size (fe.type);
        write (addr, fe.bytes, len);
      }
      pthread_mutex_unlock (fd->mutex);
      usleep (100000);
    }
  }
  catch (const std::exception & e)
  {
    LOGE("freeze_thread_fn exception: %s", e.what());
  }
  catch (...)
  {
    LOGE("freeze_thread_fn unknown exception");
  }
  delete fd;
  return nullptr;
}

class Scanner
{
public:
  std::vector<ScanResult> results;
  ScanType type = SCAN_DWORD;
  RegionType region_type = REGION_ALL;
  volatile bool is_scanning = false;
  std::vector<std::pair<Address, FreezeEntry>> frozen;
  pthread_mutex_t freeze_mutex;
  volatile bool freeze_running = false;
  pthread_t freeze_thread;

  Scanner ()
  {
    pthread_mutex_init (&freeze_mutex, nullptr);
  }

  ~Scanner ()
  {
    stop_freeze ();
    pthread_mutex_destroy (&freeze_mutex);
  }

  void start_freeze ()
  {
    if (freeze_running)
      return;
    freeze_running = true;
    auto * fd = new FreezeData ();
    fd->mutex = &freeze_mutex;
    fd->frozen = &frozen;
    fd->running = &freeze_running;
    pthread_create (&freeze_thread, nullptr, freeze_thread_fn, fd);
  }

  void stop_freeze ()
  {
    if (!freeze_running)
      return;
    freeze_running = false;
    pthread_join (freeze_thread, nullptr);
  }

  void toggle_freeze (Address address)
  {
    pthread_mutex_lock (&freeze_mutex);
    bool found = false;
    for (auto it = frozen.begin (); it != frozen.end (); ++it)
    {
      if (it->first == address)
      {
        frozen.erase (it);
        found = true;
        break;
      }
    }
    if (!found)
    {
      FreezeEntry fe;
      fe.type = type;
      gsize len = scan_type_size (type);
      read_raw (address, fe.bytes, len);
      frozen.push_back ({ address, fe });
    }
    pthread_mutex_unlock (&freeze_mutex);
    if (!frozen.empty ())
      start_freeze ();
    else
      stop_freeze ();
  }

  bool is_frozen (Address address) const
  {
    for (auto & p : frozen)
      if (p.first == address)
        return true;
    return false;
  }

  void reset ()
  {
    results.clear ();
    stop_freeze ();
    frozen.clear ();
  }

  void set_region (RegionType rt)
  {
    region_type = rt;
  }

  void first (ScanType t, const char * text)
  {
    if (is_scanning)
      return;
    type = t;
    guint8 raw[8];
    if (!parse_value (t, text, raw))
      return;
    ScanContext * ctx = new ScanContext ();
    ctx->type = type;
    ctx->region_type = region_type;
    std::memcpy (ctx->search_bytes, raw, 8);
    is_scanning = true;
    pthread_t tid;
    pthread_create (&tid, nullptr, scan_thread_fn, ctx);
    pthread_detach (tid);
    pending_ctx = ctx;
  }

  void next (const char * text)
  {
    if (is_scanning)
      return;
    if (results.empty ())
      return;
    guint8 raw[8];
    if (!parse_value (type, text, raw))
      return;
    ScanContext * ctx = new ScanContext ();
    ctx->type = type;
    ctx->region_type = region_type;
    std::memcpy (ctx->search_bytes, raw, 8);
    ctx->prev_results = results;
    is_scanning = true;
    pthread_t tid;
    pthread_create (&tid, nullptr, scan_thread_fn, ctx);
    pthread_detach (tid);
    pending_ctx = ctx;
  }

  bool poll ()
  {
    if (is_scanning && pending_ctx != nullptr && pending_ctx->done)
    {
      results.swap (pending_ctx->results);
      delete pending_ctx;
      pending_ctx = nullptr;
      is_scanning = false;
      return true;
    }
    return false;
  }

  double read_as (Address a) const
  {
    switch (type)
    {
      case SCAN_BYTE: return static_cast<double> (read_byte (a));
      case SCAN_INT: return static_cast<double> (read_int (a));
      case SCAN_DWORD: return static_cast<double> (read_dword (a));
      case SCAN_QWORD: return static_cast<double> (read_qword (a));
      case SCAN_FLOAT: return static_cast<double> (read_float (a));
      case SCAN_DOUBLE: return read_double (a);
    }
    return 0.0;
  }

private:
  ScanContext * pending_ctx = nullptr;
};

struct PointerPath
{
  Address base;
  std::vector<gsize> offsets;
};

static inline std::vector<PointerPath> find_pointers_to (Address target, int max_depth = 3)
{
  ensure ();
  std::vector<PointerPath> paths;
  if (max_depth < 1)
    return paths;
  
  std::vector<Address> direct;
  guint8 target_bytes[8];
  std::memcpy (target_bytes, &target, sizeof (Address));
  direct = find_bytes (target_bytes, sizeof (Address));
  
  for (Address ptr : direct)
  {
    PointerPath p;
    p.base = ptr;
    paths.push_back (p);
  }
  
  if (max_depth > 1)
  {
    for (Address ptr : direct)
    {
      auto nested = find_pointers_to (ptr, max_depth - 1);
      for (auto & n : nested)
      {
        n.offsets.insert (n.offsets.begin (), static_cast<gsize> (target - ptr));
        paths.push_back (n);
      }
    }
  }
  
  return paths;
}

static inline Address resolve_pointer_path (const PointerPath & path)
{
  Address cur = path.base;
  for (gsize off : path.offsets)
  {
    Address next = 0;
    if (!read_raw (cur, reinterpret_cast<guint8 *> (&next), sizeof (Address)))
      return 0;
    cur = next + off;
  }
  return cur;
}

}

#endif