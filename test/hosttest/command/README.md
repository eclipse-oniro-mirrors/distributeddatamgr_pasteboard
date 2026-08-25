# Host-side test loop — Command (pure logic + a real finding)

Host-runnable unit test for `services/dfx/src/command.cpp` (the CLI
argument/help/action helper used by dump commands). No device, no IPC. Pure
logic — no shim, no fake needed.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract as the other suites. Current status: **7 tests,
94.74% line coverage**.

## Finding surfaced while building this loop

**`services/dfx/src/command.h` was missing includes (FIXED).** It uses
`std::function` and `std::vector` but only `#include <string>`. Same latent-
coupling class as the earlier findings in `serializable.h`,
`concurrent_map.h`, `event_center.h` — compiled on-device only via transitive
includes. Fixed by adding `#include <functional>` and `#include <vector>`.
This is the 5th instance of this bug class found by host-isolating a module;
see `../README.md` for the running list.

Note: `command.h` lives beside its `.cpp` in `services/dfx/src/` (not an
`include/` dir), which is why the runner's `CMD_INC` points there.
