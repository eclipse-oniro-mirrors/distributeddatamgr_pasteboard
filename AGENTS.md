# Repository Guidelines

## Repository Positioning

This repository is the OpenHarmony pasteboard service component, located at
`/foundation/distributeddatamgr/pasteboard` in the OpenHarmony source tree.

## Code Map

This AGENTS.md applies to the repository root. No subdirectory agent rules.

This component implements clipboard data storage, IPC access,
framework/client APIs, language bindings, and service configuration. The key
boundary is between public APIs in `interfaces/` and `framework/innerkits/`,
client logic in `framework/`, and service behavior in `services/`.

Key areas:
- `interfaces/`: public API bindings for NAPI, NDK, CJ, and Taihe.
- `framework/innerkits/`: native client APIs and paste data models.
- `framework/framework/` and `framework/tlv/`: client support, adapters, and
  TLV serialization.
- `services/core/`: service behavior, validation, account/user handling, and
  permissions.
- `services/*.idl` and `services/zidl/`: IPC definitions. The main
  client-service IPC contract is declared in IDL files under `services/`
  (for example `IPasteboardService.idl`, `IPasteboardSignal.idl`,
  `IPasteboardClientDeathObserver.idl`, with shared types in
  `PasteboardTypes.idl`); the generated stubs/proxies/transaction codes are
  emitted under
  `[OpenHarmony source root]/out/[product-name]/gen/foundation/distributeddatamgr/pasteboard/services`
  (for example `.../out/rk3568/gen/...`) at build time. `services/zidl/` only
  holds the hand-maintained stub/proxy/code for auxiliary interfaces
  (observers, getters, disposable observer), so it is not a complete view of
  the IPC surface; always consult the IDL files for the authoritative list of
  client-service methods and their `[ipccode N]` transaction codes.
- `adapter/`: integration with device profile, security level, and data share.
- `etc/` and `profile/`: init and system ability profiles.
- `test/`, `services/test/`, `framework/test/`, `adapter/test/`, and
  `utils/test`: unit, fuzz, and module tests.

Language bindings under `interfaces/`:
- NAPI: Node-style native bindings for the JavaScript/ArkTS layer.
- NDK: C API for native consumers.
- CJ: Huawei Cangjie (仓颉) language bindings.
- Taihe (太和): an IDL-based description language. Developers write the IDL
  file (`interfaces/taihe/idl/ohos.pasteboard.pasteboard.taihe`) and
  `interfaces/taihe/BUILD.gn`; at build time the IDL is compiled into actual
  `.ets` and `.cpp`/`.c` files emitted under
  `out/[product-name]/taihe/out/distributeddatamgr/pasteboard`. Its role is
  similar to NAPI, providing the bridge between the JavaScript/ArkTS layer
  and the C++ layer. The hand-maintained C++ binding implementation lives in
  `interfaces/taihe/src/*.impl.cpp` (for example
  `ohos.pasteboard.pasteboard.impl.cpp`), which the generated code calls into
  to reach the pasteboard client.

Where to look:
- Public API change -> `interfaces/`, `framework/innerkits/`, `bundle.json`.
- Service behavior change -> `services/core/`, then nearby tests.
- IPC or permission change -> `services/*.idl` (authoritative IPC contract;
  also check `services/zidl/` and `services/core/include/`).
- Serialization or data model change -> `framework/innerkits/`,
  `framework/tlv/`.
- Build flag or dependency change -> `pasteboard.gni`, `bundle.json`, nearest
  `BUILD.gn`.
- Test-only change -> inspect nearby tests and existing fixture patterns first.
- userId-related change -> first locate `GetAppInfo` in
  `services/core/src/pasteboard_service.cpp` to comb through the spec (it maps a
  calling tokenId to `{userId, bundleName, appIndex, tokenType}` and branches on
  token type: HAP via `FillHapAppInfo`, native/shell via `FillNativeAppInfo` and
  `ResolveMainDisplayUserId`); then use
  `services/core/src/pasteboard_user_context.cpp` as the supplement for other
  (non-IPC-entry) scenarios such as user switch, package removal, foreground
  user, and main display user resolution.

