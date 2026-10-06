#include "policy.hpp"
namespace sfp {
Cli parse(const std::vector<std::wstring> &args) {
    Cli c;
    for (const auto &a : args) {
        if (a == L"--json") {
            if (c.json)
                throw std::invalid_argument("Duplicate --json");
            c.json = true;
        } else if (a == L"--help")
            c.help = true;
        else if (a == L"--version")
            c.version = true;
        else if (a.empty() || a.front() == L'-' || !c.name.empty())
            throw std::invalid_argument("Expected one canonical service name");
        else
            c.name = a;
    }
    if (c.help || c.version) {
        if (args.size() != 1)
            throw std::invalid_argument("Help/version must be used alone");
    } else if (c.name.empty() || c.name.size() > 256 ||
               c.name.find_first_of(L"/\\") != std::wstring::npos)
        throw std::invalid_argument("Expected a local canonical service name");
    return c;
}
} // namespace sfp
