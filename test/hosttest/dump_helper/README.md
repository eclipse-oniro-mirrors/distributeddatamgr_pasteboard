# Host-side test loop — PasteboardDumpHelper (composition sample)

Host-runnable unit test for `services/dfx/src/pasteboard_dump_helper.cpp` (the
`hidumper`-style command registry/dispatcher). No device, no IPC.

Composition: links the real `command.cpp` (already host-tested in
`../command/`). `Dump()` writes to a raw file descriptor via `dprintf`, so the
tests route it through a private `mkstemp` file (unlinked immediately, so the fd
is the only handle) and read the captured output back — no special I/O seam
needed, just POSIX.

## Run it

```bash
./run_host_test.sh
```

Same exit-code contract. Current status: **6 tests, 100% line coverage**.

## Finding surfaced while building this loop

**`pasteboard_dump_helper.h` was missing `<memory>` (FIXED).** It uses
`std::shared_ptr<Command>` but only included `<map>` + `command.h`. On-device
build only worked via transitive includes. This is the **7th** instance of the
missing-include bug class found by host-isolating a module — see
`../README.md`'s running list, and the note there that this now looks like a
systemic pattern worth a project-wide header audit.
