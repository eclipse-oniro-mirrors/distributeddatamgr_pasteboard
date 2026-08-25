# Host-side test loop — PasteBoardTime (shallow-dependency sample)

Host-runnable unit test for `utils/native/src/pasteboard_time.cpp` (wall/boot
time helpers over POSIX clocks). No device, no IPC, no service.

Dependency footprint: stdlib + POSIX (`<sys/time.h>`, `<ctime>`) plus one
transitive include — the header's `singleton.h` from `c_utils`, which is
header-only and host-compilable. It is reached with an **include path**
(`-I .../c_utils/base/include`), so unlike the eventcenter sample this module
needs **no shim** at all. This is the lightest tier of host-testability.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract as the other suites (0 pass, 1 test fail, 2 coverage
below gate, 3 build error). Knobs: `COVERAGE_MIN` (default 90), `CXX`, `GCOV`.

Current status: **4 tests, 92.86% line coverage**.

## Note on the coverage number

Coverage is 92.86%, not 100%, and that is intentional/honest. The single
uncovered line is the `clock_gettime(...) < 0` error return in
`GetTimeMsByClockId` — only reachable if the syscall itself fails, which does
not happen on a healthy host. Forcing it to 100% would require faking
`clock_gettime`, which is more machinery than this error branch warrants. The
gate (90%) is set to accept genuine coverage rather than pushing authors to game
unreachable branches.

## Findings surfaced while building this loop

1. **Spurious include (not fixed, low value).** `utils/native/include/pasteboard_time.h`
   includes `singleton.h` but never uses the singleton template. It is harmless
   and possibly there for a planned API, so it was left as-is; noted here so the
   c_utils include-path dependency in this runner is understood as incidental,
   not essential.
