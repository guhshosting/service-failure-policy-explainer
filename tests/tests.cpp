#include "buffer.hpp"
#include "policy.hpp"
#include <iostream>
void check(bool ok) {
    if (!ok)
        throw std::runtime_error("Test assertion failed");
}
int main() {
    try {
        sfp::Buffer raw;
        raw.bytes.resize(sizeof(SERVICE_FAILURE_ACTIONSW) + 2 * sizeof(SC_ACTION));
        SERVICE_FAILURE_ACTIONSW policy{};
        policy.dwResetPeriod = INFINITE;
        policy.cActions = 2;
        policy.lpsaActions = reinterpret_cast<SC_ACTION *>(raw.bytes.data() + sizeof(policy));
        const SC_ACTION actions[] = {{SC_ACTION_RESTART, 60000}, {SC_ACTION_NONE, 0}};
        std::memcpy(raw.bytes.data(), &policy, sizeof(policy));
        std::memcpy(raw.bytes.data() + sizeof(policy), actions, sizeof(actions));
        const uint32_t unknown_type = 99;
        std::memcpy(raw.bytes.data() + sizeof(policy) + sizeof(SC_ACTION) +
                        offsetof(SC_ACTION, Type),
                    &unknown_type, sizeof(unknown_type));
        sfp::Result decoded;
        sfp::decode_failure_actions(raw, decoded);
        check(decoded.reset == INFINITE && decoded.actions.size() == 2);
        check(decoded.actions[0].raw == SC_ACTION_RESTART && decoded.actions[0].delay == 60000);
        check(decoded.actions[1].raw == 99 && decoded.warnings.size() == 1);
        policy.cActions = MAXDWORD;
        std::memcpy(raw.bytes.data(), &policy, sizeof(policy));
        bool invalid_array = false;
        try {
            sfp::Result bad;
            sfp::decode_failure_actions(raw, bad);
        } catch (const std::runtime_error &) {
            invalid_array = true;
        }
        check(invalid_array);
        policy.cActions = 1;
        policy.lpsaActions = nullptr;
        std::memcpy(raw.bytes.data(), &policy, sizeof(policy));
        invalid_array = false;
        try {
            sfp::Result bad;
            sfp::decode_failure_actions(raw, bad);
        } catch (const std::runtime_error &) {
            invalid_array = true;
        }
        check(invalid_array);
        policy.cActions = 0;
        std::memcpy(raw.bytes.data(), &policy, sizeof(policy));
        sfp::Result empty_policy;
        sfp::decode_failure_actions(raw, empty_policy);
        check(empty_policy.actions.empty());
        sfp::Buffer buffer;
        buffer.bytes.resize(8);
        check(!buffer.contains(nullptr, 0));
        check(buffer.contains(buffer.bytes.data(), 8));
        check(!buffer.contains(buffer.bytes.data() + 8, 1));
        check(!buffer.string(nullptr));
        auto empty = buffer.string(reinterpret_cast<const wchar_t *>(buffer.bytes.data()));
        check(empty && empty->empty());
        bool malformed = false;
        try {
            (void)buffer.read(reinterpret_cast<const DWORD *>(buffer.bytes.data() + 8));
        } catch (const std::runtime_error &) {
            malformed = true;
        }
        check(malformed);
        std::fill(buffer.bytes.begin(), buffer.bytes.end(), static_cast<unsigned char>(1));
        malformed = false;
        try {
            (void)buffer.string(reinterpret_cast<const wchar_t *>(buffer.bytes.data()));
        } catch (const std::runtime_error &) {
            malformed = true;
        }
        check(malformed);
        sfp::Result r;
        r.name = L"test";
        check(sfp::human(r).find("No failure actions") != std::string::npos);
        check(sfp::json(r).find("\"failure_actions\":[]") != std::string::npos);
        const std::vector<std::string> types = {"none", "restart", "reboot", "run_command",
                                                "unknown"};
        size_t type_index = 0;
        for (uint32_t t : {0u, 1u, 2u, 3u, 99u}) {
            r.actions = {{t, 0}};
            check(sfp::action_type(t) == types[type_index++]);
            check(sfp::human(r).find("Failure 1 and later") != std::string::npos);
            check(sfp::json(r).find("\"raw_type\":" + std::to_string(t)) != std::string::npos);
        }
        r.actions = {{1, 60000}, {2, 4294967295u}, {3, 0}, {0, 1}};
        check(sfp::human(r).find("Failure 4 and later") != std::string::npos);
        check(sfp::duration(0) == "0 milliseconds");
        check(sfp::duration(60000) == "1 minute");
        check(sfp::duration(4294967295u) ==
              "49 days 17 hours 2 minutes 47 seconds 295 milliseconds");
        check(sfp::reset_meaning(INFINITE).find("Never") != std::string::npos);
        check(sfp::reset_meaning(0).find("Zero") != std::string::npos);
        check(sfp::reset_meaning(86400).find("1 day") != std::string::npos);
        r.command = L"工具.exe --秘密=値\n\"quoted\"";
        r.reboot = L"再起動";
        r.non_crash = true;
        check(sfp::json(r).find("\\n\\\"quoted\\\"") != std::string::npos);
        check(sfp::json(r).find("\"failure_actions_on_non_crash_failures\":true") !=
              std::string::npos);
        r.non_crash = false;
        check(sfp::json(r).find("\"failure_actions_on_non_crash_failures\":false") !=
              std::string::npos);
        check(sfp::explanations(r)[2].find("only when") != std::string::npos);
        r.non_crash.reset();
        r.policy_available = false;
        check(sfp::json(r).find("\"reset_period_seconds\":null") != std::string::npos);
        check(sfp::json(r).find("\"failure_actions_on_non_crash_failures\":null") !=
              std::string::npos);
        check(sfp::quote(std::string("\x01\b\f\r\t\\\"")) ==
              "\"\\u0001\\u0008\\u000c\\r\\t\\\\\\\"\"");
        check(sfp::parse({L"--json", L"服务"}).json);
        check(sfp::parse({L"--help"}).help);
        check(sfp::parse({L"--version"}).version);
        for (const auto &args :
             std::vector<std::vector<std::wstring>>{{},
                                                    {L"--fix"},
                                                    {L"--json"},
                                                    {L"a", L"b"},
                                                    {L"--help", L"a"},
                                                    {L"--json", L"--json", L"a"}}) {
            bool rejected = false;
            try {
                (void)sfp::parse(args);
            } catch (const std::invalid_argument &) {
                rejected = true;
            }
            check(rejected);
        }
        sfp::Result fixture;
        fixture.name = L"服务";
        fixture.command = L"工具.exe --値=秘密";
        fixture.actions = r.actions;
        fixture.non_crash = true;
        sfp::output(sfp::json(fixture));
        return 0;
    } catch (const std::exception &e) {
        try {
            sfp::output(e.what(), true);
        } catch (...) {
            return 1;
        }
        return 1;
    } catch (...) {
        return 1;
    }
}
