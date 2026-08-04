#pragma once

#include <frida-gum.h>
#include <gumpp.hpp>
#include <android/log.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <limits>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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
    GumAddress start{};
    GumAddress end{};
    GumPageProtection protection{GUM_PAGE_NO_ACCESS};
    bool shared{};
    guint64 offset{};
    unsigned int devMajor{};
    unsigned int devMinor{};
    guint64 inode{};
    std::string path;
    RegionType region{RegionType::OTHER};
};

struct Result
{
    GumAddress address{};
    ValueType type{ValueType::DWORD};
    RegionType region{RegionType::OTHER};
    std::array<guint8, 8> bytes{};
};

struct ParsedValue
{
    std::array<guint8, 8> bytes{};
    gsize size{};
};

struct Snapshot
{
    std::vector<Result> results;
    size_t generation{};
};

struct State
{
    std::vector<Result> results;
    std::mutex resultsMutex;
    std::mutex statusMutex;
    std::mutex operationMutex;
    std::atomic<bool> busy{false};
    std::atomic<bool> cancelRequested{false};
    std::atomic<size_t> processed{0};
    std::atomic<size_t> matched{0};
    std::atomic<size_t> unreadable{0};
    std::atomic<size_t> generation{0};
    ValueType resultType{ValueType::DWORD};
    RegionType resultRegion{RegionType::ALL};
    std::string status{"Ready"};
};

struct SearchTask
{
    State *state{};
    ParsedValue value{};
    ValueType type{ValueType::DWORD};
    RegionType region{RegionType::ALL};
    bool refine{};
    size_t expectedGeneration{};
};

struct MatchContext
{
    std::vector<Result> *results{};
    State *state{};
    ValueType type{ValueType::DWORD};
    RegionType region{RegionType::OTHER};
    GumAddress scanBase{};
    GumAddress sourceBase{};
};

inline constexpr size_t ResultLimit = 250000;
inline constexpr size_t DisplayLimit = 10000;

inline void Log(int priority, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    __android_log_vprint(priority, "ScanEngine", format, args);
    va_end(args);
}

inline void SetStatus(State &state, const std::string &status)
{
    {
        std::lock_guard<std::mutex> lock(state.statusMutex);
        state.status = status;
    }
    Log(ANDROID_LOG_INFO, "%s", status.c_str());
}

inline std::string GetStatus(State &state)
{
    std::lock_guard<std::mutex> lock(state.statusMutex);
    return state.status;
}

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
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
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
    size_t size = std::strlen(suffix);
    return value.size() >= size && value.compare(value.size() - size, size, suffix) == 0;
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
    std::string path = Lower(entry.path);
    bool readable = (entry.protection & GUM_PAGE_READ) != 0;
    bool writable = (entry.protection & GUM_PAGE_WRITE) != 0;
    bool executable = (entry.protection & GUM_PAGE_EXECUTE) != 0;
    bool anonymous = entry.inode == 0 && (entry.path.empty() || StartsWith(path, "[anon:"));
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

inline bool ReadMaps(std::vector<MapEntry> &maps, std::string &error, bool emitLog = true)
{
    maps.clear();
    FILE *file = std::fopen("/proc/self/maps", "re");
    if (file == nullptr) file = std::fopen("/proc/self/maps", "r");
    if (file == nullptr)
    {
        error = std::string("Unable to open /proc/self/maps: ") + std::strerror(errno);
        if (emitLog) Log(ANDROID_LOG_ERROR, "%s", error.c_str());
        return false;
    }
    char *line = nullptr;
    size_t capacity = 0;
    ssize_t length = 0;
    while ((length = getline(&line, &capacity, file)) >= 0)
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
        if (pathOffset > 0 && pathOffset < length) path.assign(line + pathOffset, static_cast<size_t>(length - pathOffset));
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
    if (line != nullptr) std::free(line);
    bool failed = std::ferror(file) != 0;
    std::fclose(file);
    if (failed)
    {
        error = "Failed while reading /proc/self/maps";
        if (emitLog) Log(ANDROID_LOG_ERROR, "%s", error.c_str());
        return false;
    }
    if (emitLog) Log(ANDROID_LOG_INFO, "Loaded %zu memory mappings", maps.size());
    return true;
}

