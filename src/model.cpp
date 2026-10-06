#include "policy.hpp"
#include <limits>
#include <sstream>
namespace sfp {
std::string utf8(const std::wstring &s) {
    if (s.empty())
        return {};
    if (s.size() > static_cast<size_t>(INT_MAX))
        throw std::runtime_error("String too large");
    int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0,
                                nullptr, nullptr);
    if (!n)
        throw ApiError("UTF-8 conversion", GetLastError());
    std::string r(static_cast<size_t>(n), '\0');
    if (!WideCharToMultiByte(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), r.data(), n, nullptr,
                             nullptr))
        throw ApiError("UTF-8 conversion", GetLastError());
    return r;
}
std::string action_type(uint32_t t) {
    switch (t) {
    case SC_ACTION_NONE:
        return "none";
    case SC_ACTION_RESTART:
        return "restart";
    case SC_ACTION_REBOOT:
        return "reboot";
    case SC_ACTION_RUN_COMMAND:
        return "run_command";
    default:
        return "unknown";
    }
}
std::string duration(uint64_t ms) {
    if (!ms)
        return "0 milliseconds";
    const uint64_t units[] = {86400000, 3600000, 60000, 1000, 1};
    const char *names[] = {"day", "hour", "minute", "second", "millisecond"};
    std::string r;
    for (size_t i = 0; i < 5; ++i) {
        auto n = ms / units[i];
        ms %= units[i];
        if (n) {
            if (!r.empty())
                r += ' ';
            r += std::to_string(n) + " " + names[i] + (n == 1 ? "" : "s");
        }
    }
    return r;
}
std::string reset_meaning(uint32_t s) {
    if (s == INFINITE)
        return "Never reset due to elapsed time (INFINITE)";
    if (!s)
        return "Zero-second failure-free interval; no positive reset interval configured";
    return "Reset after " + duration(static_cast<uint64_t>(s) * 1000) + " without failure";
}
std::vector<std::string> explanations(const Result &r) {
    std::vector<std::string> e;
    if (r.actions.empty())
        e.push_back(r.policy_available ? "No failure actions configured."
                                       : "Failure actions configuration unavailable.");
    else
        e.push_back("Failure N selects action N; failures beyond the list repeat the last action. "
                    "SCM counts failures since system boot, subject to the reset period.");
    e.push_back("Delays are configured milliseconds before the selected action; a counter reset "
                "does not restart the service.");
    if (!r.non_crash)
        e.push_back("Non-crash failure flag unavailable; its behavior cannot be determined.");
    else if (*r.non_crash)
        e.push_back("With actions configured, SCM queues them when the process terminates without "
                    "reporting SERVICE_STOPPED, or reports SERVICE_STOPPED with dwWin32ExitCode "
                    "other than ERROR_SUCCESS.");
    else
        e.push_back("With actions configured, SCM queues them only when the service terminates "
                    "without reporting SERVICE_STOPPED.");
    e.push_back("The non-crash flag is ignored without configured actions; changes to the flag "
                "take effect after the next system start.");
    return e;
}
std::vector<std::string> limitations() {
    std::vector<std::string> items;
    items.emplace_back(
        "Configuration is not evidence that any recovery action previously occurred. Current "
        "failure count and pending actions are not queried.");
    items.emplace_back(
        "Configured actions may fail or be inapplicable (including service type, permissions, "
        "disabled startup or unavailable command). A reboot or command is conditional on "
        "reaching its action.");
    items.emplace_back(
        "Queries are separate snapshots; configuration may change during inspection.");
    items.emplace_back(
        "Output may contain secrets in configured command text. Commands are displayed as "
        "configuration only, never executed or expanded.");
    items.emplace_back(
        "The application code performs only inspection/query operations and intentionally "
        "requests no configuration changes.");
    return items;
}
} // namespace sfp
