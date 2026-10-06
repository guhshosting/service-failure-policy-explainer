#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <windows.h>
namespace sfp {
inline constexpr const char *version = "1.0.0";
struct Action {
    uint32_t raw;
    uint32_t delay;
};
struct Result {
    std::wstring name, display, account;
    uint32_t start = 0, reset = 0;
    bool policy_available = true;
    std::optional<std::wstring> reboot, command;
    std::optional<bool> non_crash;
    std::vector<Action> actions;
    std::vector<std::string> warnings;
};
struct ApiError : std::runtime_error {
    DWORD code;
    ApiError(const char *operation, DWORD value);
};
struct Cli {
    bool json = false, help = false, version = false;
    std::wstring name;
};
Cli parse(const std::vector<std::wstring> &args);
Result inspect(const std::wstring &name);
std::string utf8(const std::wstring &value);
std::string action_type(uint32_t type);
std::string duration(uint64_t milliseconds);
std::string reset_meaning(uint32_t seconds);
std::vector<std::string> explanations(const Result &result);
std::vector<std::string> limitations();
std::string human(const Result &result);
std::string json(const Result &result);
std::string quote(const std::string &value);
void output(const std::string &value, bool error = false);
} // namespace sfp
