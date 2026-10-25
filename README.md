# LayerKeySort

English | [简体中文](README.zh-CN.md)

**A family of independent C17 libraries for maintaining the order of changing collections.**

LayerKeySort maintains *live sequence order* as applications insert, remove, and move occurrences. It is not a replacement for one-shot array sorting. The application owns its items and business identities; each product defines its own order-management contract.

## Choose a product

| | **[Full](docs/FULL.md)** | **[Mini](mini/README.md)** |
| --- | --- | --- |
| Latest published Stable | **[v4.1.0](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0)** | **[v1.0.0](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0)** |
| Current `main` source | **4.1.0** ([released update](docs/V4_1_0.md)) | **1.0.0** (unchanged) |
| Best fit | More complete ordering and historical workflows | Lightweight local, mutable sequences |
| Live ordering | Relative operations, stable resident handles, comparator-managed option | Relative operations and stable occurrence handles |
| Arbitrary live-order comparison | Available under the Full contract | O(n) worst case |
| Snapshots and persistence | Immutable snapshots, LS1 keys, explicit wire format and restore | Not provided |
| Group and Batch utilities | Provided | Not provided |
| Core implementation | Bounded blocks and balanced index | Independent non-intrusive doubly linked list |
| Public API | 64 functions (Full v4) | 18 functions (Mini v1) |
| Build from this repository | Root `CMakeLists.txt` | `mini/CMakeLists.txt` |

**Full and Mini are independent products, not parent-and-child dependencies.** Their version numbers, public APIs, compatibility promises and release cycles are separate. Neither library links to the other's production source.

These are **capability distinctions, not benchmarks or claims that one product is universally faster**. Choose Mini when its narrower contract is enough; choose Full when its additional ordering, snapshot or batch capabilities are required.

## Start with the product you need

| Product | Overview and usage | API / contract | Download |
| --- | --- | --- | --- |
| Full | [Full README](docs/FULL.md) · [Integration](docs/INTEGRATION.md) · [Examples](examples/README.md) | [API](docs/API.md) · [V4 frozen contract](docs/V4_FREEZE.md) | [Published v4.1.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/v4.1.0) · [4.1.0 changes](docs/V4_1_0.md) |
| Mini | [Mini README](mini/README.md) · [Usage](mini/docs/USAGE.md) · [Example](mini/examples/basic.c) | [Mini v1 public contract](mini/docs/CONTRACT.md) | [Mini v1.0.0 release](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0) |

For Mini, use the release's **Mini-only source ZIP**; GitHub's automatic Source code archive downloads the **whole repository**. Full publishes a separate two-file amalgamation asset. Follow the chosen product's instructions; the root CMake build remains Full by default.

## Repository layout

- `include/`, `src/`, root `CMakeLists.txt`, `tests/`, `benchmarks/` and most of `docs/`: **Full**.
- `mini/`: independent **Mini** implementation, documentation, tests and build.
- `consumers/layer-order-workbench/`: **Mini consumer application and validation**, not Mini production code or a third ordering library.
- `.github/`, selected `tools/` and [product-line policy](docs/PRODUCT_LINES.md): repository-level integration and product-specific maintenance tooling.

A possible Embedded/MCU edition is **only a future concept**. No Embedded product, API or release is currently offered.

## Versions, compatibility and maintenance

- Full's published tags use `v*` (current published Stable: `v4.1.0`).
- Mini's published tags use `mini-v*` (current Stable `mini-v1.0.0`).
- GitHub's repository-wide **Latest** designation is not a per-product version selector. Use each product's explicit release link.
- Old Full releases and V1–V4 research remain available as history; they are not Mini contracts.
- License: [MIT](LICENSE). Each product keeps its own documentation and validation requirements.

Read [product-line and release policy](docs/PRODUCT_LINES.md) ([简体中文](docs/PRODUCT_LINES.zh-CN.md)) and [contribution guidelines](CONTRIBUTING.md) before cross-product changes.

## Roadmaps and evidence

- **Full:** [Full engineering roadmap and completed three-step optimization cycle, Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3); [historical problem ledger, Issue #4](https://github.com/RXY712200/LayerKeySort/issues/4).
- **Mini and its consumers:** [Mini roadmap, Issue #7](https://github.com/RXY712200/LayerKeySort/issues/7).
- **Full examples and validation:** [Full README](docs/FULL.md) · [Full validation](docs/VALIDATION.md).
- **Mini examples and validation:** [Mini README](mini/README.md) · [Mini development](mini/docs/DEVELOPMENT.md).

## External project directory

- [Quiveld — author's independent projects directory](https://quiveld.com/projects/independent/).

Do not infer real-world adoption, universal platform support, or performance superiority from a feature table. Evaluate your application's actual ordering and resource requirements.
