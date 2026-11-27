# v4.2.0 post-publication documentation transition

[简体中文](REVIEW.zh-CN.md)

Scope: living publication status and navigation only. Released source remains `f751e1ee2afd682874281d6dc495969f9eca49eb`; annotated tag object `a7f249c54afd3ae131bb0cbe32e8d5eb45a1610b` and Release 460067556 are unchanged.

## Findings and dispositions

- Correct stale 4.1 published selectors, planned downloads and obsolete integration handoff in the eight Repo entrypoints. Living Full documentation links now select main; example source selects the released commit. Historical figures and source references remain immutable.
- Correct Integration, cumulative changelog, 4.2 edition record, release notes and current Benchmark Stable statement in both languages. Publication date is 2026-11-27.
- Add exact released-source reproduction pointers in Compatibility, Validation, Development and all four detailed-guide pairs. Preserve RC regression pins and measured operational-source pins as historical baselines.
- Consumer handoff now leads with the published release; earlier entries retain their own checkpoint status. OrdWeave dependency statements remain unchanged.
- Inspect remaining current-facing inventory matches in API, Architecture, Usage, migrations, examples and benchmark entrypoints. Preparation inside operation descriptions means prepare/commit, not release status. 4.1 comparisons and older Preview/RC/changelog records are historical and retained. No Mini/Workbench edit is indicated.

## Verification boundary

Required checks run on each actual PR head and again on final integrated main. Ownership separates Full and Repo changes. Link and counterpart checks cover modified living documents; release and asset URLs resolve to the existing official release. Source, header/version metadata, tests, frozen fixtures, build files, dependencies and archived measurements must remain byte-identical to released source. No benchmark campaign or asset regeneration is part of this update. Final run IDs and merge commits are supplied in the completion report, avoiding a self-referential commit pin.

Technical Debt Impact: No change to FULL-RI-001–007.
