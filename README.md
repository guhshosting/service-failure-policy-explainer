# service-failure-policy-explainer 1.0.0

> Release context: see [qualified platforms, runtime requirements and package scope](RELEASE-QUALIFICATION.md).


A Windows-native, local, read-only SCM recovery configuration explainer. Uses C++17,
Unicode Windows SDK APIs and minimum query permissions. No external commands,
network requests, telemetry, automatic elevation, service control or remediation.

```text
service-failure-policy-explainer Spooler
service-failure-policy-explainer --json Spooler
service-failure-policy-explainer --help
service-failure-policy-explainer --version
```

Supply a canonical service name, rather than its display name. Display names are
explicitly labeled. Release uses the static Microsoft C++ runtime; copy the
installed bin executable to a suitable directory without a runtime installer.
Targets Windows 10, Windows 11 and corresponding supported Server generations.
Older Windows versions are outside the support contract. See VALIDATION.md for
the exact tested host and remaining compatibility work.

## Interpretation

SCM counts failures since boot, subject to the configured reset interval. The Nth
failure selects the Nth action; beyond the array, the final action repeats. An empty
array is successful inspection with no configured actions. A listed `none` is an
explicit action and may be the repeating final action. Unknown future values are
retained numerically and never assigned invented behavior.

Delays are milliseconds before the selected action. Supported action types are
none, restart service, reboot computer and run command. Configuration does not
prove an action occurred, succeeded, or is currently pending. This tool does not
retrieve failure history or the current counter. Disabled services, drivers,
permissions, missing commands and other runtime conditions can affect execution.

The reset period is seconds without a failure before SCM resets the counter to
zero, not an instruction to restart the service. `INFINITE` (4294967295) means no
elapsed-time reset; boot still establishes the counting lifetime. Zero means no
positive reset interval. The SDK's zero/null deletion and preservation rules
describe configuration writes; this application only reads the returned state.

Reboot messages and recovery commands are configuration text. They are displayed
even if no current action references them, without implying they will run. A reboot
message applies when a reboot action is reached. Recovery command text is never
executed, expanded, tested or contacted. **Output can expose configured secrets.**
Human strings are quoted and escaped to avoid control characters disrupting the
terminal; JSON strings preserve their content with standard JSON escaping.

When the non-crash flag is false, configured actions are queued when a service
terminates without reporting SERVICE_STOPPED. When true, this also covers a
service reporting SERVICE_STOPPED with dwWin32ExitCode other than ERROR_SUCCESS.
It does not mean every stop triggers recovery, and is ignored without actions.
Microsoft documents that changes to this flag take effect after the next system
start; the configured flag alone cannot establish the currently effective value
after an unobserved change.

Authoritative semantics:
[SERVICE_FAILURE_ACTIONSW](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-service_failure_actionsw),
[SERVICE_FAILURE_ACTIONS_FLAG](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-service_failure_actions_flag),
[SC_ACTION](https://learn.microsoft.com/en-us/windows/win32/api/winsvc/ns-winsvc-sc_action).

## Safety and privileges

The application code performs only inspection/query operations and intentionally
requests no configuration changes. It opens the local SCM with SC_MANAGER_CONNECT
and the service with SERVICE_QUERY_CONFIG. It never requests status/control/write
access. Service ACLs can deny inspection even to an otherwise authorized user;
no auto-elevation is performed. No intentional networking occurs. SCM and other
Windows libraries have their own internal implementation outside this guarantee.
Separate queries are snapshots, not an atomic transaction. There is no editor,
--fix, monitoring, remote inspection, event correlation or execution path.

Exit codes: 0 success (including no policy), 2 usage, 3 service absent/deleted,
4 access denied, 5 required API/output failure, 6 malformed data/internal failure.
Unsupported policy information is reported as unavailable with a warning; an
unavailable optional non-crash flag is null. Required metadata/API failures go to
stderr. Commands are never repeated in error messages.

## JSON contract

Schema version 1 uses UTF-8 without BOM, one object on stdout, with diagnostics
only on stderr. Fields include application_version, service, reset_period_seconds,
reboot_message, recovery_command, failure_actions,
failure_actions_on_non_crash_failures, explanations, warnings and limitations.
Actions have one-based index, raw_type, type, delay_ms and delay_duration. Null
means unavailable/absent; an empty string is retained when SCM returns it.
failure_actions_available distinguishes an unavailable query from an empty array.
New additive fields may appear in schema 1; incompatible changes require a new
schema version. Explanatory prose is not a machine-stable enum. The service object
includes name, display_name, account, start_type and raw_start_type. Start type
is the basic SCM value; delayed automatic start is outside this focused report.

## Build and validation

Use an x64 Visual Studio developer shell with MSVC, Windows SDK and CMake 3.24+.
Select the installed Visual Studio generator if needed.

```text
cmake -S . -B build -A x64
cmake --build build --config Debug
cmake --build build --config Release
ctest --test-dir build -C Debug --output-on-failure
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install
cmake -S . -B analyze -A x64 -DENABLE_ANALYZE=ON
cmake --build analyze --config Release
cmake -S . -B asan -A x64 -DENABLE_ASAN=ON
cmake --build asan --config RelWithDebInfo
ctest --test-dir asan -C RelWithDebInfo --output-on-failure
```

/W4 /WX are enabled. ASan uses RelWithDebInfo to avoid incompatible Debug runtime
checks. Synthetic tests do not create or mutate services. Live validation only
inspects existing services and tolerates ACL/configuration differences. See
scripts/validate.ps1 for independent JSON parsing and source/import audits.
Architecture separates CLI, model/decoding/durations/explanations, SCM acquisition
with bounded pointer validation and retrying RAII buffers, and human/JSON output.
Tests must never configure a service to make deterministic fixtures.
