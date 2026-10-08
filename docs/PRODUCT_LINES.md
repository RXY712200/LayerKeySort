# Product lines and release policy

LayerKeySort is **one repository with independently versioned products**, not one
product with interchangeable build modes. This policy describes repository-wide
naming and delivery; each product's own README and API contract remain authoritative.

## Products and locations

| Product | Sources, build and docs | Public identity | Status on 2026-10-08 |
| --- | --- | --- | --- |
| Full | Repository root: `include/`, `src/`, root `CMakeLists.txt`, `docs/` | `LayerKeySort`, `layerkeysort` | v4.0.0 Stable, already published |
| Mini | Self-contained `mini/` subtree | `LayerKeySort Mini`, `layerkeysort_mini` | [v1.0.0 Stable](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0) |

Full and Mini are **alternative implementations for different requirements**,
not a dependency hierarchy. Full offers larger ordering and snapshot workflows.
Mini deliberately uses a separate, lightweight doubly linked list, has its own
18-function contract and does not import Full production code. A consumer can
copy and build `mini/` without the repository root. Neither package's numeric
version implies compatibility with the other.

Do not move existing Full sources under `full/` merely for visual symmetry.
The repository root builds Full by default; `cmake -S mini` builds Mini.
A combined consumer may explicitly include both libraries, but neither is
implicitly linked into the other. Platform support (Windows, Linux, macOS) is
verified per product and toolchain; an OS is not a separate product/version.

## Independent version and global Git tag namespace

Each product follows its **own** semantic version and compatibility contract.
Never reset, rewrite, repoint or reuse a released tag.

| Product | Version displayed to consumers | Repository-level Git tag example |
| --- | --- | --- |
| Full | v4.0.0 | `v4.0.0` (retain existing historic `v*` tags) |
| Mini | v1.0.0 | `mini-v1.0.0` |

The existing root `v1.0.0` belongs to historical Full. It is **not** Mini's
v1.0.0 and must never be reused. New Mini tags always start with `mini-v`.
Future independently maintained products, if approved, must receive distinct tag
prefixes; a platform-specific binary variant does not need a new product tag.

Tags point to *whole-repository commits*, not to a directory. Record the exact
40-character source commit in each release. A Mini tag can point to a commit
that also contains Full; this does not make Full a Mini dependency. A release
can retain its original tag/commit even if another product advances later.

## Release pages and download artifacts

GitHub Releases are **repository-wide**. The Releases list and GitHub's
`Latest` designation cannot independently represent both products. The
individual root and Mini READMEs are the authoritative version selectors.
Release titles must start with `LayerKeySort` (Full) or `LayerKeySort Mini`
(Mini); link the exact tag, avoid ambiguous titles and verify `Latest`
behavior during publication. Do not silently replace the recommended Full
release with a Mini release.

Full keeps its existing release asset generator and prior release history.
Mini has its own explicit candidate packaging workflow and source-archive script.
Mini's **primary** download is a ZIP whose contents are only the tracked
`mini/` subtree, rooted at `LayerKeySort-Mini-vX.Y.Z/`, plus a separate
SHA-256 manifest. These archives are produced reproducibly from an exact Git
commit; their contents are verified against that commit. Packaging requires Git
with `archive --mtime` support and Python 3.9+; entry timestamps come from the
source commit and blob bytes are independent of local LF/CRLF checkout settings. GitHub's automatic
Source code ZIP/tarball snapshots the *whole repository* and is not the Mini-only
distribution. The archival ZIP is C17 source, not a platform-specific binary.

The Mini candidate tooling only **builds and uploads Actions artifacts for
review**. It never creates a tag, GitHub Release, or public distribution by
itself. Manual publication requires explicit authorization, completed CI
for the exact target SHA and review of release notes, manifest, version and
artifacts. Do not mark an unmerged or untagged candidate as released.

Mini v1.0.0 uses `mini-v1.0.0` and its [own release page](https://github.com/RXY712200/LayerKeySort/releases/tag/mini-v1.0.0).
Full v4.0.0 remains the repository-wide Latest; Mini is not designated Latest.

## Development, CI and review

Use one integration branch (`main`) with short-lived feature/release-preparation
branches. Do not maintain a permanent `full` or `mini` branch as an
alternative source of truth. Changes to Mini stay under `mini/`, except shared
navigation, scripts and CI. Full-specific changes remain in the root source
tree. Cross-product changes must be explicitly described and tested.

Full's original CI continues to validate Full. The separate Mini matrix tests
GCC, Clang, MSVC, AppleClang and instrumented Linux ASan/UBSan, plus standalone
extraction, consumers and optional coexistence. PR integration checks both;
path-based exclusions must not silently bypass either product's required checks.
Changes to a packaging script also require the corresponding package verification.
Candidate SHA results are evidence, not a publication decision.

Use product-qualified descriptions on PRs/Issues (`Full:`, `Mini:`, or
`Repo:`) and explain affected contracts. Existing historical Full Issue and
release references remain intact. Do not retroactively rename older releases,
edit unrelated issue histories or infer that an unimplemented product exists.

## Publication checklist for Mini releases

1. Reconcile the reviewed candidate with the integration commit and confirm
   the final target SHA; review all changed paths and ensure Full production
   files and frozen API have not changed unintentionally.
2. Require completed, passing Full and Mini CI and a successful Mini-only
   archive verification at that exact target SHA.
3. Confirm the intended Mini version and a new, unused `mini-v*` tag;
   never reuse the historical Full `v1.0.0`.
4. After explicit authorization only: merge, create an annotated Mini tag
   at the approved SHA, prepare `LayerKeySort Mini v1.0.0` release notes,
   attach the verified Mini ZIP and SHA256SUMS, and verify the release links.
5. Update README candidate wording and publication references in a separately
   reviewed change. Verify the repository-wide `Latest` marker still directs
   readers appropriately; record any limitation transparently.

No merge, tag, GitHub Release or Issue change is authorized by this document.
