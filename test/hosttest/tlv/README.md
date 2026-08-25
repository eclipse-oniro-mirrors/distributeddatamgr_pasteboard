# Host-side test loop — TLVUtils (deep-dependency sample, with fakes)

Host-runnable unit test for `framework/tlv/tlv_utils.cpp`. This is the
**deep-dependency** proof: unlike the shallow samples, TLVUtils sits behind
several heavy OHOS platform types, so it builds against a set of minimal
**fakes** under `fakes/` instead of a device.

## Why fakes (not just include paths)

`tlv_utils.{h,cpp}` needs:

| Real dependency        | Why it's hard host-side              | Fake in `fakes/`      |
|------------------------|--------------------------------------|-----------------------|
| `parcel.h` (c_utils)   | `Parcel`/`Parcelable` tied to RefBase serialization | `parcel.h` — real byte buffer |
| `pixel_map.h` (image)  | heavy image object                   | `pixel_map.h` — blob round-trip |
| `unified_meta.h` (udmf)| pulls a UDMF type chain              | `unified_meta.h` — `API_EXPORT` + fwd-decl |
| `pasteboard_hilog.h`   | OHOS hilog platform lib              | `pasteboard_hilog.h` — no-op + control-flow macros |

`securec` is **not** faked: the suite compiles and links the real
bounds-checking library from `third_party/bounds_checking_function`, so the safe
functions the unit calls are the sanctioned ones rather than test-local
substitutes.

The `fakes/` dir is put on `-I` **first** so these shadow the real headers. The
fakes are faithful, not mocks: e.g. the fake `Parcel` actually buffers bytes, so
`Parcelable2Raw -> Raw2Parcel -> Unmarshalling` round-trips through the real
TLVUtils code path (allocate + copy + parse), not a stubbed call graph.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract as the other suites. Knobs: `COVERAGE_MIN` (default 90),
`CXX`, `GCOV`. Current status: **12 tests, 97.22% line coverage**.

## Reaching the error branches

`TLVUtils::Raw2Parcel` has three defensive error branches. Two are reachable
with nothing but a hostile input, because the tests own the `RawMem` struct:

- `bufferLen = SIZE_MAX` — no allocator can satisfy it, so the allocation
  branch is taken.
- `bufferLen = 0x80000000` — above securec's `SECUREC_MEM_MAX_LEN`, so the safe
  copy is rejected before any byte moves.

The third needs a seam: `fakes/fault_inject.h` exposes `g_forceParseFromFail`,
which the fake `Parcel::ParseFrom` consults, letting the test drive the
parse-failure branch deterministically. This kind of error-path coverage is
essentially impossible on a device — it is a direct benefit of the fake seam.

Note the preference order: drive a branch with real inputs where you can, and
only reach for an injected fault where you cannot. Faking a *safe function*
(`memcpy_s`) to force a failure would mean shipping a test-local substitute for
a sanctioned security API — worse than leaving the line uncovered.

## Reuse note

This is the template for any module behind heavy platform types: identify the
*exact* surface the unit calls (often small), write faithful fakes for just that
surface, add fault-injection switches where you need to reach error branches,
and put `fakes/` first on the include path. Compare with `../eventcenter/`
(single-header shim) and `../pasteboard_time/` (include path only) for the
lighter tiers.
