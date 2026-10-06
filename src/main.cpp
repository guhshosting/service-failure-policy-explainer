#include "policy.hpp"
int wmain(int argc, wchar_t **argv) {
    try {
        std::vector<std::wstring> args;
        for (int i = 1; i < argc; ++i)
            args.emplace_back(argv[i]);
        sfp::Cli c;
        try {
            c = sfp::parse(args);
        } catch (const std::invalid_argument &e) {
            sfp::output(std::string(e.what()) + "\nUse --help for usage.\n", true);
            return 2;
        }
        if (c.help) {
            sfp::output(
                "Usage: service-failure-policy-explainer [--json] <canonical-service-name>\n       "
                "service-failure-policy-explainer --help | --version\nLocal read-only SCM "
                "inspection. Commands are displayed, never executed.\nOutput may include sensitive "
                "configured command text. JSON/redirection uses UTF-8.\n");
            return 0;
        }
        if (c.version) {
            sfp::output(std::string(sfp::version) + "\n");
            return 0;
        }
        auto r = sfp::inspect(c.name);
        sfp::output(c.json ? sfp::json(r) : sfp::human(r));
        return 0;
    } catch (const sfp::ApiError &e) {
        try {
            sfp::output(std::string(e.what()) + "\n", true);
        } catch (...) {
            return 5;
        }
        if (e.code == ERROR_SERVICE_DOES_NOT_EXIST || e.code == ERROR_SERVICE_MARKED_FOR_DELETE)
            return 3;
        if (e.code == ERROR_ACCESS_DENIED)
            return 4;
        return 5;
    } catch (const std::exception &e) {
        try {
            sfp::output(std::string("Internal failure: ") + e.what() + "\n", true);
        } catch (...) {
            return 6;
        }
        return 6;
    } catch (...) {
        return 6;
    }
}
