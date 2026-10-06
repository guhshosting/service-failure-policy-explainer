#include "policy.hpp"
#include <iomanip>
#include <sstream>
namespace sfp {
std::string quote(const std::string &s) {
    std::ostringstream o;
    o << '"';
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            o << "\\\"";
            break;
        case '\\':
            o << "\\\\";
            break;
        case '\n':
            o << "\\n";
            break;
        case '\r':
            o << "\\r";
            break;
        case '\t':
            o << "\\t";
            break;
        default:
            if (c < 32)
                o << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                  << static_cast<unsigned>(c) << std::dec;
            else
                o << static_cast<char>(c);
        }
    }
    o << '"';
    return o.str();
}
namespace {
std::string start(uint32_t t) {
    switch (t) {
    case SERVICE_BOOT_START:
        return "boot";
    case SERVICE_SYSTEM_START:
        return "system";
    case SERVICE_AUTO_START:
        return "automatic";
    case SERVICE_DEMAND_START:
        return "manual";
    case SERVICE_DISABLED:
        return "disabled";
    default:
        return "unknown";
    }
}
std::string optional(const std::optional<std::wstring> &s) {
    return s ? quote(utf8(*s)) : "null";
}
std::string array(const std::vector<std::string> &items) {
    std::string s = "[";
    for (const auto &i : items) {
        if (s.size() > 1)
            s += ',';
        s += quote(i);
    }
    return s + "]";
}
} // namespace
std::string json(const Result &r) {
    std::ostringstream o;
    o << "{\"schema_version\":1,\"application_version\":\"" << version
      << "\",\"service\":{\"name\":" << quote(utf8(r.name))
      << ",\"display_name\":" << quote(utf8(r.display))
      << ",\"start_type\":" << quote(start(r.start)) << ",\"raw_start_type\":" << r.start
      << ",\"account\":" << quote(utf8(r.account))
      << "},\"failure_actions_available\":" << (r.policy_available ? "true" : "false")
      << ",\"reset_period_seconds\":";
    if (r.policy_available)
        o << r.reset;
    else
        o << "null";
    o << ",\"reset_period_meaning\":"
      << (r.policy_available ? quote(reset_meaning(r.reset)) : "null")
      << ",\"reboot_message\":" << optional(r.reboot)
      << ",\"recovery_command\":" << optional(r.command) << ",\"failure_actions\":[";
    for (size_t i = 0; i < r.actions.size(); ++i) {
        if (i)
            o << ',';
        const auto &a = r.actions[i];
        o << "{\"index\":" << i + 1 << ",\"raw_type\":" << a.raw
          << ",\"type\":" << quote(action_type(a.raw)) << ",\"delay_ms\":" << a.delay
          << ",\"delay_duration\":" << quote(duration(a.delay)) << "}";
    }
    o << "],\"failure_actions_on_non_crash_failures\":"
      << (r.non_crash ? (*r.non_crash ? "true" : "false") : "null")
      << ",\"explanations\":" << array(explanations(r)) << ",\"warnings\":" << array(r.warnings)
      << ",\"limitations\":" << array(limitations()) << "}\n";
    return o.str();
}
std::string human(const Result &r) {
    std::ostringstream o;
    o << "Service name: " << quote(utf8(r.name)) << "\nDisplay name: " << quote(utf8(r.display))
      << "\nStart type: " << start(r.start) << " (" << r.start
      << ")\nService account: " << quote(utf8(r.account)) << "\n\nRecovery policy:\n";
    if (r.actions.empty())
        o << (r.policy_available ? "  No failure actions configured.\n"
                                 : "  Configuration unavailable.\n");
    for (size_t i = 0; i < r.actions.size(); ++i) {
        const auto &a = r.actions[i];
        o << "  Failure " << i + 1 << (i + 1 == r.actions.size() ? " and later" : "") << ": ";
        switch (a.raw) {
        case SC_ACTION_NONE:
            o << "Take no action";
            break;
        case SC_ACTION_RESTART:
            o << "Restart service";
            break;
        case SC_ACTION_REBOOT:
            o << "Reboot computer";
            break;
        case SC_ACTION_RUN_COMMAND:
            o << "Run configured command";
            break;
        default:
            o << "Unknown action (" << a.raw << ")";
        }
        o << "; configured delay " << duration(a.delay) << " (" << a.delay << " ms)\n";
    }
    o << "\nFailure counter: " << (r.policy_available ? reset_meaning(r.reset) : "unavailable")
      << "\n";
    if (r.policy_available)
        o << "Raw reset period: " << r.reset << " seconds\n";
    if (r.reboot && !r.reboot->empty())
        o << "Configured reboot message: " << quote(utf8(*r.reboot)) << "\n";
    if (r.command && !r.command->empty())
        o << "Configured recovery command (text only): " << quote(utf8(*r.command)) << "\n";
    o << "Non-crash failure flag: "
      << (r.non_crash ? (*r.non_crash ? "enabled" : "disabled") : "unavailable") << "\n";
    for (const auto &s : explanations(r))
        o << "\n" << s << "\n";
    for (const auto &s : r.warnings)
        o << "Warning: " << s << "\n";
    for (const auto &s : limitations())
        o << "\n" << s << "\n";
    return o.str();
}
} // namespace sfp