template <typename T>
inline bool ParseUnsigned(const char *text, ParsedValue &output)
{
    if (text == nullptr) return false;
    while (std::isspace(static_cast<unsigned char>(*text))) ++text;
    if (*text == '\0' || *text == '-') return false;
    errno = 0;
    char *end = nullptr;
    unsigned long long value = std::strtoull(text, &end, 0);
    while (end != nullptr && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (errno != 0 || end == text || end == nullptr || *end != '\0' || value > static_cast<unsigned long long>(std::numeric_limits<T>::max())) return false;
    T converted = static_cast<T>(value);
    output.bytes.fill(0);
    output.size = sizeof(T);
    std::memcpy(output.bytes.data(), &converted, sizeof(T));
    return true;
}

inline bool ParseValue(ValueType type, const char *text, ParsedValue &output)
{
    output = {};
    switch (type)
    {
        case ValueType::BYTE: return ParseUnsigned<guint8>(text, output);
        case ValueType::WORD: return ParseUnsigned<guint16>(text, output);
        case ValueType::DWORD: return ParseUnsigned<guint32>(text, output);
        case ValueType::QWORD: return ParseUnsigned<guint64>(text, output);
        case ValueType::FLOAT:
        {
            if (text == nullptr) return false;
            while (std::isspace(static_cast<unsigned char>(*text))) ++text;
            errno = 0;
            char *end = nullptr;
            float value = std::strtof(text, &end);
            while (end != nullptr && std::isspace(static_cast<unsigned char>(*end))) ++end;
            if (errno != 0 || end == text || end == nullptr || *end != '\0' || !std::isfinite(value)) return false;
            output.size = sizeof(value);
            std::memcpy(output.bytes.data(), &value, sizeof(value));
            return true;
        }
        case ValueType::DOUBLE:
        {
            if (text == nullptr) return false;
            while (std::isspace(static_cast<unsigned char>(*text))) ++text;
            errno = 0;
            char *end = nullptr;
            double value = std::strtod(text, &end);
            while (end != nullptr && std::isspace(static_cast<unsigned char>(*end))) ++end;
            if (errno != 0 || end == text || end == nullptr || *end != '\0' || !std::isfinite(value)) return false;
            output.size = sizeof(value);
            std::memcpy(output.bytes.data(), &value, sizeof(value));
            return true;
        }
    }
    return false;
}

inline bool RangeContains(const MapEntry &entry, GumAddress address, gsize size)
{
    if (address < entry.start || address >= entry.end) return false;
    if (size > entry.end - address) return false;
    return true;
}

inline bool LookupRange(GumAddress address, gsize size, MapEntry &entry, bool emitLog = true)
{
    std::vector<MapEntry> maps;
    std::string error;
    if (!ReadMaps(maps, error, emitLog)) return false;
    auto iterator = std::upper_bound(maps.begin(), maps.end(), address, [](GumAddress value, const MapEntry &candidate) { return value < candidate.start; });
    if (iterator == maps.begin()) return false;
    --iterator;
    if (!RangeContains(*iterator, address, size)) return false;
    entry = *iterator;
    return true;
}

inline bool SafeRead(GumAddress address, void *output, gsize size, bool quiet = false)
{
    if (output == nullptr || size == 0) return false;
    try
    {
        GumPageProtection protection = GUM_PAGE_NO_ACCESS;
        if (!gum_memory_query_protection(reinterpret_cast<gconstpointer>(address), &protection) || (protection & GUM_PAGE_READ) == 0)
        {
            if (!quiet) Log(ANDROID_LOG_WARN, "Read rejected address=0x%llx size=%zu", static_cast<unsigned long long>(address), size);
            return false;
        }
        gsize bytesRead = 0;
        guint8 *data = gum_memory_read(reinterpret_cast<gconstpointer>(address), size, &bytesRead);
        if (data == nullptr || bytesRead != size)
        {
            if (data != nullptr) g_free(data);
            if (!quiet) Log(ANDROID_LOG_WARN, "Read failed address=0x%llx size=%zu read=%zu", static_cast<unsigned long long>(address), size, bytesRead);
            return false;
        }
        std::memcpy(output, data, size);
        g_free(data);
        return true;
    }
    catch (const std::exception &error)
    {
        if (!quiet) Log(ANDROID_LOG_ERROR, "Read exception address=0x%llx error=%s", static_cast<unsigned long long>(address), error.what());
        return false;
    }
    catch (...)
    {
        if (!quiet) Log(ANDROID_LOG_ERROR, "Read exception address=0x%llx", static_cast<unsigned long long>(address));
        return false;
    }
}

inline bool SafeWrite(GumAddress address, const void *data, gsize size)
{
    if (data == nullptr || size == 0) return false;
    try
    {
        MapEntry entry;
        if (!LookupRange(address, size, entry) || (entry.protection & GUM_PAGE_READ) == 0)
        {
            Log(ANDROID_LOG_ERROR, "Write rejected address=0x%llx size=%zu", static_cast<unsigned long long>(address), size);
            return false;
        }
        bool changedProtection = (entry.protection & GUM_PAGE_WRITE) == 0;
        guint pageSize = gum_query_page_size();
        if (pageSize == 0 || (pageSize & (pageSize - 1)) != 0) return false;
        GumAddress pageStart = address & ~static_cast<GumAddress>(pageSize - 1);
        GumAddress finalAddress = address + size - 1;
        if (finalAddress < address) return false;
        GumAddress pageEnd = (finalAddress & ~static_cast<GumAddress>(pageSize - 1)) + pageSize;
        gsize span = static_cast<gsize>(pageEnd - pageStart);
        if (changedProtection && !gum_try_mprotect(reinterpret_cast<gpointer>(pageStart), span, static_cast<GumPageProtection>(entry.protection | GUM_PAGE_WRITE)))
        {
            Log(ANDROID_LOG_ERROR, "mprotect failed address=0x%llx span=%zu", static_cast<unsigned long long>(pageStart), span);
            return false;
        }
        bool written = gum_memory_write(reinterpret_cast<gpointer>(address), static_cast<const guint8 *>(data), size) != FALSE;
        bool restored = !changedProtection || gum_try_mprotect(reinterpret_cast<gpointer>(pageStart), span, entry.protection);
        if (!restored) Log(ANDROID_LOG_ERROR, "Protection restore failed address=0x%llx span=%zu", static_cast<unsigned long long>(pageStart), span);
        if (!written)
        {
            Log(ANDROID_LOG_ERROR, "Write failed address=0x%llx size=%zu", static_cast<unsigned long long>(address), size);
            return false;
        }
        Log(ANDROID_LOG_INFO, "Write success address=0x%llx size=%zu type=%s region=%s", static_cast<unsigned long long>(address), size, "RAW", RegionTypeName(entry.region));
        return restored;
    }
    catch (const std::exception &error)
    {
        Log(ANDROID_LOG_ERROR, "Write exception address=0x%llx error=%s", static_cast<unsigned long long>(address), error.what());
        return false;
    }
    catch (...)
    {
        Log(ANDROID_LOG_ERROR, "Write exception address=0x%llx", static_cast<unsigned long long>(address));
        return false;
    }
}

inline std::string FormatValue(ValueType type, const guint8 *bytes)
{
    if (bytes == nullptr) return {};
    char output[96]{};
    switch (type)
    {
        case ValueType::BYTE:
        {
            guint8 value{};
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%u", static_cast<unsigned int>(value));
            break;
        }
        case ValueType::WORD:
        {
            guint16 value{};
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%u", static_cast<unsigned int>(value));
            break;
        }
        case ValueType::DWORD:
        {
            guint32 value{};
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%u", value);
            break;
        }
        case ValueType::QWORD:
        {
            guint64 value{};
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%llu", static_cast<unsigned long long>(value));
            break;
        }
        case ValueType::FLOAT:
        {
            float value{};
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%.9g", static_cast<double>(value));
            break;
        }
        case ValueType::DOUBLE:
        {
            double value{};
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%.17g", value);
            break;
        }
    }
    return output;
}

inline std::string PatternFromValue(const ParsedValue &value)
{
    std::string pattern;
    char byteText[4]{};
    for (gsize index = 0; index < value.size; ++index)
    {
        if (index != 0) pattern.push_back(' ');
        std::snprintf(byteText, sizeof(byteText), "%02X", value.bytes[index]);
        pattern += byteText;
    }
    return pattern;
}

inline bool RegionSelected(RegionType selected, RegionType actual)
{
    if (selected == RegionType::ALL) return actual != RegionType::BAD;
    return selected == actual;
}

inline gboolean MatchFound(GumAddress address, gsize, gpointer userData)
{
    MatchContext *context = static_cast<MatchContext *>(userData);
    if (context == nullptr || context->results == nullptr || context->state == nullptr) return FALSE;
    if (context->state->cancelRequested.load(std::memory_order_relaxed)) return FALSE;
    if (context->results->size() >= ResultLimit) return FALSE;
    gsize size = ValueTypeSize(context->type);
    GumAddress sourceAddress = context->sourceBase + (address - context->scanBase);
    if (size == 0 || sourceAddress % size != 0) return TRUE;
    Result result{};
    result.address = sourceAddress;
    result.type = context->type;
    result.region = context->region;
    std::memcpy(result.bytes.data(), reinterpret_cast<gconstpointer>(address), size);
    context->results->push_back(result);
    context->state->matched.store(context->results->size(), std::memory_order_relaxed);
    return context->results->size() < ResultLimit ? TRUE : FALSE;
}

inline void Publish(State &state, std::vector<Result> &&results, ValueType type, RegionType region)
{
    size_t count = results.size();
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        state.results = std::move(results);
        state.resultType = type;
        state.resultRegion = region;
        state.generation.fetch_add(1, std::memory_order_release);
    }
    state.matched.store(count, std::memory_order_relaxed);
}

