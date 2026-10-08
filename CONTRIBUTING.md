# Contributing to LayerKeySort

## Before opening a change

- Understand the affected public semantics before changing implementation.
- Keep the scope focused.
- Avoid unrelated formatting changes.

## Products, building and versions

The root build and [development guide](docs/DEVELOPMENT.md) describe **Full**.
For **Mini**, use [mini/README.md](mini/README.md) and
[mini/docs/DEVELOPMENT.md](mini/docs/DEVELOPMENT.md). See the
[product-line and release policy](docs/PRODUCT_LINES.md) before changing tags,
release packaging or shared workflows. Full and Mini have independent version
and API contracts. Review and run checks for both when a change crosses lines.
For Full, consult the [validation guide](docs/VALIDATION.md).

## Testing

Add or update relevant regression tests and run them before opening a change. Report which tests and configurations you ran.

## Public API changes

Public API changes should be deliberate and documented. Describe behavior and compatibility impact in the change. For Full, review the active V4 contract in [docs/V4_FREEZE.md](docs/V4_FREEZE.md)
and [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md). For Mini, review
[mini/docs/CONTRACT.md](mini/docs/CONTRACT.md). Do not apply one product's
compatibility claims to the other.

## Pull requests

Include a concise description, the reason for the change, tests performed, and behavior or compatibility impact.

Release-facing changes also need a complete documentation and presentation
review, including examples, diagrams, and GitHub Pages navigation. The
[development guide](docs/DEVELOPMENT.md) describes the Stable-release check.
