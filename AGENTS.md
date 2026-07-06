# Repository Guidelines

## Repository Positioning

This repository is the OpenHarmony pasteboard service component, located at
`/foundation/distributeddatamgr/pasteboard` in the OpenHarmony source tree.

## Code Map

This AGENTS.md applies to the repository root. No subdirectory agent rules were
found during creation.

This component implements clipboard data storage, IPC access,
framework/client APIs, language bindings, and service configuration. The key
boundary is between public APIs in `interfaces/` and `framework/innerkits/`,
client logic in `framework/`, and service behavior in `services/`.

Key areas:
- `interfaces/`: public API bindings for NAPI, NDK, CJ, ANI, and Taihe.
- `framework/innerkits/`: native client APIs and paste data models.
- `framework/framework/` and `framework/tlv/`: client support, adapters, and
  TLV serialization.
- `services/core/`: service behavior, validation, account/user handling, and
  permissions.
- `services/zidl/`: IPC contracts, stubs, proxies, and transactions.
- `adapter/`: integration with device profile, security level, and data share.
- `etc/` and `profile/`: init and system ability profiles.
- `test/`, `services/test/`, `framework/test/`, `adapter/test/`, and
  `utils/test`: unit, fuzz, and module tests.

Where to look:
- Public API change -> `interfaces/`, `framework/innerkits/`, `bundle.json`.
- Service behavior change -> `services/core/`, then nearby tests.
- IPC or permission change -> `services/zidl/`, `services/core/include/`.
- Serialization or data model change -> `framework/innerkits/`,
  `framework/tlv/`, `framework/test/src/`.
- Build flag or dependency change -> `pasteboard.gni`, `bundle.json`, nearest
  `BUILD.gn`.
- Test-only change -> inspect nearby tests and existing fixture patterns first.

## Constraints and Boundaries

### Architecture Invariants

- Public APIs in `interfaces/` and `framework/innerkits/` express stable
  pasteboard capability semantics, not service-internal implementation details.
- Permission, caller identity, account/user, lock-screen, drag, and owner checks
  must stay at service or IPC entry points before data is exposed.
- `services/zidl/` transports requests; it must not own high-level pasteboard
  policy that belongs in `services/core/`.
- TLV, IPC, delayed record, and paste data formats must remain compatible across
  versions unless the task explicitly includes migration work.
- DFX, HiLog, HiSysEvent, and error codes must preserve business-critical
  diagnostics for copy, paste, clear, permission denial, and data validation.
- Cross-device and multi-user behavior must handle offline devices, user
  switches, reconnects, version mismatch, and authorization changes.

### Do Not

- Do not introduce new production dependencies without explicit approval.
- Do not change public API signatures, error codes, permission behavior, or
  lifecycle semantics unless the task explicitly requires it.
- Do not bypass `IsDataValid`, permission checks, lock-screen checks, account
  boundaries, or in-app owner restrictions to make a path or test pass.
- Do not leak internal service types, IPC details, or adapter assumptions into
  public APIs.
- Do not modify generated files directly; update the source spec and regenerate.
- Do not remove DFX, logging, events, checks, or compatibility shims only to
  simplify tests.
- Do not run destructive HDC/device operations without user confirmation.

### Ask Before

- Adding dependencies or changing component declarations in `bundle.json`.
- Changing public API semantics, error mapping, or lifecycle behavior.
- Changing permission model, caller trust boundary, or user/account isolation.
- Changing IPC compatibility, TLV layout, persisted data, or migration logic.
- Removing compatibility shims, fallback logic, or build flags in
  `pasteboard.gni`.
- Running commands that may alter connected devices, mounted partitions, or user
  data.

## Build, Test, and Development Commands

- Run build commands from the OpenHarmony source root, not from this component
  directory.
- `./build.sh --product-name rk3568 --build-target pasteboard`: build the
  pasteboard component.
- `./build.sh --product-name rk3568 --build-target //foundation/distributeddatamgr/pasteboard/test:unittest`:
  build unit tests.
- `./build.sh --product-name rk3568 --build-target //foundation/distributeddatamgr/pasteboard/test:fuzztest`:
  build fuzzers.
- `--build-target` may be any valid GN target, such as
  `//foundation/distributeddatamgr/pasteboard/services:pasteboard_service`.
- `rg "symbol" services framework interfaces`: search the main code paths.

## Coding Style & Naming Conventions

