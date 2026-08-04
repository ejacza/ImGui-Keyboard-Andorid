#pragma once

#include <frida-gum.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <pthread.h>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <gumpp.hpp>

namespace ScanEngine
{

enum class ValueType
{
    DWORD,
    FLOAT,
    DOUBLE,
    WORD,
    BYTE,
    QWORD
};

enum class RegionType
{
    ALL,
    JAVA_HEAP,
    C_HEAP,
    C_ALLOC,
    C_DATA,
    C_BSS,
    PPSSPP,
    ANONYMOUS,
    JAVA,
    STACK,
    ASHMEM,
    VIDEO,
    OTHER,
    BAD,
    CODE_APP,
    CODE_SYS
};

struct MapEntry
{
    GumAddress start;
    GumAddress end;
    GumPageProtection protection;
    bool shared;
    guint64 offset;
    unsigned int devMajor;
    unsigned int devMinor;
    guint64 inode;
    std::string path;
    RegionType region;
};

struct Result
{
    GumAddress address;
    ValueType type;
    RegionType region;
    std::array<guint8, 8> bytes;
};

struct ParsedValue
{
    std::array<guint8, 8> bytes;
    gsize size;
};

struct State
{
    std::vector<Result> results;
    std::mutex mutex;
    std::mutex statusMutex;
    std::atomic<bool> busy{false};
    std::atomic<bool> completed{false};
    std::atomic<size_t> scanned{0};
    std::atomic<size_t> matched{0};
    ValueType type{ValueType::DWORD};
    RegionType region{RegionType::ALL};
    ValueType resultType{ValueType::DWORD};
    RegionType resultRegion{RegionType::ALL};
    char value[96]{};
    char editValue[96]{};
    char status[160]{"Ready"};
    int selectedResult{-1};
};

struct SearchTask
{
    State *state;
    ParsedValue value;
    ValueType type;
    RegionType region;
    bool refine;
};

inline constexpr size_t ResultLimit = 250000;
inline constexpr size_t VisibleResultLimit = 10000;

inline State &GetState()
{
    static State state;
    return state;
}

inline const char *ValueTypeName(ValueType type)
{
    switch (type)
    {
        case ValueType::DWORD: return "DWORD";
        case ValueType::FLOAT: return "FLOAT";
        case ValueType::DOUBLE: return "DOUBLE";
        case ValueType::WORD: return "WORD";
        case ValueType::BYTE: return "BYTE";
        case ValueType::QWORD: return "QWORD";
    }
    return "DWORD";
}

inline const char *RegionTypeName(RegionType type)
{
    switch (type)
    {
        case RegionType::ALL: return "ALL";
        case RegionType::JAVA_HEAP: return "JAVA_HEAP";
        case RegionType::C_HEAP: return "C_HEAP";
        case RegionType::C_ALLOC: return "C_ALLOC";
        case RegionType::C_DATA: return "C_DATA";
        case RegionType::C_BSS: return "C_BSS";
        case RegionType::PPSSPP: return "PPSSPP";
        case RegionType::ANONYMOUS: return "ANONYMOUS";
        case RegionType::JAVA: return "JAVA";
        case RegionType::STACK: return "STACK";
        case RegionType::ASHMEM: return "ASHMEM";
        case RegionType::VIDEO: return "VIDEO";
        case RegionType::OTHER: return "OTHER";
        case RegionType::BAD: return "BAD";
        case RegionType::CODE_APP: return "CODE_APP";
        case RegionType::CODE_SYS: return "CODE_SYS";
    }
    return "OTHER";
}

inline gsize ValueTypeSize(ValueType type)
{
    switch (type)
    {
        case ValueType::BYTE: return 1;
        case ValueType::WORD: return 2;
        case ValueType::DWORD:
        case ValueType::FLOAT: return 4;
        case ValueType::QWORD:
        case ValueType::DOUBLE: return 8;
    }
    return 4;
}

inline std::string Lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

inline bool Contains(const std::string &value, const char *needle)
{
    return value.find(needle) != std::string::npos;
}

inline bool StartsWith(const std::string &value, const char *prefix)
{
    return value.rfind(prefix, 0) == 0;
}

inline bool EndsWith(const std::string &value, const char *suffix)
{
    size_t n = std::strlen(suffix);
    return value.size() >= n && value.compare(value.size() - n, n, suffix) == 0;
}

inline bool IsAppPath(const std::string &path)
{
    return StartsWith(path, "/data/app/") || StartsWith(path, "/data/user/") || StartsWith(path, "/data/user_de/") || StartsWith(path, "/data/data/") || StartsWith(path, "/mnt/expand/") || Contains(path, "/android/data/") || Contains(path, "!/lib/");
}

inline bool IsSystemPath(const std::string &path)
{
    return StartsWith(path, "/system/") || StartsWith(path, "/system_ext/") || StartsWith(path, "/product/") || StartsWith(path, "/vendor/") || StartsWith(path, "/odm/") || StartsWith(path, "/apex/") || StartsWith(path, "/linkerconfig/");
}

inline bool IsJavaHeapName(const std::string &path)
{
    if (!Contains(path, "dalvik")) return false;
    return Contains(path, "main space") || Contains(path, "large object space") || Contains(path, "non moving space") || Contains(path, "non-moving space") || Contains(path, "zygote space") || Contains(path, "bump pointer space") || Contains(path, "region space") || Contains(path, "free list large object space") || Contains(path, "dalvik-heap");
}

inline bool IsJavaRuntimeName(const std::string &path)
{
    return EndsWith(path, ".dex") || EndsWith(path, ".odex") || EndsWith(path, ".vdex") || EndsWith(path, ".oat") || EndsWith(path, ".art") || EndsWith(path, ".jar") || Contains(path, "dalvik-jit") || Contains(path, "dalvik-linearalloc") || Contains(path, "dalvik-linear alloc") || Contains(path, "dalvik-indirect ref") || Contains(path, "dalvik-card table") || Contains(path, "dalvik-mark stack") || Contains(path, "dalvik-allocation stack") || Contains(path, "dalvik-mod-union") || StartsWith(path, "/data/dalvik-cache/");
}

inline bool IsVideoName(const std::string &path)
{
    return Contains(path, "/dev/kgsl") || Contains(path, "kgsl") || Contains(path, "/dev/ion") || Contains(path, "/dev/dma_heap/") || Contains(path, "dma_buf") || Contains(path, "dmabuf") || Contains(path, "gralloc") || Contains(path, "graphicbuffer") || Contains(path, "ahardwarebuffer") || Contains(path, "mali") || Contains(path, "adreno") || Contains(path, "hwcomposer") || Contains(path, "videocodec") || Contains(path, "/dev/video") || Contains(path, "v4l2");
}

inline RegionType Classify(const MapEntry &entry, const MapEntry *previous)
{
    const std::string path = Lower(entry.path);
    const bool readable = (entry.protection & GUM_PAGE_READ) != 0;
    const bool writable = (entry.protection & GUM_PAGE_WRITE) != 0;
    const bool executable = (entry.protection & GUM_PAGE_EXECUTE) != 0;
    const bool anonymous = entry.inode == 0 && (entry.path.empty() || StartsWith(path, "[anon:"));
    if (!readable || entry.start >= entry.end) return RegionType::BAD;
    if (Contains(path, "guard") || path == "[vectors]" || path == "[vsyscall]") return RegionType::BAD;
    if (path == "[stack]" || StartsWith(path, "[stack:") || Contains(path, "stack_and_tls") || Contains(path, "thread stack") || Contains(path, "signal stack")) return RegionType::STACK;
    if (IsVideoName(path)) return RegionType::VIDEO;
    if (IsJavaHeapName(path)) return RegionType::JAVA_HEAP;
    if (Contains(path, "ppsspp")) return RegionType::PPSSPP;
    if (path == "[heap]") return RegionType::C_HEAP;
    if (Contains(path, "scudo:") || Contains(path, "libc_malloc") || Contains(path, "jemalloc")) return RegionType::C_ALLOC;
    if (Contains(path, ".bss")) return RegionType::C_BSS;
    if (previous != nullptr && anonymous && writable && !executable && previous->end == entry.start && previous->inode != 0 && (previous->protection & GUM_PAGE_WRITE) != 0) return RegionType::C_BSS;
    if (IsJavaRuntimeName(path)) return RegionType::JAVA;
    if (Contains(path, "/dev/ashmem/") || StartsWith(path, "/memfd:") || StartsWith(path, "[anon_shmem:")) return RegionType::ASHMEM;
    if (executable && IsAppPath(path)) return RegionType::CODE_APP;
    if (executable && (IsSystemPath(path) || path == "[vdso]")) return RegionType::CODE_SYS;
    if (writable && !executable && entry.inode != 0 && IsAppPath(path)) return RegionType::C_DATA;
    if (anonymous) return RegionType::ANONYMOUS;
    return RegionType::OTHER;
}

inline std::vector<MapEntry> ReadMaps()
{
    std::vector<MapEntry> maps;
    FILE *file = std::fopen("/proc/self/maps", "r");
    if (file == nullptr) return maps;
    char line[2048];
    while (std::fgets(line, sizeof(line), file) != nullptr)
    {
        unsigned long long start = 0;
        unsigned long long end = 0;
        unsigned long long offset = 0;
        unsigned long long inode = 0;
        unsigned int major = 0;
        unsigned int minor = 0;
        char permissions[5]{};
        int pathOffset = 0;
        int count = std::sscanf(line, "%llx-%llx %4s %llx %x:%x %llu %n", &start, &end, permissions, &offset, &major, &minor, &inode, &pathOffset);
        if (count < 7 || start >= end) continue;
        std::string path;
        if (pathOffset > 0 && static_cast<size_t>(pathOffset) < std::strlen(line)) path.assign(line + pathOffset);
        while (!path.empty() && (path.back() == '\n' || path.back() == '\r')) path.pop_back();
        GumPageProtection protection = GUM_PAGE_NO_ACCESS;
        if (permissions[0] == 'r') protection = static_cast<GumPageProtection>(protection | GUM_PAGE_READ);
        if (permissions[1] == 'w') protection = static_cast<GumPageProtection>(protection | GUM_PAGE_WRITE);
        if (permissions[2] == 'x') protection = static_cast<GumPageProtection>(protection | GUM_PAGE_EXECUTE);
        MapEntry entry{static_cast<GumAddress>(start), static_cast<GumAddress>(end), protection, permissions[3] == 's', offset, major, minor, inode, path, RegionType::OTHER};
        const MapEntry *previous = maps.empty() ? nullptr : &maps.back();
        entry.region = Classify(entry, previous);
        maps.push_back(std::move(entry));
    }
    std::fclose(file);
    return maps;
}

}

#include "ScanEngineCore.hpp"
#include "ScanEngineUI.hpp"
