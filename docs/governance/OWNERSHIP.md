# LayerKeySort repository ownership and scope enforcement

Repository-wide **policy**, not a change to either product's public API. The machine-readable source of truth is [ownership.json](ownership.json). [scope-exceptions.json](scope-exceptions.json) starts empty.

## Product areas

- `full`: root Full C17 sources, public header, legacy/regression tests, benchmarks, examples, root CMake and Full-specific documentation. Full CI and release workflows are technically maintained by Full but belong to the protected **repo** control-plane path class.
- `mini`: `mini/**`, two explicitly listed Mini packaging scripts, Mini CI and release candidate workflow under the same protected **repo** workflow-control plane (technical owner remains Mini).
- `consumer`: `consumers/layer-order-workbench/**` ; Workbench CI/evidence workflows remain technically owned by Mini/Workbench but have **repo** path class to prevent product PRs from silently weakening validation. A Workbench defect is not automatically a Mini library defect.
- `repo`: product-family README and navigation, the shared product policy, contribution/license metadata, CODEOWNERS, PR template, governance policy and **all explicitly listed GitHub Actions workflow definitions**, even when a workflow is technically owned by Full, Mini or Workbench. The special `docs/FULL.*` entry pages and `docs/index.html` are classified `repo` for the initial multi-product navigation migration; **Full retains technical review responsibility for their Full claims**.

Unknown paths, ambiguous/malformed paths, missing changed-file pages, unsafe rename classifications and cross-owner sets without a **previously committed, exact-head-SHA authorization** are rejected by default. Path ownership is determined by exact match, else the longest matching declared prefix; it is *not* taken from PR title/label/body.

A normal PR should touch files in only one owner class. In particular, changing both product source code and its CI workflow is a cross-class change and needs a previously authorized exact-file exception or two separate PRs. For exceptionally necessary cross-product PRs, the real repository owner must FIRST approve a separate `repo`-only governance PR adding an exception to `scope-exceptions.json` that binds the exact PR number, **head commit SHA** and sorted changed-file inventory. **Do not record `base_sha` in the exception:** the authorization PR must be merged into `main` first, so the final base SHA cannot be known without self-reference. The exception is read from the **protected target branch**, never from the proposed PR. Independently, on every run, `read_pr_inventory()` verifies the GitHub PR number, head SHA, base branch, base SHA from the event, and complete file list against the live API before approving. A base-branch update does not by itself invalidate an unchanged authorized head, but any PR head update or changed inventory requires a new reviewed exception.

### Exact cross-product authorization sequence

1. Open the cross-product PR and stop changing its head commit. Record its PR number, full 40-character HEAD SHA, and sorted `changed_inventory()` entries (including rename sources and statuses).
2. Open and separately review a **repo-only** PR adding the exact `{"pr_number": ..., "head_sha": ..., "files": [...]}` exception. No `base_sha` key is permitted.
3. Merge the governance authorization into `main` by the authorized normal path. It is important that this approval is independent of the cross-product PR.
4. Re-run the trusted scope check against the current `main`. It validates the current base identity dynamically and the unchanged PR head/inventory against the precommitted exception.
5. If the feature PR is rebased, updated with `main`, or its file inventory changes, its HEAD changes and the exception **fails closed**. Repeat authorization from the new final HEAD.

**Do not enable strict "Require branches to be up to date" for this exception design.** Updating a cross-product PR's head after the exception commit moves `main` creates a second HEAD-SHA authorization cycle. Start with loose required status checks and independent merge-commit regression checks, then redesign the exception mechanism if strict-up-to-date is essential. A loose-check policy is not proof that an outdated PR is safe to merge; exact head + live base identity + full CI validation are still required.

## Trust boundary

The authoritative Scope Guard runs on `pull_request_target` with read-only repository and PR permissions and checks out ONLY the trusted `main` base SHA. It never executes or checks out the PR head. The GitHub API changed-file list is treated as untrusted data and must be complete.

Changes to the guard or policy itself are still reviewed using the **base's** guard, never the new PR code. However, after a governance PR merges, the new program becomes the future policy. Therefore these changes require manual review by the human owner and test evidence; CODEOWNERS does not provide a second GitHub identity or independently enforce that review. Administrators and Codex accounts using the same real GitHub identity share authority.

GitHub's public-repository event policy may block `pull_request_target` from 2026-11-02. **Do not activate a Ruleset requiring `scope-guard` until GitHub event policy explicitly permits the trusted workflow and a real PR confirms its check runs on the required SHA**. Do not treat a skipped, absent, neutral, or locally simulated check as proof of real enforcement.

## Checks and release constraints

The proposed required merge checks are `full-gate`, `mini-gate`, `workbench-gate` and `scope-guard`. Their existing upstream matrix jobs must complete successfully, not merely be skipped. The initial summary gate checks the upstream matrix-job aggregate, **not an independently frozen matrix-member count**. Matrix-definition changes are therefore classified as `repo` control-plane changes requiring separate review. `Workbench Measurement Evidence` is initially advisory for ordinary PRs (a specific acceptance condition for the measurement-focused PR #9), not part of the always-required `workbench-gate`; see [RULESETS.md](RULESETS.md). Rulesets are an **additional** platform setting: source files alone do not enforce PR-only merging or protected tags.

- Do not touch the frozen Full 4.x public API, LS1 and LKS4SNP1 formats as part of governance.
- Do not touch Mini 1.x's 18 API functions, source implementation or released contract.
- Do not rewrite historic tags, releases, benchmark measurements, issue histories or asset checksums.
- Workbench PR #9 must remain independent, and any new CI requirements should be enabled only after verified transition.
- `repo` is a small path class, **not** a license to modify Full, Mini, or Workbench production code.

CODEOWNERS maps paths to the *real* account `@RXY712200`; the Full and Mini ChatGPT conversations are independent architecture advisors, not two separate GitHub reviewers.
