#include "buffer.hpp"
#include "policy.hpp"
#include <memory>
namespace sfp {
namespace {
struct Close {
    void operator()(SC_HANDLE h) const noexcept {
        if (h)
            CloseServiceHandle(h);
    }
};
using Handle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, Close>;
Buffer query(SC_HANDLE h, DWORD level, bool metadata = false) {
    Buffer b;
    DWORD needed = 0;
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        BOOL ok =
            metadata
                ? QueryServiceConfigW(h, reinterpret_cast<QUERY_SERVICE_CONFIGW *>(b.bytes.data()),
                                      static_cast<DWORD>(b.bytes.size()), &needed)
                : QueryServiceConfig2W(h, level, b.bytes.data(), static_cast<DWORD>(b.bytes.size()),
                                       &needed);
        if (ok)
            return b;
        auto err = GetLastError();
        if (err != ERROR_INSUFFICIENT_BUFFER)
            throw ApiError(metadata ? "QueryServiceConfigW" : "QueryServiceConfig2W", err);
        if (!needed || needed > 16 * 1024 * 1024)
            throw std::runtime_error("Unexpected SCM buffer size");
        b.bytes.resize(needed);
    }
    throw ApiError("SCM configuration changed repeatedly", ERROR_RETRY);
}
bool unsupported(DWORD c) {
    return c == ERROR_INVALID_LEVEL || c == ERROR_NOT_SUPPORTED || c == ERROR_INVALID_PARAMETER;
}
void metadata(SC_HANDLE h, Result &r) {
    auto b = query(h, 0, true);
    auto c = b.read(reinterpret_cast<const QUERY_SERVICE_CONFIGW *>(b.bytes.data()));
    r.display = b.string(c.lpDisplayName).value_or(L"");
    r.account = b.string(c.lpServiceStartName).value_or(L"");
    r.start = c.dwStartType;
}
void policy(SC_HANDLE h, Result &r) {
    try {
        auto b = query(h, SERVICE_CONFIG_FAILURE_ACTIONS);
        decode_failure_actions(b, r);
    } catch (const ApiError &e) {
        if (!unsupported(e.code))
            throw;
        r.policy_available = false;
        r.warnings.push_back("Failure actions information level unsupported.");
    }
    try {
        auto b = query(h, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG);
        auto f = b.read(reinterpret_cast<const SERVICE_FAILURE_ACTIONS_FLAG *>(b.bytes.data()));
        r.non_crash = f.fFailureActionsOnNonCrashFailures != FALSE;
    } catch (const ApiError &e) {
        if (!unsupported(e.code) && e.code != ERROR_ACCESS_DENIED)
            throw;
        r.warnings.push_back("Non-crash failure flag unavailable (Windows error " +
                             std::to_string(e.code) + ").");
    }
}
} // namespace
Result inspect(const std::wstring &name) {
    Handle scm(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scm)
        throw ApiError("OpenSCManagerW", GetLastError());
    Handle service(OpenServiceW(scm.get(), name.c_str(), SERVICE_QUERY_CONFIG));
    if (!service)
        throw ApiError("OpenServiceW", GetLastError());
    Result r;
    r.name = name;
    metadata(service.get(), r);
    policy(service.get(), r);
    return r;
}
} // namespace sfp
