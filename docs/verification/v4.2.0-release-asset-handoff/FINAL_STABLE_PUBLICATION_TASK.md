# LayerKeySort v4.2.0 Stable Publication — Authorized Execution Task

## Authority and decision

The repository owner explicitly authorized the following conditional action in the current ChatGPT conversation on 2026-11-27:

> Start the final verification. If verification has no issue, do not ask for another authorization; directly complete all remaining work.

The condition has now passed independent ChatGPT verification of the actual GitHub-hosted release assets. This is an execution authorization for the exact Stable publication described below. **Do not ask the owner for another confirmation.**

Follow the current Development-Playbook in full, especially WORKFLOW.md and RELEASE_POLICY.md, plus all applicable LayerKeySort project instructions. Record the exact policy revision and files actually read.

## Fixed source and independently verified evidence

Repository: `RXY712200/LayerKeySort`

Exact integrated `main` commit:
`f751e1ee2afd682874281d6dc495969f9eca49eb`

Exact tree:
`18261736d37cba995a20d9a3132313d954ff3ef1`

Evidence branch:
`audit/v4.2.0-release-asset-evidence-20261127`

Evidence commit:
`333a111e1e7dd329baf475b906c2393635510be0`

Evidence commit parent is exactly the fixed main commit. The five handoff files are under:
`docs/verification/v4.2.0-release-asset-handoff/`

ChatGPT independently retrieved all five files from GitHub and recomputed their SHA-256 values. All matched the handoff records. It independently decompressed the actual ZIP, checked the five-member central-directory inventory, member sizes and CRC-32 values, and independently calculated the archive and member SHA-256 values.

Verified assets:

- `LayerKeySort-4.2.0-amalgamation.zip`: 35,696 bytes; SHA-256 `1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb`.
- `LayerKeySort-4.2.0-SHA256SUMS.txt`: 102 bytes; SHA-256 `36e06a53ad41f0a01ee0435e5c81ec81b8611b61df2351f5cf322ea1acd6f68e`.
- `PUBLICATION.json`: 2,762 bytes; SHA-256 `c3fd0015eb2e1ee31a15f028e1a3489fcf1dbb8388bc4b170a30e51431a74b33`.

The ZIP contains exactly, in this order:

1. `LayerKeySort-4.2.0-amalgamation/example.c` — 806 bytes; SHA-256 `305f5e981fd079bf09d6530b0d65b0d9e1d700377b460365d10bcebbc9e556e7`; CRC-32 `a38ec1a0`.
2. `LayerKeySort-4.2.0-amalgamation/layerkeysort.c` — 127,600 bytes; SHA-256 `ea40c37b222577784a7974471cce25a11d6bcb68395c1c5c46fae5aa0778cc10`; CRC-32 `cf537472`.
3. `LayerKeySort-4.2.0-amalgamation/layerkeysort.h` — 16,784 bytes; SHA-256 `9c755fa05ea1ed0e9d0a00860afb5b2854315c3dd10c5ac5d53b6a9e87a91c39`; CRC-32 `2f675a9c`.
4. `LayerKeySort-4.2.0-amalgamation/LICENSE` — 1,066 bytes; SHA-256 `114b7a8bc6e367d27ccad97352e71f998e8b8f933db28f0f769ca3cac38a3df5`; CRC-32 `dfa13558`.
5. `LayerKeySort-4.2.0-amalgamation/README.txt` — 1,081 bytes; SHA-256 `b64dbf9edcfaaf8572be58bc7a508d963e893efc56a2f16783628fed27818cca`; CRC-32 `223c07b5`.

The four non-generated members were byte-identical to the corresponding final-main sources or the exact README literal in `tools/amalgamate.py`. The generated `layerkeysort.c` was independently compared section-by-section against all 8 private headers and 16 production modules fetched from the fixed main SHA. All 24 sections matched exactly and were contiguous: the exact 98-byte preamble ended at offset 98; private headers occupied offsets 98–24,211; production modules occupied offsets 24,211–127,600; no unverified trailing bytes remained.

