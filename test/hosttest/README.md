# Host-side test suites (`test/hosttest/`)

Unit tests that build and run on a plain Linux host — **no device, no IPC, no
running pasteboard service**. They give an AI (or a developer, or CI) a fast
red/green + coverage signal for a code change, which the on-device
`ohos_unittest` targets cannot do without hardware.

## One command

```bash
./run_all.sh          # runs every <module>/run_host_test.sh; exit 0 iff all pass
```

Per-suite:

```bash
cd <module> && ./run_host_test.sh
COVERAGE_MIN=95 ./run_host_test.sh    # tighten the gate
```

Exit-code contract (same for `run_all.sh` and each suite):

| code | meaning                                   |
|------|-------------------------------------------|
| 0    | tests passed **and** coverage >= gate     |
| 1    | a test failed                             |
| 2    | coverage below gate                       |
| 3    | build / toolchain problem                 |

`run_all.sh` is the single verification entry point for an AI change→verify
loop and for CI: run it, read the exit code, act on it. It runs the full suite
in **~25s** (googletest is compiled once into a shared cache and reused by every
suite, rather than rebuilt once per suite). A single suite runs in ~7s standalone.

**Concurrency caveat:** suites use fixed `.build/` directories, so do NOT run
two suites (or two `run_all.sh`) at once in the same checkout — the shared build
dirs will clobber each other. Run them sequentially (which `run_all.sh` does).

## The suites (three tiers of host-testability)

| Suite             | Dependency depth | Seam technique              | Tests | Coverage |
|-------------------|------------------|-----------------------------|-------|----------|
| `serializable`    | pure logic (cJSON)| none                        | 14    | 99.07%   |
| `command`         | pure logic        | none                        | 7     | 94.74%   |
| `config`          | composition (Serializable)| none (links serializable.cpp)| 6 | 100%   |
| `dump_helper`     | composition (Command)| none (links command.cpp) | 6     | 100%     |
| `pasteboard_time` | POSIX + 1 header  | include path only           | 4     | 92.86%   |
| `progress_signal` | shallow (unused heavy include) | empty shim + c_utils path | 6 | 100% |
| `eventcenter`     | shallow (hilog)   | single-header shim          | 9     | 94.44%   |
| `clip_plugin`     | shallow (hilog + dfx) | single-header shims + links serializable | 16 | 100% |
| `security_level`  | deep (DEVSL + DMAdapter) | fakes with test hooks (level/udid) | 7 | 100% |
| `tlv`             | deep (parcel/pixelmap/want/uri/securec/udmf/hilog) | faithful fakes + fault injection | 68 | 97.22% / 97.37% / 91.19% |
| `paste_data_entry`| composition (TLV codec) + deep (udmf) | links real TLV codec + reuses `tlv/fakes` | 52 | 100% |

Read each suite's `README.md` for its specifics. `tlv/` covers three units
(`tlv_utils` / `tlv_writeable` / `tlv_readable`), each gated separately so a
well-covered file cannot mask a bare one. `tlv/` and `security_level/` are
templates for modules behind heavy platform types; `config/`, `dump_helper/`,
`clip_plugin/` and `paste_data_entry/` show building a suite on top of an
already-host-tested module (serializable / command / the TLV codec).

`paste_data_entry/` owns no fakes of its own — it reuses `tlv/fakes` and links
the real TLV codec. That is the intended pattern for the rest of
`framework/innerkits`, whose files are all TLV-serialisable data objects.

**On `LCOV_EXCL` markers:** several product files wrap large regions in
`LCOV_EXCL_START` / `LCOV_EXCL_STOP`. Those are honoured by `lcov`, **not** by
the `gcov` this loop uses, so these gates measure the whole file. That is
deliberate — in `paste_data_entry.cpp` the excluded region is the type-mapping
logic, which is precisely where a wrong pairing hides.

## Toolchain

System `g++` + `gcov-12` (matching versions). NOT the OHOS LLVM toolchain under
`/opt/llvm` — its coverage runtime is built against OHOS libc and won't link a
host binary. gtest source is at `third_party/googletest`.

