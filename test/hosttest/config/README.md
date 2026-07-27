# Host-side test loop — Config (composition sample)

Host-runnable unit test for `services/load/src/config.cpp`. No device, no IPC.

This is the **composition** sample: `Config` and `Config::Component` are
`Serializable` subclasses, so this suite links the real `config.cpp` against the
real `serializable.cpp` + cJSON — no shim, no fake. It shows a host suite built
on top of another already-host-tested module, and exercises the base
`Serializable` `vector<T>` / nested-object template paths through a production
type.

Coverage is measured on `config.cpp` only (serializable.cpp is a linked
dependency, already covered by its own suite).

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract. Knobs: `COVERAGE_MIN` (default 90), `CXX`, `GCOV`.
Current status: **6 tests, 100% line coverage** on config.cpp.

## Finding surfaced while building this loop (the important one)

Building this suite **found a real crash in production code**: marshalling a
`Config` with a non-empty `std::vector<std::string>` field segfaulted.

Root cause: `Serializable::Marshall()` declared `json node;` — an
**uninitialized `cJSON*`**. The first `SetValue` → `SetValueToObject` calls
`cJSON_IsObject(node)`, dereferencing that garbage pointer. It is undefined
behaviour: benign in some code paths / build flags, a segfault in others (plain
`-O0` crashed; ASan/UBSan builds masked it by changing memory layout).

Fix (1 line, committed): `json node = nullptr;` — `cJSON_IsObject(nullptr)` is
defined to return false, so the object is created correctly.

Impact: this path is hit by **every** `Serializable` subclass's `Marshall()`,
so the bug was latent across all config/serialized-data marshalling. The
serializable suite gained a `VectorStringMarshallRegression` test for the same
path (see its file for the note on UB non-determinism).
