# Host-side test loop — ClipPlugin / DefaultClip (registry + Serializable)

Host-runnable unit test for `framework/framework/clip/clip_plugin.cpp` and
`default_clip.cpp`. No device, no IPC.

Covers three things:
- the **plugin registry** (`RegCreator` / `CreatePlugin` / `DestroyPlugin`,
  including the null-factory, duplicate-name, unknown-name-falls-back-to-default,
  and factory-delegation branches),
- the `ClipPlugin` **base-class default virtuals** (exercised through a
  `FakeClip` that inherits them, plus `DefaultClip`'s own overrides), and
- `GlobalEvent`'s **Serializable** Marshal/Unmarshal round-trip (links the real
  serializable.cpp).

Seam: single-header shims under `shim/` for `pasteboard_hilog.h` (device logging)
and `pasteboard_event_dfx.h` (the `RADAR_REPORT` macro / hisysevent). Shim dir is
first on the include path.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract. Current status: **16 tests, 100% combined line
coverage** (clip_plugin.cpp + default_clip.cpp).

## Findings surfaced while building this loop

1. **`clip_plugin.h` was missing includes (FIXED).** It uses `std::function`
   (the `DelayDataCallback` etc. typedefs), `std::pair` and `std::vector` but
   only `#include <map>` + serializable.h. Compiled on-device via transitive
   includes. Fixed by adding `<functional>`, `<utility>`, `<vector>`. This is the
   6th instance of the missing-include bug class; see `../README.md`.

2. **C++ name-hiding (not a bug, a test note).** `DefaultClip` overrides the
   `(topN, user)` overload of `GetTopEvents` and `Clear(int32_t)`, which hides
   the base's single-argument overloads on a `DefaultClip` object. The tests
   reach the hidden base overloads through a `ClipPlugin&` reference — worth
   knowing when writing tests against this hierarchy.
