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
- `services/account/`: NOT the real account/user state management for pasteboard. It is a thin `AccountManager` that only exposes `GetCurrentAccount()` (returning a string). Real userId/account handling lives in `services/core/src/pasteboard_service.cpp::GetAppInfo` and `services/core/src/pasteboard_user_context.cpp`; do not look here for user-switch, main-display-user, or token-to-user mapping logic.
- `services/switch/`: pasteboard feature on/off switches (`pasteboard_switch`).
- `services/load/`: service config and dynamic loader for optional SOs (`loader`, `config`).
- `services/dialog/`: the pasteboard progress/toast dialog HAP — NOT a permission-confirm or share-option dialog. It builds the `pasteboard_dialog` HAP whose only UI abilities are `PasteboardProgressAbility` (progress bar, used for cross-device paste progress) and `ToastExtensionAbility` (toast). The HAP holds `ohos.permission.MANAGE_SECURE_SETTINGS`, `MANAGE_SETTINGS`, and `GET_BUNDLE_INFO`, but it does not host any permission-confirm or share-option UI.
- `services/dfx/`: DFX — fault/behaviour/statistic reporters, hiview adapter, dump helper, and trace (`dfx_code_constant.h`, `pasteboard_event_dfx`, `reporter`).
- `framework/distributed/`: the distributed `ClipPlugin` implementation — NOT just generic sync adapters. It builds the `distributed_clip` shared library (gated by `pasteboard_distributed_clip_enabled` in `pasteboard.gni`), providing `ClipFactory` (extending `ClipPlugin::Factory`) plus the plugin's internals: broadcast manager, store/observer, all-connect adapter, sync callback, and system-ability listener. The service loads it via the `clip/clip_plugin.h` factory interface. When investigating service-side `ClipPlugin` interface calls (how the service reaches distributed-clip sync, P2P link, or broadcast), read this directory first.
- `tools/ohos-pasteboard/`: `ohos-pasteboard` shell CLI (set/get/clear/has-data/has-data-type/has-remote-data).
- `utils/native/`: shared low-level utilities — hilog macros, the innerkits error enum (`pasteboard_error.h`), FD sanitizer guard, singleton, and time helper.
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

### High-risk / frequent-change paths

- `services/core/src/pasteboard_service.cpp::IsDataValid` (around line 1112): the central data, share-option, cross-border, and expiry validation gate; called from every Set/Get path. Do not weaken it.
- `services/core/src/pasteboard_service.cpp::VerifyPermission` (around line 1075): the entry-point permission gate enforcing `ohos.permission.READ_PASTEBOARD` and `ohos.permission.SECURE_PASTE`; `AccessTokenKit::GetTokenTypeFlag` lets `TOKEN_NATIVE`/`TOKEN_SHELL` callers bypass.
- `services/core/src/pasteboard_service.cpp::CheckAndGrantRemoteUri` (around line 1532): branches on `data.IsRemote()` to grant remote URI permission — this is the trust boundary attacked by the `isRemote` TLV-forgery incident (see `docs/20261109-fix-isremote-forgery`).
- `services/core/src/pasteboard_service.cpp::HandleScreenLocked` (around line 5099) and `screenStatusMap_`: lock-screen state consumed by `IsDataValid`'s cross-border check.
- `services/core/src/security_guard_adapter.cpp`: security-guard risk reporting adapter.
- `framework/innerkits/src/paste_data.cpp::PasteDataProperty::DecodeTLV` (around line 813): client-side TLV decode of `props_.isRemote` and `originInfo` — these values are untrusted; the service must force `isRemote=false` at the Set trust boundary.
- `services/core/include/pasteboard_serv_ipc_interface_code.h`: a hand-maintained header whose `PasteboardServiceInterfaceCode` enum (sequential 0-25) is a DEAD/legacy artifact — it is no longer referenced anywhere and is NOT a source of main service IPC transaction codes. The authoritative main `IPasteboardService` transaction codes live ONLY in `services/IPasteboardService.idl` via `[ipccode N]` (groups: 1-9 observer/lifecycle, 100-105 set, 200-204 get/paste, 300-309 query, 400-404 share option). The header is still included by `services/zidl/` because it defines the AUXILIARY interface codes that are still in use — `PasteboardObserverInterfaceCode`, `PasteboardDisposableInterfaceCode`, `EntityObserverInterfaceCode`, `PasteboardEntryGetterInterfaceCode`. Do not add new main service codes to this header; add them to the IDL instead. Changes to this header still require the `CODEOWNERS` reviewer (`@leonchan5`) because of the auxiliary codes.

## Knowledge Routing

Before editing, state in your work notes: (1) the task category matching a "Where to look" bullet above; (2) any `docs/` incident notes or knowledge docs you read; (3) the high-risk constraints (from Architecture Invariants / Do Not / Ask Before) that apply — or "none touched" if none.

Task/keyword -> read first:

