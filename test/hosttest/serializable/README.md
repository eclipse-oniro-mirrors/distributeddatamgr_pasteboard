# Host-side test loop — sample (Serializable module)

This directory is a **proof-of-concept** for a test loop that an AI (or any
developer / CI job) can run to verify a code change **without a device, without
IPC, and without a running pasteboard service**.

It targets one pure-logic module: `framework/framework/serializable/serializable.cpp`
(JSON (de)serialization over cJSON).

## Why this exists

The repo's real tests (`framework/test`, `services/test`, ~1800 `HWTEST` cases)
are `ohos_unittest` targets that compile to arm64 and run on a device via
HDC/xdevice. Many "unit" tests also call `PasteboardClient::GetInstance()->...`,
i.e. they cross a real IPC boundary into the SA. In an environment with no
device attached, there is no red/green signal — so an AI cannot self-verify a
change it just wrote.

This sample closes that gap for one module: it builds and runs on a plain Linux
host and reports pass/fail **and** line coverage in a single command.

## Run it

```bash
./run_host_test.sh
```

Exit codes (the machine-readable contract for an AI loop):

| code | meaning                                        |
|------|------------------------------------------------|
| 0    | all tests passed **and** coverage >= gate      |
| 1    | a unit test failed                             |
| 2    | tests passed but coverage below the gate       |
| 3    | build / toolchain problem                      |

Knobs (env vars):

```bash
COVERAGE_MIN=95 ./run_host_test.sh   # raise the coverage gate (default 90)
CXX=g++-11 GCOV=gcov-11 ./run_host_test.sh
```

Current status: **13 tests, 99.07% line coverage** on serializable.cpp.

## How it works

- Compiles `serializable.cpp` **with** `--coverage` (gcov instrumentation).
- Compiles cJSON, googletest and the test **without** coverage, so the gate
  measures only the unit under test.
- Runs the gtest binary, then reads gcov's line-coverage number and compares it
  to `COVERAGE_MIN`.
- Uses the system `g++` / `gcov-12`, not the OHOS LLVM toolchain under
  `/opt/llvm` (its coverage runtime is built against OHOS libc and will not link
  a host binary — see Findings).

The test itself (`serializable_host_test.cpp`) depends on **only**
`serializable.h` + cJSON + gtest. Contrast with the on-device
`framework/test/src/serializable_test.cpp`, which pulls in `clip_plugin.h`,
`config.h` and `pasteboard_hilog.h` and therefore cannot run host-side.

## Findings surfaced while building this loop

1. **`serializable.h` had missing includes (FIXED).** It uses `std::vector`,
   `uint8_t`, `int32_t`, etc. but did not `#include <vector>` or `<cstdint>`.
   On-device builds only succeeded because other headers included those
   transitively; building the module in isolation exposed it. Fixed by adding
   `#include <cstdint>` and `#include <vector>` to the header. This is exactly
   the kind of latent coupling a host-side isolated build catches earlier than
   a full-tree device build.

## How to extend this pattern to other modules

Good next candidates are other dependency-light, pure-logic units. Each gets
its own `test/hosttest/<module>/` dir with a `*_host_test.cpp` + a copy of the
runner adjusted for its sources. Modules that reach into IPC / media / ability
(e.g. TLV, which needs `message_parcel`, `pixel_map`, `want`, `ashmem`) need a
fake/mock seam first and are a larger effort.

The end goal: a top-level `test/hosttest/run_all.sh` that runs every host suite
and is wired into CI as the AI's verification entry point.
