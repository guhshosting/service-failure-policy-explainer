#pragma once
#include "policy.hpp"
#include <cstring>
namespace sfp {
struct Buffer {
    std::vector<unsigned char> bytes;
    bool contains(const void *p, size_t n) const {
        auto base = reinterpret_cast<uintptr_t>(bytes.data()),
             addr = reinterpret_cast<uintptr_t>(p);
        return p && addr >= base && addr - base <= bytes.size() &&
               n <= bytes.size() - (addr - base);
    }
    template <class T> T read(const T *p) const {
        if (!contains(p, sizeof(T)))
            throw std::runtime_error("Malformed SCM structure");
        T t{};
        std::memcpy(&t, p, sizeof(T));
        return t;
    }
    std::optional<std::wstring> string(const wchar_t *p) const {
        if (!p)
            return std::nullopt;
        std::wstring s;
        if (!contains(p, sizeof(wchar_t)))
            throw std::runtime_error("Malformed SCM string");
        auto offset = reinterpret_cast<uintptr_t>(p) - reinterpret_cast<uintptr_t>(bytes.data());
        while (offset <= bytes.size() && sizeof(wchar_t) <= bytes.size() - offset) {
            wchar_t c{};
            std::memcpy(&c, bytes.data() + offset, sizeof(c));
            if (!c)
                return s;
            s += c;
            offset += sizeof(c);
        }
        throw std::runtime_error("Unterminated SCM string");
    }
};
// Pure decoding: all pointer-bearing data must reside in this owned query buffer.
void decode_failure_actions(const Buffer &buffer, Result &result);
} // namespace sfp
