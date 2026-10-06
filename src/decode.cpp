#include "buffer.hpp"
namespace sfp {
void decode_failure_actions(const Buffer &b, Result &r) {
    auto c = b.read(reinterpret_cast<const SERVICE_FAILURE_ACTIONSW *>(b.bytes.data()));
    r.reset = c.dwResetPeriod;
    r.reboot = b.string(c.lpRebootMsg);
    r.command = b.string(c.lpCommand);
    if (c.cActions &&
        (c.cActions > b.bytes.size() / sizeof(SC_ACTION) ||
         !b.contains(c.lpsaActions, static_cast<size_t>(c.cActions) * sizeof(SC_ACTION))))
        throw std::runtime_error("Malformed SCM action array");
    for (DWORD i = 0; i < c.cActions; ++i) {
        const auto *action_bytes = reinterpret_cast<const unsigned char *>(c.lpsaActions + i);
        // Preserve future numeric values without evaluating an out-of-range SDK enum.
        uint32_t raw_type = 0;
        static_assert(sizeof(SC_ACTION_TYPE) == sizeof(raw_type),
                      "Unexpected SDK action representation");
        DWORD delay = 0;
        std::memcpy(&raw_type, action_bytes + offsetof(SC_ACTION, Type), sizeof(raw_type));
        std::memcpy(&delay, action_bytes + offsetof(SC_ACTION, Delay), sizeof(delay));
        r.actions.push_back({raw_type, delay});
        if (action_type(raw_type) == "unknown")
            r.warnings.push_back("Unknown action type " + std::to_string(raw_type) +
                                 " retained without interpreting behavior.");
    }
}
} // namespace sfp
