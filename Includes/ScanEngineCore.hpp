#pragma once

#include "ScanEngine.hpp"

namespace ScanEngine
{

template <typename T>
inline bool ParseIntegral(const char *text, ParsedValue &out)
{
    if (text == nullptr || *text == '\0') return false;
    errno = 0;
    char *end = nullptr;
    if constexpr (std::is_signed_v<T>)
    {
        long long value = std::strtoll(text, &end, 0);
        while (end != nullptr && *end == ' ') ++end;
        if (errno != 0 || end == text || (end != nullptr && *end != '\0')) return false;
        if (value < static_cast<long long>(std::numeric_limits<T>::min()) || value > static_cast<long long>(std::numeric_limits<T>::max())) return false;
        T converted = static_cast<T>(value);
        out.size = sizeof(T);
        std::memcpy(out.bytes.data(), &converted, sizeof(T));
    }
    else
    {
        if (*text == '-') return false;
        unsigned long long value = std::strtoull(text, &end, 0);
        while (end != nullptr && *end == ' ') ++end;
        if (errno != 0 || end == text || (end != nullptr && *end != '\0') || value > static_cast<unsigned long long>(std::numeric_limits<T>::max())) return false;
        T converted = static_cast<T>(value);
        out.size = sizeof(T);
        std::memcpy(out.bytes.data(), &converted, sizeof(T));
    }
    return true;
}

inline bool ParseValue(ValueType type, const char *text, ParsedValue &out)
{
    out.bytes.fill(0);
    switch (type)
    {
        case ValueType::BYTE: return ParseIntegral<guint8>(text, out);
        case ValueType::WORD: return ParseIntegral<guint16>(text, out);
        case ValueType::DWORD: return ParseIntegral<guint32>(text, out);
        case ValueType::QWORD: return ParseIntegral<guint64>(text, out);
        case ValueType::FLOAT:
        {
            if (text == nullptr || *text == '\0') return false;
            errno = 0;
            char *end = nullptr;
            float value = std::strtof(text, &end);
            while (end != nullptr && *end == ' ') ++end;
            if (errno != 0 || end == text || (end != nullptr && *end != '\0') || !std::isfinite(value)) return false;
            out.size = sizeof(value);
            std::memcpy(out.bytes.data(), &value, sizeof(value));
            return true;
        }
        case ValueType::DOUBLE:
        {
            if (text == nullptr || *text == '\0') return false;
            errno = 0;
            char *end = nullptr;
            double value = std::strtod(text, &end);
            while (end != nullptr && *end == ' ') ++end;
            if (errno != 0 || end == text || (end != nullptr && *end != '\0') || !std::isfinite(value)) return false;
            out.size = sizeof(value);
            std::memcpy(out.bytes.data(), &value, sizeof(value));
            return true;
        }
    }
    return false;
}

inline bool SafeRead(GumAddress address, void *output, gsize size)
{
    try
    {
        if (output == nullptr || size == 0) return false;
        GumPageProtection protection = GUM_PAGE_NO_ACCESS;
        if (!gum_memory_query_protection(reinterpret_cast<gconstpointer>(address), &protection) || (protection & GUM_PAGE_READ) == 0) return false;
        gsize readSize = 0;
        guint8 *data = gum_memory_read(reinterpret_cast<gconstpointer>(address), size, &readSize);
        if (data == nullptr) return false;
        bool valid = readSize == size;
        if (valid) std::memcpy(output, data, size);
        g_free(data);
        return valid;
    }
    catch (...)
    {
        return false;
    }
}

inline bool SafeWrite(GumAddress address, const void *data, gsize size)
{
    try
    {
        if (data == nullptr || size == 0) return false;
        GumPageProtection protection = GUM_PAGE_NO_ACCESS;
        if (!gum_memory_query_protection(reinterpret_cast<gconstpointer>(address), &protection)) return false;
        if ((protection & GUM_PAGE_WRITE) != 0) return gum_memory_write(reinterpret_cast<gpointer>(address), static_cast<const guint8 *>(data), size) != FALSE;
        if ((protection & GUM_PAGE_READ) == 0) return false;
        guint pageSize = gum_query_page_size();
        GumAddress pageStart = address & ~static_cast<GumAddress>(pageSize - 1);
        GumAddress pageEnd = (address + size + pageSize - 1) & ~static_cast<GumAddress>(pageSize - 1);
        gsize span = static_cast<gsize>(pageEnd - pageStart);
        if (!gum_try_mprotect(reinterpret_cast<gpointer>(pageStart), span, static_cast<GumPageProtection>(protection | GUM_PAGE_WRITE))) return false;
        bool written = gum_memory_write(reinterpret_cast<gpointer>(address), static_cast<const guint8 *>(data), size) != FALSE;
        gum_try_mprotect(reinterpret_cast<gpointer>(pageStart), span, protection);
        return written;
    }
    catch (...)
    {
        return false;
    }
}

inline std::string FormatValue(ValueType type, const guint8 *bytes)
{
    char output[96]{};
    switch (type)
    {
        case ValueType::BYTE:
        {
            guint8 value = 0;
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%u", static_cast<unsigned int>(value));
            break;
        }
        case ValueType::WORD:
        {
            guint16 value = 0;
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%u", static_cast<unsigned int>(value));
            break;
        }
        case ValueType::DWORD:
        {
            guint32 value = 0;
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%u", value);
            break;
        }
        case ValueType::QWORD:
        {
            guint64 value = 0;
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%llu", static_cast<unsigned long long>(value));
            break;
        }
        case ValueType::FLOAT:
        {
            float value = 0;
            std::memcpy(&value, bytes, sizeof(value));
            std::snprintf(output, sizeof(output), "%.9g", static_cast<double>(value));
            break;
        }
        case ValueType::DOUBLE:
        {
            double value = 0;
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
    for (gsize i = 0; i < value.size; ++i)
    {
        if (i != 0) pattern.push_back(' ');
        std::snprintf(byteText, sizeof(byteText), "%02X", value.bytes[i]);
        pattern += byteText;
    }
    return pattern;
}

struct MatchContext
{
    std::vector<Result> *results;
    ValueType type;
    RegionType region;
    size_t limit;
};

inline gboolean OnMatch(GumAddress address, gsize, gpointer userData)
{
    MatchContext *context = static_cast<MatchContext *>(userData);
    gsize alignment = ValueTypeSize(context->type);
    if ((address % alignment) != 0) return TRUE;
    if (context->results->size() >= context->limit) return FALSE;
    Result result{};
    result.address = address;
    result.type = context->type;
    result.region = context->region;
    if (SafeRead(address, result.bytes.data(), ValueTypeSize(context->type))) context->results->push_back(result);
    return context->results->size() < context->limit ? TRUE : FALSE;
}

inline bool RegionSelected(RegionType selected, RegionType actual)
{
    if (selected == RegionType::ALL) return actual != RegionType::BAD;
    return selected == actual;
}

inline void SetStatus(State &state, const char *text)
{
    std::lock_guard<std::mutex> lock(state.statusMutex);
    std::snprintf(state.status, sizeof(state.status), "%s", text);
}

inline std::string GetStatus(State &state)
{
    std::lock_guard<std::mutex> lock(state.statusMutex);
    return state.status;
}

inline void *RunSearch(void *argument)
{
    SearchTask *task = static_cast<SearchTask *>(argument);
    State &state = *task->state;
    try
    {
        std::vector<Result> output;
        if (task->refine)
        {
            std::vector<Result> previous;
            {
                std::lock_guard<std::mutex> lock(state.mutex);
                previous = state.results;
            }
            output.reserve(previous.size());
            for (const Result &result : previous)
            {
                std::array<guint8, 8> current{};
                if (SafeRead(result.address, current.data(), task->value.size) && std::memcmp(current.data(), task->value.bytes.data(), task->value.size) == 0)
                {
                    Result kept = result;
                    kept.bytes = current;
                    output.push_back(kept);
                }
                state.scanned.fetch_add(1, std::memory_order_relaxed);
            }
        }
        else
        {
            std::vector<MapEntry> maps = ReadMaps();
            std::string patternText = PatternFromValue(task->value);
            GumMatchPattern *pattern = gum_match_pattern_new_from_string(patternText.c_str());
            if (pattern == nullptr) throw std::runtime_error("Invalid Frida pattern");
            for (const MapEntry &entry : maps)
            {
                if (!RegionSelected(task->region, entry.region) || (entry.protection & GUM_PAGE_READ) == 0) continue;
                if (entry.end - entry.start < task->value.size) continue;
                if (output.size() >= ResultLimit) break;
                GumMemoryRange range{entry.start, static_cast<gsize>(entry.end - entry.start)};
                MatchContext context{&output, task->type, entry.region, ResultLimit};
                gum_memory_scan(&range, pattern, OnMatch, &context);
                state.scanned.fetch_add(1, std::memory_order_relaxed);
            }
            gum_match_pattern_unref(pattern);
        }
        state.matched.store(output.size(), std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.results.swap(output);
            state.resultType = task->type;
            state.resultRegion = task->region;
        }
        if (state.matched.load() >= ResultLimit) SetStatus(state, "Result limit reached");
        else SetStatus(state, task->refine ? "Refine completed" : "Search completed");
    }
    catch (const std::exception &error)
    {
        SetStatus(state, error.what());
    }
    catch (...)
    {
        SetStatus(state, "Search failed");
    }
    state.busy.store(false, std::memory_order_release);
    state.completed.store(true, std::memory_order_release);
    delete task;
    return nullptr;
}

inline bool StartSearch(bool refine)
{
    State &state = GetState();
    if (state.busy.load(std::memory_order_acquire)) return false;
    ParsedValue value{};
    if (!ParseValue(state.type, state.value, value))
    {
        SetStatus(state, "Invalid value");
        return false;
    }
    if (refine)
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.results.empty())
        {
            SetStatus(state, "No results to refine");
            return false;
        }
        if (state.type != state.resultType)
        {
            SetStatus(state, "Refine type must match first search");
            return false;
        }
    }
    state.scanned.store(0, std::memory_order_relaxed);
    state.matched.store(0, std::memory_order_relaxed);
    state.completed.store(false, std::memory_order_relaxed);
    state.busy.store(true, std::memory_order_release);
    SetStatus(state, refine ? "Refining previous results" : "Scanning selected region");
    SearchTask *task = new SearchTask{&state, value, state.type, state.region, refine};
    pthread_t thread{};
    if (pthread_create(&thread, nullptr, RunSearch, task) != 0)
    {
        delete task;
        state.busy.store(false, std::memory_order_release);
        SetStatus(state, "Failed to start search thread");
        return false;
    }
    pthread_detach(thread);
    return true;
}

inline bool WriteResult(size_t index, const char *text)
{
    State &state = GetState();
    ParsedValue value{};
    Result selected{};
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (index >= state.results.size()) return false;
        selected = state.results[index];
    }
    if (!ParseValue(selected.type, text, value) || !SafeWrite(selected.address, value.bytes.data(), value.size)) return false;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (index < state.results.size() && state.results[index].address == selected.address) state.results[index].bytes = value.bytes;
    }
    return true;
}

}
