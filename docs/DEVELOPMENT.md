# V4 development / RC

Final Preview.5 completes Preview development on PASS. Next Stage 5 — Release
Candidate, no Preview.6. [Freeze](V4_FREEZE.md) defines permitted changes.

Run tools/verify_v4_api.py, strict C17 build/CTest, all seven examples,
tools/validate_distribution.py and tools/validate_release_assets.py. Legacy V3
regressions use private declarations, not primary public examples.
RC may fix bugs, sanitizers/portability, docs/tests/package and performance while
preserving frozen API/semantics/wire. Incompatible corrections revoke freeze.
No new feature or architecture under a bug-fix label. Preserve historical refs;
Stable/main integration requires separate authorization. Preview.4 full evidence
remains primary, no new policy search during freeze.
