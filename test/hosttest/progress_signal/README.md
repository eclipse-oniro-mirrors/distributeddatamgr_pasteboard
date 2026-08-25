# Host-side test loop — ProgressSignalClient (atomic state machine)

Host-runnable unit test for `framework/innerkits/src/pasteboard_progress_signal.cpp`
(the copy/paste-progress cancel-signal singleton). No device, no IPC.

The unit is an atomic cancel-state machine (`Init` / `Cancel` /
`SetRemoteTaskCancel` / `IsCanceled` / `CheckCancelIfNeed`). Its only external
include is unused (see finding 2), so the suite shims that empty and pulls
`nocopyable.h` from c_utils via an include path — no real platform dependency.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract. Current status: **6 tests, 100% line coverage**.

`ProgressSignalClient` is a process-wide singleton, so each test `Init()`s it
back to a known state in `SetUp()` to stay independent.

## Findings surfaced while building this loop

1. **`pasteboard_progress_signal.h` was missing includes (FIXED).** It uses
   `std::atomic_bool` and the `DISALLOW_COPY_AND_MOVE` macro but only included
   `api/visibility.h`. Compiled on-device via transitive includes. Added
   `#include <atomic>` and `#include "nocopyable.h"`. This is the **9th**
   instance of the missing-include bug class — see `../README.md`.

2. **Unnecessary heavy include (noted, not changed).** The `.cpp` `#include`s
   `distributed_file_daemon_manager.h` but references nothing from it — an
   unused include that drags in the dfs_service dependency chain on-device. The
   suite shims it empty. Removing it from the `.cpp` would be a safe cleanup
   (an include removal is slightly riskier to do blind than an include add, so
   it's left for a maintainer to confirm), but it is dead weight.