inline void *SearchThread(void *argument)
{
    std::unique_ptr<SearchTask> task(static_cast<SearchTask *>(argument));
    if (!task || task->state == nullptr) return nullptr;
    State &state = *task->state;
    std::unique_lock<std::mutex> operationLock(state.operationMutex);
    std::vector<Result> output;
    bool success = false;
    try
    {
        if (task->refine)
        {
            std::vector<Result> previous;
            {
                std::lock_guard<std::mutex> lock(state.resultsMutex);
                if (task->expectedGeneration != state.generation.load(std::memory_order_acquire)) throw std::runtime_error("Result generation changed");
                previous = state.results;
            }
            output.reserve(previous.size());
            for (const Result &result : previous)
            {
                if (state.cancelRequested.load(std::memory_order_relaxed)) break;
                std::array<guint8, 8> current{};
                if (SafeRead(result.address, current.data(), task->value.size, true))
                {
                    if (std::memcmp(current.data(), task->value.bytes.data(), task->value.size) == 0)
                    {
                        Result kept = result;
                        kept.bytes = current;
                        output.push_back(kept);
                    }
                }
                else
                {
                    state.unreadable.fetch_add(1, std::memory_order_relaxed);
                }
                state.processed.fetch_add(1, std::memory_order_relaxed);
            }
            success = !state.cancelRequested.load(std::memory_order_relaxed);
        }
        else
        {
            std::vector<MapEntry> maps;
            std::string mapsError;
            if (!ReadMaps(maps, mapsError)) throw std::runtime_error(mapsError);
            std::string patternText = PatternFromValue(task->value);
            GumMatchPattern *rawPattern = gum_match_pattern_new_from_string(patternText.c_str());
            if (rawPattern == nullptr) throw std::runtime_error("Frida rejected scan pattern");
            std::unique_ptr<GumMatchPattern, void (*)(GumMatchPattern *)> pattern(rawPattern, gum_match_pattern_unref);
            for (const MapEntry &entry : maps)
            {
                if (state.cancelRequested.load(std::memory_order_relaxed) || output.size() >= ResultLimit) break;
                if (!RegionSelected(task->region, entry.region) || (entry.protection & GUM_PAGE_READ) == 0 || entry.end - entry.start < task->value.size) continue;
                GumPageProtection currentProtection = GUM_PAGE_NO_ACCESS;
                if (!gum_memory_query_protection(reinterpret_cast<gconstpointer>(entry.start), &currentProtection) || (currentProtection & GUM_PAGE_READ) == 0)
                {
                    state.unreadable.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }
                constexpr gsize chunkSize = 8 * 1024 * 1024;
                gsize patternSize = task->value.size;
                GumAddress cursor = entry.start;
                while (cursor < entry.end && !state.cancelRequested.load(std::memory_order_relaxed) && output.size() < ResultLimit)
                {
                    gsize remaining = static_cast<gsize>(entry.end - cursor);
                    gsize request = std::min(chunkSize, remaining);
                    if (remaining > request && request <= std::numeric_limits<gsize>::max() - (patternSize - 1)) request += patternSize - 1;
                    gsize bytesRead = 0;
                    guint8 *data = gum_memory_read(reinterpret_cast<gconstpointer>(cursor), request, &bytesRead);
                    if (data == nullptr || bytesRead < patternSize)
                    {
                        if (data != nullptr) g_free(data);
                        state.unreadable.fetch_add(1, std::memory_order_relaxed);
                        cursor += std::min(chunkSize, remaining);
                        continue;
                    }
                    GumMemoryRange range{reinterpret_cast<GumAddress>(data), bytesRead};
                    MatchContext context{&output, &state, task->type, entry.region, range.base_address, cursor};
                    gum_memory_scan(&range, pattern.get(), MatchFound, &context);
                    g_free(data);
                    cursor += std::min(chunkSize, remaining);
                    state.processed.fetch_add(1, std::memory_order_relaxed);
                }
            }
            success = !state.cancelRequested.load(std::memory_order_relaxed);
        }
        if (success)
        {
            Publish(state, std::move(output), task->type, task->region);
            size_t count = state.matched.load(std::memory_order_relaxed);
            if (count >= ResultLimit) SetStatus(state, "Completed at result limit");
            else SetStatus(state, task->refine ? "Refine completed" : "Search completed");
            Log(ANDROID_LOG_INFO, "%s complete type=%s region=%s results=%zu processed=%zu unreadable=%zu", task->refine ? "Refine" : "Search", ValueTypeName(task->type), RegionTypeName(task->region), count, state.processed.load(), state.unreadable.load());
        }
        else
        {
            SetStatus(state, "Operation cancelled");
            Log(ANDROID_LOG_WARN, "%s cancelled processed=%zu matches=%zu", task->refine ? "Refine" : "Search", state.processed.load(), state.matched.load());
        }
    }
    catch (const std::exception &error)
    {
        SetStatus(state, std::string("Failed: ") + error.what());
        Log(ANDROID_LOG_ERROR, "%s failed type=%s region=%s error=%s", task->refine ? "Refine" : "Search", ValueTypeName(task->type), RegionTypeName(task->region), error.what());
    }
    catch (...)
    {
        SetStatus(state, "Failed: unknown exception");
        Log(ANDROID_LOG_ERROR, "%s failed with unknown exception", task->refine ? "Refine" : "Search");
    }
    state.busy.store(false, std::memory_order_release);
    return nullptr;
}