Follow existing OpenHarmony C++ style: Apache 2.0 headers, 4-space indentation,
function/class braces on their own line, and `OHOS::MiscServices` namespaces.
GN targets and files use lower_snake_case, such as
`pasteboard_service_get_test.cpp`. C++ classes use PascalCase; tests use numbered
names such as `GetMimeTypesTest001`. Avoid logging sensitive data.

## Testing Guidelines

Unit tests use GTest and `HWTEST_F`. Add tests next to the changed module and
wire new files into the nearest `BUILD.gn`. Use `test:unittest` for regression
coverage. Add fuzz tests under `test/fuzztest/<name>_fuzzer` for external
parsing, IPC, TLV, or buffer-handling changes.

## Verification

### Minimum Checks

- Search/inspection: use `rg` to confirm affected entry points, tests, and build
  files.
- Build current module from the OpenHarmony source root:
  `./build.sh --product-name rk3568 --build-target pasteboard`.
- Build focused tests from the source root, using the nearest valid GN target,
  for example
  `./build.sh --product-name rk3568 --build-target //foundation/distributeddatamgr/pasteboard/test:unittest`.
- For fuzz-sensitive parsing, IPC, TLV, or buffer changes, also build the
  relevant fuzzer target or `//foundation/distributeddatamgr/pasteboard/test:fuzztest`.
- For public API changes, inspect exported headers, IDL/ETS files, symbol maps,
  and `bundle.json` metadata; then build the affected interface target.

### Task-Specific Checks

- NDK API change -> inspect `interfaces/ndk/include/oh_pasteboard*.h` and
  `interfaces/ndk/pasteboard_interface_map`, then build
  `//foundation/distributeddatamgr/pasteboard/interfaces/ndk:libpasteboard` and
  `//foundation/distributeddatamgr/pasteboard/interfaces/ndk/unittest:unittest`.
- NAPI/JS API change -> inspect `interfaces/kits/napi/`,
  `interfaces/kits/pasteboard_js_map`, and JS unit tests, then build
  `//foundation/distributeddatamgr/pasteboard/interfaces/kits:pasteboard_napi`
  plus the affected `pasteboardapi` or `pasteboardperf` test target.
- InnerKit API change -> inspect `framework/innerkits/include/`,
  `framework/innerkits/pasteboard_kit_map`, and `bundle.json` `inner_kits`,
  then build the affected `pasteboard_client` or `pasteboard_data` target.
- CJ/ANI/Taihe API change -> inspect `interfaces/cj/include/`,
  `interfaces/ani/ets/@ohos.pasteboard.ets`, or
  `interfaces/taihe/idl/ohos.pasteboard.pasteboard.taihe`, then build the
  corresponding `interfaces/cj`, `interfaces/ani`, or `interfaces/taihe` target.
- Native C++ service change -> build `pasteboard` or the affected GN target and
  build/run nearby unit tests when the environment supports execution.
- Permission, user, lock-screen, drag, or owner validation change -> cover both
  allowed and denied paths in service tests.
- IPC, TLV, delayed record, or paste data format change -> build relevant unit
  tests and fuzz targets, and document compatibility impact.
- DFX/logging change -> verify related diagnosis, HiLog, HiSysEvent, and error
  code behavior is preserved.
- Cross-device or multi-user change -> run focused smoke or integration tests
  when devices are available; otherwise state the missing device dependency.
- Test-only change -> build the changed test target and at least one nearby
  related test target.

### Done Definition

A task is done only when:
- The requested behavior is implemented.
- Relevant build, test, lint, fuzz, or compatibility checks have been run, or a
  concrete reason is given for anything skipped.
- The final answer includes changed files, validation commands and results, and
  remaining risks.
- No unrelated formatting, refactor, or drive-by changes are included.

## Final Response Format

For non-trivial tasks, include:
- Summary of changes.
- Files changed.
- Validation commands and results.
- Compatibility, permission, DFX, multi-user, or cross-device impact if relevant.
- Remaining risks or follow-up items.

## Commit & Pull Request Guidelines

Keep commit subjects concise and scoped. PRs should include linked issue,
purpose, change summary, binary-source status, and TDD/XTS/Pretest results.
Complete log and secure-coding checklists when relevant. Changes to
`services/core/include/pasteboard_serv_ipc_interface_code.h` require the
reviewer named in `CODEOWNERS`.
