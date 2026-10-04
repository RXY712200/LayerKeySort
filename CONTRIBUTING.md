# Contributing to LayerKeySort

## Before opening a change

- Understand the affected public semantics before changing implementation.
- Keep the scope focused.
- Avoid unrelated formatting changes.

## Building

Follow the [development guide](docs/DEVELOPMENT.md) for the verified Visual Studio / MSVC build configurations.

## Testing

Add or update relevant regression tests and run them before opening a change. Report which tests and configurations you ran.

## Public API changes

Public API changes should be deliberate and documented. Describe behavior and compatibility impact in the change. Review the active 3.x contract or historical 2.x contract in [docs/COMPATIBILITY.md](docs/COMPATIBILITY.md) before proposing one.

## Pull requests

Include a concise description, the reason for the change, tests performed, and behavior or compatibility impact.

Release-facing changes also need a complete documentation and presentation
review, including examples, diagrams, and GitHub Pages navigation. The
[development guide](docs/DEVELOPMENT.md) describes the Stable-release check.
