# Complete v4.2.0 post-publication documentation transition

The owner has authorized direct correction of outdated public documentation and navigation after the successful Stable publication, without a version bump. Complete this work through integration; do not ask for another routine approval or introduce additional development phases. Follow the current Development-Playbook and repository ownership/Scope Guard rules.

## Established facts

- Full v4.2.0 is published, non-draft, non-prerelease and repository Latest: https://github.com/RXY712200/LayerKeySort/releases/tag/v4.2.0
- Published at 2026-11-27T01:39:14Z; release ID 460067556.
- Annotated tag object a7f249c54afd3ae131bb0cbe32e8d5eb45a1610b peels to f751e1ee2afd682874281d6dc495969f9eca49eb (the integrated main at review time).
- Mini remains independently released at mini-v1.0.0; Workbench remains a Mini consumer. Do not alter either product or OrdWeave's dependency pin.
- Official assets: https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-amalgamation.zip (SHA-256 1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb) and https://github.com/RXY712200/LayerKeySort/releases/download/v4.2.0/LayerKeySort-4.2.0-SHA256SUMS.txt (SHA-256 36e06a53ad41f0a01ee0435e5c81ec81b8611b61df2351f5cf322ea1acd6f68e).
- Publication receipt: https://github.com/RXY712200/LayerKeySort/blob/9f6a2ca9b7d7a7f26bd443dd712b30f66c171772/docs/verification/v4.2.0-release-asset-handoff/PUBLICATION_COMPLETED.md
- The previous Stable preparation record explicitly anticipated this post-publication transition. It was missed.

## Fix confirmed issues

Repo-owned current-facing files (keep separate from Full-owned changes):
- README.md and README.zh-CN.md: download/source status and Full/Mini comparison table's Full Published release row are stale (both still link to v4.1.0). Present v4.2.0 as published Stable/Latest, link the actual release and assets, retain old 4.1 links only for historical access.
- docs/FULL.md and docs/FULL.zh-CN.md: same issues, plus remove/replace the obsolete integration-navigation paragraph at the bottom.
- docs/PRODUCT_LINES.md and docs/PRODUCT_LINES.zh-CN.md: current edition/status, tag/version table and Latest date must reflect v4.2.0.
- docs/index.html: both language publication status/download links are wrong.
- CONTRIBUTING.md: inspect for consistency; change only if needed.

Full-owned files:
- docs/INTEGRATION.md and docs/INTEGRATION.zh-CN.md: current version, recommended pin and download instructions must refer to v4.2.0; retain useful prior-version compatibility facts.
- CHANGELOG.md and docs/CHANGELOG.zh-CN.md: mark 4.2.0 released on 2026-11-27; correct current Stable statement and release entry, not historical entries.
- docs/V4_2_0.md and docs/V4_2_0.zh-CN.md: convert current edition/publication sections to actual published status; record the release date, exact source/tag, assets, final validation and receipt links. Preserve preparation history as past-tense evidence. Remove unfulfilled-looking instructions for the already completed transition.
- docs/V4_2_0_RELEASE_NOTES.md and docs/V4_2_0_RELEASE_NOTES.zh-CN.md: make the current main copies accurate final release notes, with actual release/source/assets links and date, not proposed notes. Preserve measured limitations and substantive release claims.
- docs/BENCHMARKS.md and docs/BENCHMARKS.zh-CN.md: explicitly incorrect current Stable v4.1.0 statements must be corrected; keep historical benchmark comparisons.
- Inspect both editions of COMPATIBILITY, VALIDATION, DEVELOPMENT, all four IMPLEMENTATION guides, and the three FULL_CONSUMER_HANDOFF editions. Fix misleading current preparation/publication status, stale current-source navigation, and final Stable reproduction pointers as applicable. Do not rewrite historical RC/Preview pins, old measurements, consumer dependency pins or old entry records. The four guides may retain RC test pins explicitly labeled historical; point readers seeking the released source to v4.2.0.

## Check the rest, including link destinations

Search tracked Markdown/HTML and other relevant public-facing navigation for v4.1.0, 4.1.0, not published, not yet tagged, awaiting publication, preparation, proposed release, current Stable, Latest, future tag, 尚未发布, 未发布, 拟发布, 准备, 当前 Stable, and the preparation commit c119c930b36becbc24392f6bb29bef761e72ea09. Review matches in context; do not blindly replace numbers or historical text. Check other current docs, examples, benchmarks, and the public Pages site entry/navigation. Leave genuine historical snapshots and evidence intact. If out-of-scope Mini/Workbench content is genuinely stale, report it separately without editing.

Important: current README/FULL/Pages documentation links are pinned to the old Full preparation commit. Merely fixing the visible README release link leaves readers landing on outdated copies. Use correct relative links or main links for living/current documentation (including changelog, integration and version record), and use v4.2.0 or exact release SHA for immutable released SOURCE. Preserve intentionally immutable historical benchmark evidence, figures and historical source links. Do not redirect current docs to the immutable pre-publication tag snapshot if that would reintroduce stale status. Verify bilingual switches, local paths/anchors, public release and asset links. Put a concise working release/download entry near the top where appropriate without redesigning the pages.

## Delivery and scope

Use two appropriately scoped documentation-only branches/PRs from current main (Repo-owned and Full-owned respectively), as ownership rules require. Complete the corrections in one work unit, with the normal required Scope Guard and CI checks; integrate both via normal authorized PR merges when the required checks pass. Do not bypass branch protection. Verify the final main diff consists only of relevant documentation/navigation; check the English and Chinese entry pages, current download links and Pages deployment if available. No additional performance campaign, RC, release preparation or repetitive asset audit.

Do not change production code, API, version metadata, CMake, tests, frozen fixtures, evidence measurements, governance, Mini, Workbench or dependencies. Do not create a new tag/release, move the v4.2.0 tag, replace assets, or edit historical release objects. Keep the existing official release unchanged. Current main docs may naturally be newer than the immutable release source snapshot. Report any true out-of-scope discrepancy; do not modify unrelated material.

Return a concise English completion report: findings fixed, PRs/commits and final main SHA, changed files by owner, actual required checks, working links and Pages status, unchanged release/tag/assets/version, any remaining limitation, and Technical Debt Impact: No change unless evidence indicates otherwise. Archive substantial new evidence if needed in the repository, not just a local path.
