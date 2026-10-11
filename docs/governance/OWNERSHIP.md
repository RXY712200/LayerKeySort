# LayerKeySort repository ownership and scope enforcement

Repository-wide **policy**, not a change to either product's public API. The machine-readable source of truth is [ownership.json](ownership.json). [scope-exceptions.json](scope-exceptions.json) starts empty.

## Product areas

- `full`: root Full C17 sources, public header, legacy/regression tests, benchmarks, examples, root CMake and Full-specific documentation and CI.
- `mini`: `mini/**`, two explicitly listed Mini packaging scripts, Mini CI and release candidate workflow.
- `consumer`: `consumers/layer-order-workbench/**` plus Workbench CI/evidence workflows. A Workbench defect is not automatically a Mini library defect.
- `repo`: product-family README and navigation, the shared product policy, contribution/license metadata, CODEOWNERS, PR template, governance policy and guard workflows. The special `docs/FULL.*` entry pages and `docs/index.html` are classified `repo` for the initial multi-product navigation migration; **Full retains technical review responsibility for their Full claims**.

Unknown paths, ambiguous/malformed paths, missing changed-file pages, unsafe rename classifications and cross-owner sets without a **previously committed, exact-SHA authorization** are rejected by default. Path ownership is determined by exact match, else the longest matching declared prefix; it is *not* taken from PR title/label/body.

A normal PR should touch files in only one owner class. For exceptionally necessary cross-product PRs, the real repository owner must FIRST approve a separate `repo`-only governance PR adding an exception to `scope-exceptions.json` that binds the exact PR number, head commit, base commit and changed-file inventory. The exception is read from the **protected target branch**, never from the proposed PR. Updating the target branch can invalidate an exception; re-approval then requires a new trusted record.

## Trust boundary

The authoritative Scope Guard runs on `pull_request_target` with read-only repository and PR permissions and checks out ONLY the trusted `main` base SHA. It never executes or checks out the PR head. The GitHub API changed-file list is treated as untrusted data and must be complete.

Changes to the guard or policy itself are still reviewed using the **base's** guard, never the new PR code. However, after a governance PR merges, the new program becomes the future policy. Therefore these changes require manual review by the human owner and test evidence; CODEOWNERS does not provide a second GitHub identity or independently enforce that review. Administrators and Codex accounts using the same real GitHub identity share authority.

GitHub's public-repository event policy may block `pull_request_target` from 2026-11-02. **Do not activate a Ruleset requiring `scope-guard` until GitHub event policy explicitly permits the trusted workflow and a real PR confirms its check runs on the required SHA**. Do not treat a skipped, absent, neutral, or locally simulated check as proof of real enforcement.

## Checks and release constraints

The proposed required merge checks are `full-gate`, `mini-gate`, `workbench-gate` and `scope-guard`. Their upstream matrix jobs must all complete successfully, not merely be skipped. Rulesets are an **additional** platform setting: source files alone do not enforce PR-only merging or protected tags.

- Do not touch the frozen Full 4.x public API, LS1 and LKS4SNP1 formats as part of governance.
- Do not touch Mini 1.x's 18 API functions, source implementation or released contract.
- Do not rewrite historic tags, releases, benchmark measurements, issue histories or asset checksums.
- Workbench PR #9 must remain independent, and any new CI requirements should be enabled only after verified transition.
- `repo` is a small path class, **not** a license to modify Full, Mini, or Workbench production code.

CODEOWNERS maps paths to the *real* account `@RXY712200`; the Full and Mini ChatGPT conversations are independent architecture advisors, not two separate GitHub reviewers.
