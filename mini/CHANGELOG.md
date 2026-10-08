# Mini changelog

## v1.0.0-preview.3 — 2026-10-08

- Preserve all 18 public APIs and the production doubly linked list unchanged.
- Add an independent array model linking the production library: four fixed
  xorshift32 seeds, 24,000 total operations, three orders, bounded live sizes.
- Add adversarial sequences, full observable checks and comparison properties.
- Extend private invariant checks, six-node moves and a live allocation ledger.
- Add 24 systematic single-allocation failure/recovery cases and a 190-case
  legal-input error/default/precedence matrix, including synthetic capacity.
- Record strict Windows GCC Debug/Release, static analysis and extraction checks;
  ASan/UBSan probe fails due to absent libraries. Other platforms remain unverified.
- No production correctness defect found; no production algorithm fixes needed.
- Leave cross-platform CI/integration and Stable validation to later milestones.

## v1.0.0-preview.2 — 2026-10-08

- Complete the 18-function public surface with four stable-handle move functions
  and live-order comparison returning exactly -1, 0 or +1.
- Reconnect existing nodes in O(1), with validated self/endpoint/adjacent no-ops;
  preserve occurrence identity, ownership, payload and size.
- Compare current order in O(n) worst case without payload/address ordering.
- Guarantee no allocation/free during movement or comparison, including errors.
- Extend public/private tests and the example; update standalone documentation.
- Leave randomized modeling and systematic fault campaigns for Preview.3.

## v1.0.0-preview.1 — 2026-10-08

- Introduce the independent C17 `layerkeysort_mini` static library and opaque
  order/occurrence model using a non-intrusive doubly linked list.
- Implement exactly 13 foundation APIs: lifetime, size/endpoints/neighbors/item,
  front/back/before/after insertion and occurrence removal.
- Support stable live handles, duplicate and NULL payloads, caller-owned items,
  wrong-order rejection, safe error outputs and atomic allocation failure.
- Add standalone CMake builds, public/internal CTest tests and a runnable example.
- Document the public contract, ownership, usage, architecture, development,
  complexity and preview limitations in an independent project distribution.

Movement and comparison are reserved for Preview.2; they are not available here.