The checksum file names the exact ZIP and its expected SHA-256. `PUBLICATION.json` pins the exact main commit/tree and the two asset hashes, and describes an unexecuted annotated tag/Stable Release plan.

At the time of independent preflight, GitHub `main` was still the fixed SHA, `v4.2.0` tag and Release did not exist, and Latest was `v4.1.0`. Recheck all of these immediately before any write.

## Objective

Complete the exact LayerKeySort Full `v4.2.0` Stable publication, and nothing else:

- Create and push an **annotated** Git tag `v4.2.0` targeting exactly `f751e1ee2afd682874281d6dc495969f9eca49eb`.
- Create a public, published, non-draft, non-prerelease GitHub Release titled `LayerKeySort v4.2.0`, tagged `v4.2.0`, and set it as Latest.
- Attach exactly the two original, already-verified assets listed above.
- Independently verify the published tag, Release state, and downloaded asset bytes/digests.
- Produce a complete publication receipt.

## Preflight — mandatory immediately before publication

1. Read current project instructions and the full current Developer-Playbook `WORKFLOW.md` and `RELEASE_POLICY.md`. Do not rely on summaries. Record the exact policy commit/revision and full files read.
2. Verify the local repository is the correct LayerKeySort checkout. Verify local `main` and `origin/main` both equal the fixed full SHA above. Verify the working tree is clean and there are no unexpected staged, unstaged, or untracked changes. Do not reset, clean, stash, or overwrite anything.
3. Verify the pinned commit's tree equals `18261736d37cba995a20d9a3132313d954ff3ef1`.
4. Verify the local original assets and `PUBLICATION.json` in the already-existing publication workspace, expected under:
   `D:\\LayerKeySort\\.local\\full-v4.2.0-final-f751e1e\\build\\v4.2.0-publication\\`
   Preserve the existing bytes. Recompute the two asset hashes locally and compare to the values above. Do not regenerate, rebuild, repackage, normalize, or edit either asset.
5. Read the actual `PUBLICATION.json` and `commands.json` from that prepared publication directory. Confirm their target SHA/tree, tag annotation, Release flags, exact asset list and release-note file names agree with this task.
6. Check local and remote tag `v4.2.0`, GitHub Release by tag, and GitHub Latest. The tag and Release must still be absent; Latest must still be `v4.1.0`; `main` must still equal the fixed SHA. If any of these changed unexpectedly, stop without overwriting or deleting anything.
7. Inspect the prepared `RELEASE_NOTES.en.md` and `RELEASE_NOTES.zh-CN.md` files named by `PUBLICATION.json`. Read both completely. Confirm they are final user-facing release notes, contain no placeholders, no false claim that publication has not yet happened, and no unsupported new claims. Use the exact prepared release-note content. If the files are missing or unsuitable, inspect the corresponding final-main `docs/V4_2_0_RELEASE_NOTES.md` and `docs/V4_2_0_RELEASE_NOTES.zh-CN.md`; create only a temporary combined Release body outside tracked worktrees, removing only pre-publication status boilerplate that would be false after publication. Preserve all substantive claims and limitations. If a truthful final body cannot be produced from existing evidence, stop and report the precise blocker rather than inventing release text.
8. Check installed `git` and `gh` versions and the exact `gh release create --help` options. Use the authenticated existing GitHub CLI connection; never display or export credentials.

## Publication actions — explicitly authorized if preflight passes

1. Create the annotated tag locally with exact target and annotation:
   `git tag -a v4.2.0 f751e1ee2afd682874281d6dc495969f9eca49eb -m "LayerKeySort v4.2.0"`
2. Verify the local tag object and peeled target before pushing. Push only:
   `git push origin refs/tags/v4.2.0`
   Do not push branches or any other tags.