inline bool Start(ValueType type, RegionType region, const char *text, bool refine)
{
    State &state = GetState();
    bool expected = false;
    if (!state.busy.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
    {
        SetStatus(state, "Operation already running");
        return false;
    }
    ParsedValue value;
    if (!ParseValue(type, text, value))
    {
        state.busy.store(false, std::memory_order_release);
        SetStatus(state, "Invalid value");
        Log(ANDROID_LOG_ERROR, "Invalid value type=%s text=%s", ValueTypeName(type), text == nullptr ? "<null>" : text);
        return false;
    }
    size_t generation = state.generation.load(std::memory_order_acquire);
    if (refine)
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        if (state.results.empty())
        {
            state.busy.store(false, std::memory_order_release);
            SetStatus(state, "No results to refine");
            return false;
        }
        if (type != state.resultType)
        {
            state.busy.store(false, std::memory_order_release);
            SetStatus(state, "Refine type must match first search");
            return false;
        }
    }
    state.cancelRequested.store(false, std::memory_order_relaxed);
    state.processed.store(0, std::memory_order_relaxed);
    state.matched.store(0, std::memory_order_relaxed);
    state.unreadable.store(0, std::memory_order_relaxed);
    std::unique_ptr<SearchTask> task(new SearchTask{&state, value, type, region, refine, generation});
    pthread_t thread{};
    int result = pthread_create(&thread, nullptr, SearchThread, task.get());
    if (result != 0)
    {
        state.busy.store(false, std::memory_order_release);
        SetStatus(state, std::string("Thread creation failed: ") + std::strerror(result));
        Log(ANDROID_LOG_ERROR, "Thread creation failed error=%d", result);
        return false;
    }
    task.release();
    pthread_detach(thread);
    SetStatus(state, refine ? "Refining previous results" : "Scanning selected regions");
    Log(ANDROID_LOG_INFO, "%s started type=%s region=%s value=%s generation=%zu", refine ? "Refine" : "Search", ValueTypeName(type), RegionTypeName(region), text, generation);
    return true;
}