| Keyword / task | Read first |
| --- | --- |
| `isRemote`, remote paste, URI grant, TLV forgery, `GrantUriPermissionPrivileged` | `docs/20261109-fix-isremote-forgery/{analysis-report.md,design.md,plan.md}` — the fix forces `isRemote=false` at the Set trust boundary and fail-closes `CheckAndGrantRemoteUri`. |
| PasteId, paste seq, IDL IPC parameter, `GetPasteId`, `IPCSkeleton::GetCallingPid` | `docs/20260709-modify-paste-id/{1.分析文档.md,2.设计文档.md,3.实现文档.md}` — changes the GetPasteData IDL param from String PasteId to unsigned int PasteSeqId; the service reconstructs the full PasteId from `GetCallingPid()` + seq. |
| TLV, serialization, `DecodeTLV`/`EncodeTLV`, paste data format | `framework/tlv/` and `framework/innerkits/src/paste_data.cpp`; format must stay cross-version compatible (see Architecture Invariants). |
| Permission, `VerifyPermission`, `READ_PASTEBOARD`, `SECURE_PASTE`, token type | `services/core/src/pasteboard_service.cpp` around `VerifyPermission` and `framework/framework/include/permission/permission_utils.h`. |
| Error codes | NDK: `interfaces/ndk/include/oh_pasteboard_err_code.h`; innerkits/C++ enum: `utils/native/include/pasteboard_error.h`; NDK<->innerkits conversion: `interfaces/ndk/include/oh_pasteboard_common.h` and `interfaces/kits/napi/include/systempasteboard_napi.h`. Do not invent codes outside these files. |
| HiSysEvent / DFX events | `pasteboardEvent.yaml` and `services/dfx/`. |
| userId, account, user switch, main display user | `services/core/src/pasteboard_service.cpp::GetAppInfo` + `services/core/src/pasteboard_user_context.cpp` (see "Where to look"). |
| Distributed clip, `ClipPlugin`, `ClipFactory`, remote device, broadcast, store/observer | `framework/distributed/` — the distributed `ClipPlugin` implementation; read first for service-side plugin interface calls. |
| Capability overview | `README_ZH.md` / `README.md`. |

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
- Changing the SA profile (`profile/3701.json`) or init config (`etc/init`), which can break service boot.
- Running `ohos-pasteboard set`/`clear` CLI subcommands, which mutate the live device clipboard.

### Permission & error-code references

- Permissions: `VerifyPermission` (`services/core/src/pasteboard_service.cpp`, around line 1075) enforces `ohos.permission.READ_PASTEBOARD` and `ohos.permission.SECURE_PASTE`; `TOKEN_NATIVE`/`TOKEN_SHELL` callers bypass via `AccessTokenKit::GetTokenTypeFlag`. `ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION` is enforced separately in `SetAppShareOptions` (around line 3512) and `RemoveAppShareOptions` (around line 3537), not in `VerifyPermission`. The dialog HAP additionally holds `ohos.permission.MANAGE_SECURE_SETTINGS`, `MANAGE_SETTINGS`, and `GET_BUNDLE_INFO`.
- Error-code sources of truth: NDK `interfaces/ndk/include/oh_pasteboard_err_code.h` (e.g. `ERR_PERMISSION_ERROR=201`, `ERR_INVALID_PARAMETER=401`, `ERR_INNER_ERROR=12900000`); innerkits enum `utils/native/include/pasteboard_error.h` (`PasteboardError`). New error codes must be added in these files plus the conversion maps; do not surface raw inner codes as NDK codes.
- Security-sensitive component deps (see `bundle.json`) that need escalation when their flow changes: `dataclassification`, `dlp_permission_service`, `security_guard`.

### Known pitfalls / agent failure modes

- Client TLV is untrusted: `props_.isRemote` and `originInfo` arrive from the client via `DecodeTLV`; never branch on client-supplied `isRemote` for a privilege decision — the service must force it false at the Set boundary (see `docs/20261109-fix-isremote-forgery`).
- Do not bypass `IsDataValid`, `VerifyPermission`, or lock-screen/account/owner checks to make a test pass; these are the trust boundary.
- Do not hand-edit generated Taihe/CJ/NAPI output (emitted under `out/[product]/...`); edit the IDL/`.taihe` source and the hand-maintained `*.impl.cpp` binding, then regenerate.
- PasteId is reconstructed by the service from `GetCallingPid()` + seq (per `docs/20260709-modify-paste-id`); do not let the client dictate the full PasteId string.
- `services/zidl/` is not the complete IPC surface — consult `services/*.idl` for the authoritative transaction codes (`[ipccode N]`).
- Main service IPC transaction codes do NOT live in two places: they are defined ONLY in `services/IPasteboardService.idl` `[ipccode N]` (consumed by the generated stubs/proxies under `out/[product]/gen/...`). The `PasteboardServiceInterfaceCode` enum in `services/core/include/pasteboard_serv_ipc_interface_code.h` is a dead/legacy artifact with zero references — do not edit it for main service code changes and do not assume a second source of truth exists. The header still carries the auxiliary interface codes (observer/disposable/entity/entry-getter) used by the hand-maintained `services/zidl/` stubs/proxies; changes to those auxiliary codes still need the `CODEOWNERS` reviewer (`@leonchan5`).

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

### Lint, API diff, and test execution

- There is no repo-local cpplint/cpucodecheck target; C/C++ style is enforced externally, and per-target sanitizers (`integer_overflow`, `ubsan`, `boundary_sanitize`, `cfi`, `cfi_cross_dso`, `branch_protector_ret="pac_ret"`) are enabled in `interfaces/cj/BUILD.gn` and other BUILD.gn files — do not strip sanitize or branch-protection flags.
- For public API changes, diff `interfaces/ndk/pasteboard_interface_map`, `interfaces/kits/pasteboard_js_map`, and `framework/innerkits/pasteboard_kit_map` before and after the change to catch accidental signature/export drift.
- Tests are GTest `ohos_unittest`. Build them with the targets above and, when a device is available, run on device via the standard OpenHarmony test runner (`hdc install` the test HAP + `aa test`); if no device is available, state that execution is unverified and report build-only results.

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
`services/core/include/pasteboard_serv_ipc_interface_code.h` require the
reviewer named in `CODEOWNERS` (`@leonchan5`) — the header still defines the
auxiliary interface codes (observer/disposable/entity/entry-getter) used by
`services/zidl/`. New main `IPasteboardService` transaction codes go in
`services/IPasteboardService.idl` `[ipccode N]`, NOT in this header (whose
`PasteboardServiceInterfaceCode` enum is dead/legacy).
