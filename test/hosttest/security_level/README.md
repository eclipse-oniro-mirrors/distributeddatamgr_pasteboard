# Host-side test loop — SecurityLevel (deep dep, DEVSL fakes)

Host-runnable unit test for `adapter/security_level/security_level.cpp` (decides
whether the device's data-security level supports distributed pasteboard). No
device, no dataclassification service.

## Seam

SecurityLevel sits behind two platform dependencies:
- the **DEVSL C API** (`dev_slinfo_mgr.h`, dataclassification) —
  `DATASL_OnStart` / `DATASL_GetHighestSecLevel` / `DATASL_OnStop`, and
- **DMAdapter** (`device/dm_adapter.h`, device_manager) —
  `GetLocalDeviceUdid()`.

`fakes/` provides both. The DEVSL fake exposes `hosttest_devsl::g_result` and
`g_level`, and the DMAdapter fake exposes `hosttest_dm::g_udid`, so every branch
(high/low level, query failure, empty-udid init failure, cached-level
short-circuit) is driven deterministically. The private level getters are
reached through the public `IsSupportedDistributed(bool)`. Fakes dir is first on
the include path.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract. Current status: **7 tests, 100% line coverage**.

## Findings surfaced while building this loop

1. **`security_level.cpp` was missing `<vector>` (FIXED).** It uses
   `std::vector<uint8_t>` in `InitDEVSLQueryParams` but neither the `.cpp` nor
   `security_level.h` included `<vector>` — it came transitively via the DEVSL /
   dm_adapter headers on-device. This is the **8th** instance of the
   missing-include bug class, and the **first found in a `.cpp`** (the earlier 7
   were all in headers). Added `#include <vector>`. See `../README.md`.

2. **Faithful-fake subtlety.** The first draft of the DEVSL fake wrote the
   injected level into the out-param unconditionally; the real API only writes
   on success. A test (`DevslFailureTreatedAsLevelZero`) caught the divergence —
   the fake was corrected to write only when it returns `DEVSL_SUCCESS`. Worth
   remembering: a fake has to match the *contract on failure paths* too, not
   just the happy path.
