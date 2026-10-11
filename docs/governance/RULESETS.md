# Governance rollout: GitHub admin settings and rollback

**This file is a checklist, not proof that GitHub Rulesets are active.** Do not use it to bypass PR testing or to edit an existing Stable Tag/Release.

## Verified prerequisites for activation

- [ ] A restorable, independently saved **off-GitHub** mirror/bundle and the existing Full/Mini Stable Release assets have been retained and checksummed. PR #10's Actions artifact alone is temporary and stays on GitHub.
- [ ] The product-neutral README/Full entry PR #10 has been reviewed (including Mini descriptions and Pages links) and merged without production changes.
- [ ] Mini Workbench Draft PR #9 is either normally merged and retested or a separate migration/exception has been explicitly approved. **PR #9 changes consumer code and Workbench workflow files; these are intentionally different protected owner classes.**
- [ ] Scope Guard and all three fixed matrix gates are installed on protected-base candidate `main`.
- [ ] Real test PRs, against this base, demonstrate `scope-guard` starts and reports the right source, SHA, and pass/fail result. A PR cannot test the initial `pull_request_target` workflow addition until it is installed on its base.
- [ ] `pull_request_target` is explicitly allowed by the repository's applicable GitHub Actions event policy. GitHub's default public-repository block starts enforcing on 2026-11-02 for affected repositories.
- [ ] The exact check names `scope-guard`, `full-gate`, `mini-gate`, `workbench-gate` appear as distinct, completed checks on the same up-to-date PR. Do not substitute the informational `scope-guard-selftest` for `scope-guard`.
- [ ] Classic Branch Protection is checked directly in the GitHub admin UI (GitHub App API returned 403, **not** an authoritative no-protection result).
- [ ] The human owner confirms this configuration cannot deadlock normal solo-maintainer merge and Release creation.

## Branch Ruleset: main

GitHub repository → Settings → Rules → Rulesets → New branch ruleset; target **main only**.

1. Require a Pull Request before merging.
2. Require status checks before merging: exactly the four fixed check contexts listed above. Check source must be the intended GitHub Actions app; verify with a real test PR. Do not set a check before it has genuinely run.
3. Block force pushes and deletions; require conversation resolution where supported.
4. Keep the repository's existing **Merge Commit** workflow. Do not enable **Require linear history**.
5. As a solo maintainer, do not require an unavailable second approver or an unfulfillable CODEOWNER approval.
6. Ordinary Codex tasks must not have an automatic bypass. A real owner emergency recovery plan, including before/after settings snapshot and reason, is mandatory.
7. Required workflows are not path-filtered; their final jobs use `always()` and fail unless all listed upstream job IDs really report `success`.
8. If "Require branches to be up-to-date" is enabled, verify normal merging and CI reruns with PR #9 transition before activation.

**Pause before enforcement** if the platform cannot run the trusted `pull_request_target` event or cannot attribute the resulting check correctly. Do not silently select a weaker/self-declared guard as a substitute.

## Tag Rulesets

GitHub repository → Settings → Rules → Rulesets → New tag ruleset. Protect both historical Full `v*` and Mini `mini-v*` names; avoid accidental exclusion of either product.

- Restrict **updates and deletions** of published matching tags.
- Do **not** apply a blanket create restriction without an authorized, tested, non-bypass normal release path. Tag-creation and tag-update rules are separate.
- Test restrictions on a disposable throwaway tag in a **separate test repository or isolated controlled namespace**, never by rewriting existing Full/Mini release tags.
- Record resolved annotated-tag object and peeled commit SHA; do not confuse the tag-object SHA with the underlying source commit.

## GitHub Release integrity

Existing Full 4.0.0 and Mini 1.0.0 Releases reported `immutable=false` at capture. New Release Immutability is a **future-release setting** and is not retroactive. Preserve the four baseline published SHA-256 hashes in [BASELINE.md](BASELINE.md) when that file is merged; do not edit legacy Release assets as a test.

When the platform supports and the team approves it, create a Draft Release, upload and verify exact-SHA assets and manifests, then publish with immutability. No product release is authorized by this governance document.

## Recovery / negative tests

Before enforcing the Ruleset, demonstrate in a test repository or test PRs:

- A Mini-only PR is accepted; Mini+Full production edits are rejected.
- A Full-only PR and a Workbench consumer-only PR are accepted.
- A Repo-only PR cannot authorize product production modifications by adding a PR label.
- A PR that weakens Scope Guard cannot supply its own replacement validator to pass; the trusted base supplies the active checker.
- A missing, cancelled, failed, skipped or neutral **upstream** requirement is rejected by its corresponding fixed CI gate.
- Workflow changes in the same PR as product source are cross-owner changes and need a separately approved exact-file exception.
- Correct Full/Mini releases can create **new** tags, while previously published tags cannot be deleted/repointed through the ordinary path.

If enforcement creates a maintenance deadlock, the human repository owner should document the condition and exact check statuses, temporarily change the blocking Ruleset using the admin UI, restore a known-good governance workflow by PR, re-enable the rule and record the recovery. Do not force-push `main`, alter released history or disable the rules permanently to hide a failure.
