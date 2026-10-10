# Phase 1 validation record and evidence boundary

Reviewed dependency: Mini v1.0.0, annotated tag `mini-v1.0.0`, release commit
`3f52798b83b6580aa4b89846b6403b0bd7b15f0a`. Repository main was re-fetched before
creating the consumer branch; Full/Mini production files are not modified.

## Executed local checks (2026-10-10)

Windows x64, MSYS2 UCRT64 GCC 16.2.0, CMake 4.4.4, MinGW Makefiles,
Python 3.13.14. Debug and Release compile C17 with strict warnings as errors.
CTest executed all six consumer tests successfully; ordinary tests are not
sanitizer evidence. Actual Linux sanitizer/platform results must be read from
the exact-SHA workflow links in the Draft PR/completion report.

Three distinct authored editor-style scripts were actually executed and saved:

| Script / saved trace | Commands | Final expected IDs | Provenance |
| --- | ---: | --- | --- |
| [layer-composition](../scenarios/layer-composition.commands) / [trace](../scenarios/evidence/layer-composition.trace) | 32 | 1,7,6,4,5 | Prewritten script executed by Codex/CTest |
| [overlay-reordering](../scenarios/overlay-reordering.commands) / [trace](../scenarios/evidence/overlay-reordering.trace) | 31 | 8,7,4,3,2,1 | Prewritten script executed by Codex/CTest |
| [placeholder-recovery](../scenarios/placeholder-recovery.commands) / [trace](../scenarios/evidence/placeholder-recovery.trace) | 30 | 5,4,7,1,6 | Prewritten script executed by Codex/CTest |

Generated fixtures reach 16/128/2,048 live occurrences and execute respectively
164/276/2,197 commands including reorder, retirement and reinsertion. These are
bounded generated engineering tests, not real application trace distributions.
CTest emits summaries and complete `.trace` files under each build's `evidence/`;
CI uploads them as artifacts. Three real-human/editor-user sessions are **not**
claimed by the scripts. Terminal prompting and provenance validation are separate
application paths, not proof of adoption.

An additional [23-command terminal capture](../scenarios/evidence/codex-terminal.trace)
was executed by **Codex automation** through a Windows PowerShell ConPTY session:
the process received a real terminal, displayed prompts, and commands were sent
through the session's stdin in one automated batch. Its source is
`codex-interactive`, mode `stdin`; it replayed successfully. It is neither human
typing nor an external user's spontaneous editor session. CTest replays this
artifact too; no completion of three human sessions is claimed.

The array oracle checks after every command, including legal errors/no-ops. Units
deliberately change the real Mini order outside the command path and verify that
the checker rejects the divergence with expected/actual ID diagnostics. That
expected unit-test diagnostic is intentional, not a Mini bug. Foreign-order
tests use a second live Mini order and only supported public API calls.

Final consumer review reproduced an input-reader boundary defect: a valid
255-byte command followed by CRLF was classified as overlong because CR was
counted before line-ending normalization. The consumer reader now resolves the
ending before checking length and removes exactly one final CR. Focused 255-byte,
256-byte and double-CR cases are part of the errors/replay CTest. This is a
consumer correction, not a Mini implementation or public-contract change.

## Allocation-failure method

The official ZIP and SHA-256 manifest were downloaded afresh and checked against
their pinned published digests. A copy of only the consumer directory plus that
extracted Mini package, both outside the repository checkout, configured and
built independently and passed all six local CTest tests. Direct GCC compilation
of the application against the public Mini header and built target archive also
replayed the saved 32-command composition trace successfully.

Isolated test executable only, on Linux GNU-compatible linkers/MinGW:
`--wrap=malloc --wrap=calloc --wrap=free`. It links the unchanged real Mini static
library and consumer core; it does not include private implementation/header files.
`calloc` is also wrapped because optimizers can replace zero-initialized allocations.
The next allocation is failed during Mini order creation or one of the four
consumer insertion commands. Successful retries and zero tracked Mini allocations
after destruction are checked. Tracking is restricted to those calls; runtime
stdio/CRT allocations are not mistaken for Mini leaks. This is not all-process
allocation accounting or allocator-inclusive memory measurement.

MSVC/AppleClang use their normal public-API tests without this linker-specific
injection mechanism. Linux CI is the supported instrumented ASan/UBSan environment.
The shipped command-line program has no test allocator hook or OOM command.

## Consumer costs and unfinished roadmap evidence

No production Mini defect is established by this work. Application limitations
include fixed capacities, object retention until exit, linear ID/order lookups,
full-sequence trace sizes, and per-command oracle verification. Payload ownership
and ID-to-handle retirement are application responsibilities rather than features
delegated to Mini.

No latency benchmark, representative external-user traces, process RSS or complete
allocator overhead measurement, Full adapter comparison or snapshot workflow is
claimed. Future measurement must separate Mini operations, application mapping,
oracle work and trace/UI overhead. Do not attribute total verification-run time
to Mini performance or declare either product superior.

[Issue #7](https://github.com/RXY712200/LayerKeySort/issues/7) remains unchanged:
Phase 1 provides command integration, script evidence, bounded replay/model checks
and qualified test-only OOM/foreign-order evidence. Human sessions, representative
workload evidence and the performance/memory portions of its acceptance checklist
remain pending. Optional Full-only adapters belong to separately scoped work.
