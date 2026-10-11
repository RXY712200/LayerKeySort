# LayerKeySort repository governance — source baseline

This is an **observation record**, not a release, permission grant, or claim that an offline backup exists.

## Captured state (2026-10-11; GitHub API)

- Repository: `RXY712200/LayerKeySort`; default branch: `main`.
- `main` commit/tree: `7f17331297e88618a71257ebc65b04cb6b4b8c25`; full recursive tree returned without truncation (277 tracked blobs).
- Full Stable: tag `v4.0.0`, release target `80004ae732daf696f7e9192e417de0edb0b2aafa`.
- Mini Stable: annotated tag `mini-v1.0.0` peels to `3f52798b83b6580aa4b89846b6403b0bd7b15f0a`.
- The repository's Latest release was Full `v4.0.0`.
- Open PR: [Workbench Phase 2 #9](https://github.com/RXY712200/LayerKeySort/pull/9), **Draft**, head `6af36e0f60d93d7893f73223ce9f0845fe37a93e` at capture.
- Rulesets API returned an empty list. Classic branch protection was not independently readable through the connected integration (403); do not infer that all protection is absent.
- Successful main CI runs: [Full](https://github.com/RXY712200/LayerKeySort/actions/runs/38052087857), [Mini](https://github.com/RXY712200/LayerKeySort/actions/runs/38052087854), [Workbench](https://github.com/RXY712200/LayerKeySort/actions/runs/38052087860).

### Published Stable assets (GitHub API SHA-256 digests)

| Product | Asset | SHA-256 |
| --- | --- | --- |
| Full 4.0.0 | `LayerKeySort-4.0.0-amalgamation.zip` | `ed828c547cba05e39d05e28c9f6eaec7ffd325839d291713b7cfe0e47bef1a1d` |
| Full 4.0.0 | `LayerKeySort-4.0.0-SHA256SUMS.txt` | `0ad427855cde57af7c426dba6168830838b0e7932bfa77c8c5efce84cdbcfee2` |
| Mini 1.0.0 | `LayerKeySort-Mini-v1.0.0-source.zip` | `96b21a4d9177a98991e27712063dab7ce4d91a4adc0a76e420e8ab74263089c4` |
| Mini 1.0.0 | `LayerKeySort-Mini-v1.0.0-SHA256SUMS.txt` | `722c2d9aa5bace6ce7d15cdb5f876c270d9a0c3105b882f8e1d041db9743141a` |

All four Release assets reported `immutable=false` for their Releases at capture. Tag protections and future Release Immutability are distinct controls.

## Recovery and independent-backup requirement

An immutable commit SHA and checksum manifest provide a **recovery reference**, not an external backup. Before activating strict Rulesets or merging governance PRs, archive the repository and verify restore independently:

```sh
git clone --mirror https://github.com/RXY712200/LayerKeySort.git LayerKeySort.git
git -C LayerKeySort.git bundle create ../LayerKeySort-governance-baseline.bundle --all
git -C LayerKeySort.git bundle verify ../LayerKeySort-governance-baseline.bundle
```

Store the Git bundle **outside the GitHub repository**. Separately download the Full/Mini Stable Release attachments and verify their SHA-256 values above. A Git bundle does not include GitHub Issues, PR discussions, Actions settings, or Release assets; export those records separately if comprehensive disaster recovery is required. A GitHub Actions artifact alone should not be described as an offline backup.

**Status at initial capture:** baseline recorded; offline mirror, bundle creation, external storage and restoration verification remain **pending** in this environment. These pending items are not silently marked PASS.

Do not create a fake backup tag or rewrite existing releases. Confirm `main` and PR #9 again immediately before any production rule activation.
