# Host-side test loop — EventCenter (second sample, with a fake seam)

Host-runnable unit test for `framework/framework/eventcenter/` (the sync/async
event dispatcher: `event.cpp` + `event_center.cpp`). Runs with no device, no
IPC, no service.

This is the second host-test instance and the first that needs a **fake seam**:
`event_center.cpp` depends on `pasteboard_hilog.h`, which pulls in the OHOS
`hilog` platform library. `shim/pasteboard_hilog.h` replaces it with a host stub
that drops the logging but **preserves the control-flow semantics** of
`PASTEBOARD_CHECK_AND_RETURN_LOGE` (it still `return;`s on a false condition),
so the behaviour under test is unchanged.

See `../serializable/README.md` for the full rationale and toolchain notes.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract as the serializable sample: `0` pass+coverage,
`1` test fail, `2` coverage below gate, `3` build error. Knobs: `COVERAGE_MIN`
(default 90), `CXX`, `GCOV`.

Current status: **9 tests, 94.44% combined line coverage** across event.cpp +
event_center.cpp.

## Layout

- `event_center_host_test.cpp` — tests: sync dispatch, fan-out, null/no-subscriber,
  unsubscribe, and the `Defer` async-queue flush path.
- `shim/pasteboard_hilog.h` — the fake seam (host stub for the logging header).
- `run_host_test.sh` — build + run + combined coverage gate.

## Findings surfaced while building this loop

1. **Missing includes in two headers (FIXED).** Same latent-coupling class as
   the serializable finding:
   - `framework/framework/include/common/concurrent_map.h` used `std::function`
     but included only `<unordered_map>` / `<mutex>`.
   - `framework/framework/include/eventcenter/event_center.h` used
     `std::function` / `std::deque` / `std::unordered_map` but included only
     `<list>` / `<queue>`.
   Both compiled on-device only via transitive includes. Fixed by adding the
   missing standard headers. Isolated host builds surface these immediately.

## Fake-seam pattern (how to reuse)

When a module reaches a device-only dependency used *shallowly* (logging,
a trace macro, a single helper), put a minimal stand-in under `shim/` and add
`-I shim` **before** the real include dirs so it shadows the real header. Keep
any control-flow semantics identical. For deep dependencies (real IPC, media,
ability), a shim is not enough — those need a proper mock/fake object and are a
larger effort (e.g. TLV -> message_parcel/pixel_map/want).