inline bool Search(ValueType type, RegionType region, const char *text)
{
    return Start(type, region, text, false);
}

inline bool Refine(ValueType type, const char *text)
{
    State &state = GetState();
    return Start(type, state.resultRegion, text, true);
}

inline void Cancel()
{
    State &state = GetState();
    if (state.busy.load(std::memory_order_acquire))
    {
        state.cancelRequested.store(true, std::memory_order_release);
        SetStatus(state, "Cancellation requested");
    }
}

inline void Clear()
{
    State &state = GetState();
    if (state.busy.load(std::memory_order_acquire)) return;
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        state.results.clear();
        state.results.shrink_to_fit();
        state.generation.fetch_add(1, std::memory_order_release);
    }
    state.matched.store(0, std::memory_order_relaxed);
    SetStatus(state, "Results cleared");
    Log(ANDROID_LOG_INFO, "Results cleared");
}

inline Snapshot GetSnapshot(size_t limit = DisplayLimit)
{
    State &state = GetState();
    Snapshot snapshot;
    std::lock_guard<std::mutex> lock(state.resultsMutex);
    snapshot.generation = state.generation.load(std::memory_order_relaxed);
    size_t count = std::min(limit, state.results.size());
    snapshot.results.assign(state.results.begin(), state.results.begin() + count);
    return snapshot;
}

