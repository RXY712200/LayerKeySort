# V4 development / RC

Preview development is complete. Current stage: Stage 5 — Release Candidate 1,
no Preview.6. See [RC record](V4_RC1.md). [Freeze](V4_FREEZE.md) defines permitted changes.

Run tools/verify_v4_api.py, strict C17 build/CTest, all seven examples,
tools/validate_distribution.py and tools/validate_release_assets.py. Legacy V3
regressions use private declarations, not primary public examples.
RC may fix bugs, sanitizers/portability, docs/tests/package and performance while
preserving frozen API/semantics/wire. Incompatible corrections revoke freeze.
No new feature or architecture under a bug-fix label. Preserve historical refs;
Stable/main integration requires separate authorization. Preview.4 full evidence
remains primary, no new policy search during freeze.