3. Verify the remote tag object and peeled target. It must point to the fixed main commit exactly.
4. Create the GitHub Release using the prepared final notes and exactly these two original asset paths:
   - `LayerKeySort-4.2.0-amalgamation.zip`
   - `LayerKeySort-4.2.0-SHA256SUMS.txt`

   Required Release metadata:
   - title: `LayerKeySort v4.2.0`
   - tag: `v4.2.0`
   - draft: false
   - prerelease: false
   - Latest: true

   Use `gh release create` with the already-pushed tag and the verified local assets. Use `--verify-tag` and `--latest` if supported by the installed CLI. If the CLI lacks a supported Latest option, use its authenticated GitHub API capability to set `make_latest=true` after creation. Do not allow the CLI to silently create a lightweight tag; the annotated tag must already exist.
5. Do not upload the evidence files, `PUBLICATION.json`, `verification.json`, or handoff report as Release assets. The Release must contain exactly the ZIP and checksum file.

## Post-publication independent verification

1. Fetch the remote annotated tag and verify its tag object SHA, annotation, tag name, and peeled commit SHA.
2. Fetch the GitHub Release by tag and verify exact title, tag, public/published state, `draft=false`, `prerelease=false`, and Latest state.
3. Verify the Release has exactly two assets with exact names and expected byte sizes. Download both actual published assets into a fresh temporary directory and independently compute SHA-256. Both must match the values at the top of this task. Also compare the GitHub API asset digest if available.
4. Verify `GET /releases/latest` identifies `v4.2.0`. Confirm the historical `v4.1.0` tag and Release remain intact; do not edit them.
5. Confirm `main` remains exactly the fixed SHA and no branch, PR, source file, docs, tests, workflow, dependency, benchmark archive, governance rule, Mini or Workbench content was changed. Do not merge the evidence branch and do not create a PR.
6. Write a detailed English publication receipt under the existing local `.local/tasks/` area, recording actual UTC timestamps, tag object SHA and peeled target, Release URL/ID/flags, asset IDs/sizes/digests and downloaded-byte verification, final Latest result, final main SHA, policy revision, exact commands/actions and any limitations. Preserve all prior handoff and evidence files.
7. For durable GitHub history, add the publication receipt as a new file on this task branch only:
   `docs/verification/v4.2.0-release-asset-handoff/PUBLICATION_RECEIPT.md`
   Commit and push that receipt on `ops/v4.2.0-stable-publication-20261127`. Do not merge the branch, open a PR, or modify `main`. If this branch no longer has the expected task commit as its head, inspect and preserve its state; never force-push.

## Failure and partial-completion rules

- If any preflight condition fails, stop before creating the tag and report exact evidence.
- If the tag is successfully pushed but Release creation fails, do not delete or move the tag. Inspect the actual remote state and report precisely what succeeded and what remains. Continue only with a safe missing step still covered by the owner's authorization.
- If Release creation partially succeeds or asset upload fails, inspect the Release and its actual assets before retrying. Do not create a duplicate Release or overwrite/delete an existing asset blindly.
- Never claim full publication until the remote tag, Release, Latest state, and both downloaded asset digests are independently verified.
- Do not force-push, reset, clean, rewrite history, change existing tags/releases, edit main, create a PR, merge, or perform unrelated work.

## Completion report

Return a concise but substantive English completion report with:

- final main SHA and confirmation it did not change;
- annotated tag object SHA and peeled commit;
- Release URL, ID, title, draft/prerelease/Latest state;
- exact two asset names, sizes, SHA-256 values and post-upload download verification;
- final `/releases/latest` result;
- publication receipt local path and GitHub evidence-branch URL/commit;
- exact Git working-tree state;
- **Technical Debt Impact** (expected: No change.);
- any partial failure or limitation, without claiming success beyond evidence.

The owner has already authorized this conditional publication. Do not ask for another confirmation if the independent verification and preflight pass.
