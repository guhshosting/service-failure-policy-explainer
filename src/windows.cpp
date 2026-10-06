#include "policy.hpp"
#include <algorithm>
namespace sfp {
ApiError::ApiError(const char *op, DWORD value)
    : std::runtime_error([&] {
          wchar_t text[512]{};
          FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, value,
                         0, text, 512, nullptr);
          return std::string(op) + ": Windows error " + std::to_string(value) + " " + utf8(text);
      }()),
      code(value) {}
void output(const std::string &s, bool error) {
    HANDLE h = GetStdHandle(error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
    DWORD mode = 0, written = 0;
    if (GetConsoleMode(h, &mode)) {
        int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
        std::wstring w(static_cast<size_t>(n), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
        size_t offset = 0;
        while (offset < w.size()) {
            DWORD chunk = static_cast<DWORD>((std::min)(w.size() - offset, size_t{16384}));
            if (!WriteConsoleW(h, w.data() + offset, chunk, &written, nullptr) || !written)
                throw ApiError("WriteConsoleW", GetLastError());
            offset += written;
        }
    } else {
        size_t offset = 0;
        while (offset < s.size()) {
            DWORD chunk = static_cast<DWORD>((std::min)(s.size() - offset, size_t{16384}));
            if (!WriteFile(h, s.data() + offset, chunk, &written, nullptr) || !written)
                throw ApiError("WriteFile", GetLastError());
            offset += written;
        }
    }
}
} // namespace sfp
