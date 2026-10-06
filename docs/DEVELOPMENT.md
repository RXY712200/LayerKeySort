# V4 Stable maintenance

V4.0.0 completes Stage 6 of the fixed V4 development cycle after independent
RC.1 review. [Stable record](V4_STABLE.md), [RC record](V4_RC1.md) and the historical
[freeze](V4_FREEZE.md) preserve the release evidence and compatibility decisions.

Run tools/verify_v4_api.py, strict C17 build/CTest, all seven examples,
tools/validate_distribution.py and tools/validate_release_assets.py. Legacy V3
regressions use private declarations, not primary public examples.

4.x maintenance must preserve frozen source/layout, status and wire contracts.
Incompatible changes require a separately justified major-version decision.
Preserve historical refs. Preview.4 remains the primary performance evidence;
Stable promotion changes only release metadata and presentation, not production
algorithms, tests or policy. Deferred work is classified, not scheduled, in the
Stable record. Do not treat publication as authorization for a new research cycle.