## Adding a suite

1. `mkdir test/hosttest/<module>`; write `<module>_host_test.cpp`.
2. Identify the exact dependency surface the unit calls. Pick the lightest seam
   that works: include path > shim > fakes (see the table).
3. Copy a sibling `run_host_test.sh`; adjust sources / include dirs.
4. Add a `.gitignore` (`.build/`, `*.gcno`, `*.gcda`, `*.gcov`, `*_host_test*.xml`).
5. `run_all.sh` auto-discovers it.

## Test case conventions

These suites follow the OpenHarmony developer-test conventions
(`test/testfwk/developer_test/README_zh.md`), the same ones the repo's existing
`ohos_unittest` cases use — the fact that a case runs host-side rather than on a
device does not exempt it:

```cpp
using namespace testing::ext;

/**
 * @tc.name: EncodeSetsRemoteFlag
 * @tc.desc: Encode reports the remote flag through the thread-local.
 * @tc.type: FUNC
 * @tc.require: issueI1671
 * @tc.author:
 */
HWTEST_F(TlvCodecHostTest, EncodeSetsRemoteFlag, TestSize.Level0)
```

- **`HWTEST_F`, not `TEST_F`.** The macro comes from `gtest/hwext/gtest-ext.h`
  and needs no extra build wiring: `gtest-all.cc`, which every suite already
  compiles, includes the `hwext` sources.
- **Test suite name** is upper camel case and matches the module.
- **`@tc.desc`** states what the case *verifies*, not what it does — "Skip
  refuses a length that would run past the end of the stream", not "Skip test".
- **`@tc.require`** must start with `AR`/`SR`/`issue`. Use the issue the suite
  traces to (`issueI1669` for the original loop, `issueI1671` for the TLV codec
  and `paste_data_entry`).

## Coverage philosophy

The 90% gate accepts genuine coverage; it does not push authors to fake
unreachable branches (e.g. `clock_gettime` failing). Prefer driving an error
branch with a hostile *input* (see `tlv`'s oversize `bufferLen` cases); reach
for an injected fault only when input alone cannot get there (see
`tlv/fakes/fault_inject.h`). Never fake a sanctioned safe function just to
force its failure path.

## Latent bugs this surfaced

Isolated host compilation & execution catch defects the full-tree device build
hides. Fixed so far:

- **Missing-include coupling** (compile-time; additive `#include` lines), 9
  instances: `serializable.h`, `common/concurrent_map.h`,
  `eventcenter/event_center.h`, `services/dfx/src/command.h`, `clip/clip_plugin.h`
  (needed 3 headers), `services/dfx/src/pasteboard_dump_helper.h`,
  `adapter/security_level/security_level.cpp` (the first found in a `.cpp`), and
  `pasteboard_progress_signal.h` (`<atomic>` + `nocopyable.h`).
- **Uninitialized-pointer UB** (runtime crash): `Serializable::Marshall()`
  declared `json node;` (uninitialized `cJSON*`) then read it via
  `cJSON_IsObject`. Segfaulted when marshalling a struct with a non-empty
  `vector<string>`; benign in other paths (classic UB). Fixed to
  `json node = nullptr;`. Affected every Serializable subclass's marshalling.
  Found by the `config` suite; guarded by `serializable`'s
  `VectorStringMarshallRegression`.
- **Non-conforming explicit specialisation** (compile-time, portability):
  `tlv_writeable.h` and `tlv_readable.h` declared `template<>` specialisations
  *inside* the class body, which ISO C++ forbids. The LLVM-based device
  toolchain accepts it as an
  extension so the device build never complained; GCC rejects it outright, which
  is how the codec suite found it. Both moved to namespace scope, matching how
  the `.cpp` files already define them.

9 missing-include bugs across 11 suites suggests this is a systemic pattern in
this codebase, not isolated incidents — worth a project-wide sweep at some
point rather than only fixing them as host suites happen to surface them. Note
they appear in both headers and `.cpp` files.

Expect more when host-testing new modules.