inline size_t GetResultCount()
{
    State &state = GetState();
    std::lock_guard<std::mutex> lock(state.resultsMutex);
    return state.results.size();
}

inline bool RefreshResult(size_t index, Result &result)
{
    State &state = GetState();
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        if (index >= state.results.size()) return false;
        result = state.results[index];
    }
    std::array<guint8, 8> current{};
    if (!SafeRead(result.address, current.data(), ValueTypeSize(result.type), true)) return false;
    result.bytes = current;
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        if (index < state.results.size() && state.results[index].address == result.address) state.results[index].bytes = current;
    }
    return true;
}

inline bool WriteResult(size_t index, const char *text)
{
    State &state = GetState();
    Result result;
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        if (index >= state.results.size()) return false;
        result = state.results[index];
    }
    ParsedValue value;
    if (!ParseValue(result.type, text, value))
    {
        SetStatus(state, "Invalid edit value");
        return false;
    }
    if (!SafeWrite(result.address, value.bytes.data(), value.size))
    {
        SetStatus(state, "Write failed");
        return false;
    }
    std::array<guint8, 8> verify{};
    if (!SafeRead(result.address, verify.data(), value.size) || std::memcmp(verify.data(), value.bytes.data(), value.size) != 0)
    {
        SetStatus(state, "Write verification failed");
        Log(ANDROID_LOG_ERROR, "Write verification failed address=0x%llx", static_cast<unsigned long long>(result.address));
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(state.resultsMutex);
        if (index < state.results.size() && state.results[index].address == result.address) state.results[index].bytes = verify;
    }
    SetStatus(state, "Value written and verified");
    Log(ANDROID_LOG_INFO, "Value written address=0x%llx type=%s value=%s", static_cast<unsigned long long>(result.address), ValueTypeName(result.type), text);
    return true;
}

inline bool IsBusy()
{
    return GetState().busy.load(std::memory_order_acquire);
}

inline size_t GetProcessed()
{
    return GetState().processed.load(std::memory_order_relaxed);
}

inline size_t GetUnreadable()
{
    return GetState().unreadable.load(std::memory_order_relaxed);
}

}