Common-event data parsing: reads `data.GetWant().GetAction()` and `data.GetCode()`
then routes to handlers. For each subscribed event, the parsed fields are:
- `COMMON_EVENT_USER_SWITCHED`: userId from `data.GetCode()`.
- `COMMON_EVENT_USER_STOPPING`: userId from `data.GetCode()`.
- `COMMON_EVENT_SCREEN_LOCKED`: userId from `data.GetWant().GetIntParam("userId", ERROR_USERID)`.
- `COMMON_EVENT_SCREEN_UNLOCKED`: userId from the same Want param as above.
- `COMMON_EVENT_PACKAGE_REMOVED`: `want.GetIntParam("accessTokenId", -1)` and
  userId = `want.GetIntParam("userId", ERROR_USERID)`.

## Constraints and Boundaries

### Architecture Invariants

- Public APIs in `interfaces/` and `framework/innerkits/` express stable
  pasteboard capability semantics, not service-internal implementation details.
- Permission, caller identity, account/user, lock-screen, and owner checks
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

There are two build environments:

- Single-module build (单模块编译): used when the OpenHarmony source tree
  contains the business code of all modules. Commands:
  - `./build.sh --product-name rk3568 --build-target pasteboard`: build the
    pasteboard component.
  - `./build.sh --product-name rk3568 --build-target //foundation/distributeddatamgr/pasteboard/test:unittest`:
    build unit tests.
  - `./build.sh --product-name rk3568 --build-target //foundation/distributeddatamgr/pasteboard/test:fuzztest`:
    build fuzzers.
  - `--build-target` may be any valid GN target, such as
    `//foundation/distributeddatamgr/pasteboard/services:pasteboard_service`.
- Independent build (独立编译): used when only the current module's business
  code is present. Run at the OpenHarmony source root:
  - `hb build [component] -i`: build the component's business code (for this
    repo, `[component]` is `pasteboard`).
  - `hb build [component] -t`: build the component's test cases.

- `rg "symbol" services framework interfaces`: search the main code paths.

## Coding Style & Naming Conventions

Follow existing OpenHarmony C++ style: Apache 2.0 headers, 4-space indentation,
function/class braces on their own line, and `OHOS::MiscServices` namespaces.
GN targets and files use lower_snake_case, such as
`pasteboard_service_get_test.cpp`. C++ classes use PascalCase; tests use numbered
names such as `GetMimeTypesTest001`. Avoid logging sensitive data.

Business code file copyright header uses the format
`Copyright (c) [创建时间]-[修改时间] Huawei Device Co., Ltd.` (for example,
`Copyright (c) 2025-2026 Huawei Device Co., Ltd.`), where the right year is the
modification year and the left year is the creation year. When editing any
business source file, the modification year (right side of the range) MUST be
updated to the current year; do not touch the creation year (left side)
unless the file is newly created, in which case use a single current year
(for example, `Copyright (c) 2026 Huawei Device Co., Ltd.`) until the next
edit. This rule applies to `.cpp`, `.h`, and other business source files; it
does not apply to generated files, `BUILD.gn`, `bundle.json`, or
configuration/profile files.

## Testing Guidelines

Unit tests use GTest and `HWTEST_F`. Add tests next to the changed module and
wire new files into the nearest `BUILD.gn`. Use `test:unittest` for regression
coverage. Add fuzz tests under `test/fuzztest/<name>_fuzzer` for external
parsing, IPC, TLV, or buffer-handling changes.

When writing a test `BUILD.gn` target, always add
`cflags = [ "-fno-access-control" ]` so the test can directly invoke and assert
on private member functions and member variables of the class under test; this
matches the convention already used across `adapter/test`, `services/test`,
`framework/test`, and the fuzz targets.

Test cases should be written with the goal of verifying that interface return
values match expectations, while covering as many code branches as possible
(both success and error paths, and each branch of conditional logic).

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
- CJ/Taihe API change -> inspect `interfaces/cj/include/`,
  `interfaces/taihe/idl/ohos.pasteboard.pasteboard.taihe`, then build the
  corresponding `interfaces/cj`, `interfaces/taihe` target.
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
- Performance impact: required for any API-related change. Consider the
  potential performance impact of the change (for example extra IPC round
  trips, serialization cost, memory allocation, blocking work on the service
  thread, cross-device payload size) and surface it to the developer in the
  final response.
- Remaining risks or follow-up items.

## Commit & Pull Request Guidelines

Keep commit subjects concise and scoped. A single commit must not add more
than 2000 lines of new code; split the work into multiple scoped commits or
PRs if it would exceed that limit. PRs should include linked issue,
purpose, change summary, binary-source status, and TDD/XTS/Pretest results.
Complete log and secure-coding checklists when relevant. Changes to
`services/IPasteboardService.idl` require the
reviewer named in `CODEOWNERS`.
