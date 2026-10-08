# Mini changelog

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
