# Full historical issue reconciliation

Evaluation date: 2026-10-20. Read-only source inventories: [Issue #3](https://github.com/RXY712200/LayerKeySort/issues/3) and [Issue #4](https://github.com/RXY712200/LayerKeySort/issues/4), including all comments. Issues #1/#2 were consulted for legacy lineage; #7 was not used as a Full roadmap. No issues were edited.

All 47 Issue #4 entries are dispositioned below. Historical severity remains the entry prefix. Current action priority is a separate assessment for this consumer, not an invented adoption/frequency score. No production correctness defect was confirmed. First version is not invented where the ledger does not identify it. Historical metadata is reproduced as metadata, not endorsed as current-state wording.

Evidence keys: **E1** editor C/Python oracles and domain workflow; **E2** contracts.c managed/Group/Batch/sort; **E3** contracts.c wire/LS1/history/callbacks plus editor save/restore; **E4** failures.c GNU external interception; **E5** PERFORMANCE.md and raw CSV; **E6** read-only production architecture inspection; **E7** EVIDENCE.md executed builds/tests/manifest/goldens; **E8** reviewed Full docs/header/issues; **E9** live GitHub metadata/baseline.json.

Issue #3 active families map to Highest-6 (user evidence), Highest-7 (cost model), High-1 (allocator), High-2/Medium-2 (export/history), Medium-1 (blocks), Medium-4 (portability), and the deferred/non-goal entries below. The direction remains frozen 4.x maintenance and evidence-led integration; no V5 is authorized.

## Highest-1 — Live order and persistent/exported coordinate were the same concept

- Historical metadata: **First clearly present:** V1
- Historical metadata: **Affected:** V1, V2, V3
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Final resolution:** **V4.0.0**
- Historical metadata: **Final change type:** **Architecture replacement**

- **Current action priority:** None.
- **Current status/category:** Resolved; no live/export coupling defect reproduced.
- **Root/impact:** Legacy coordinate model coupled mutation and persistence; V4 replaces it with handles plus owned historical bytes.
- **Evidence and reproducibility:** E1/E3/E6: live edits preserve history; no live Path export API.
- **Change/dependencies/disposition:** V4 architecture replacement; archive active legacy issue, preserve lineage; no new change.

## Highest-2 — Uncontrolled Path growth / workload-dependent live coordinate size

- Historical metadata: **First clearly identified:** V1
- Historical metadata: **Affected:** V1, V2, V3
- Historical metadata: **Current status:** **Resolved for the V4 live model**
- Historical metadata: **Final resolution:** **V4.0.0**
- Historical metadata: **Final change type:** **Architecture replacement**

- **Current action priority:** None.
- **Current status/category:** Resolved for live V4; old issue not applicable.
- **Root/impact:** V1-V3 resident coordinate depth caused workload-dependent storage. V4 residents have no live Path/LK1.
- **Evidence and reproducibility:** E5/E6: stable live request totals across churn, fixed record storage.
- **Change/dependencies/disposition:** V4 replacement; archive Path-growth debt; snapshot memory is separate High-2/Medium-2.

## Highest-3 — Large/full-range relabel and synchronous tail-latency cliffs

- Historical metadata: **Origin:** V2/V3 coordinate-maintenance model
- Historical metadata: **Most visible:** V3.1 synthetic workload campaign
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Final resolution:** **V4.0.0**
- Historical metadata: **Final change type:** **Architecture replacement**

- **Current action priority:** None.
- **Current status/category:** Resolved V3 coordinate rewrite mechanism.
- **Root/impact:** Legacy depth/acceptance/layout interactions caused global relabel. V4 has local block maintenance instead.
- **Evidence and reproducibility:** E1/E5/E6: move/churn cases; source has no live coordinate relabel. Finite maxima are not latency bounds.
- **Change/dependencies/disposition:** V4 replacement; do not reopen V3 tail research. Keep OS/allocator tails separate.

## Highest-4 — Path hierarchy coupled to physical Tree topology

- Historical metadata: **First clearly identified:** V1
- Historical metadata: **Affected:** V1
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Primary resolution:** **V2**
- Historical metadata: **Further superseded:** **V4**
- Historical metadata: **Change type:** **Large refactor**, later **architecture replacement**

- **Current action priority:** None.
- **Current status/category:** Resolved V2, superseded V4.
- **Root/impact:** Path prefixes formerly constrained physical topology and deep insertion.
- **Evidence and reproducibility:** E6/E8: current API has no physical navigation or live Path.
- **Change/dependencies/disposition:** Historical large refactor/replacement; archive; no current dependency.

## Highest-5 — Product identity risk: being treated as a conventional sorting library

- Historical metadata: **Present since:** V1
- Historical metadata: **Current status:** **Mostly resolved**
- Historical metadata: **Resolution versions:** gradual V1→V4, clarified after V4 Stable
- Historical metadata: **Change type:** **Feature reduction / product boundary / documentation**

- **Current action priority:** Low.
- **Current status/category:** Mostly resolved; continuing product discipline.
- **Root/impact:** Adjacent sorting/database use cases can distort scope. Current consumer fits long-lived order.
- **Evidence and reproducibility:** E1/E8/E9: dynamic editor works; some old topics remain. Not a correctness failure.
- **Change/dependencies/disposition:** Retain boundary, merge metadata symptoms with Low-4; documentation only if authorized.

## Highest-6 — Lack of real application / real-user workload evidence

- Historical metadata: **Present since:** V1
- Historical metadata: **Current status:** **Unresolved**
- Historical metadata: **Change type:** **Documentation / research + real integration**

- **Current action priority:** Medium.
- **Current status/category:** Partially reduced integration gap; real-user evidence still missing.
- **Root/impact:** Synthetic and Codex-authored workflows cannot establish external adoption or actual editor distributions.
- **Evidence and reproducibility:** E1/E5: functioning standalone consumer and generated traces, not human sessions.
- **Change/dependencies/disposition:** Update evidence inventory; retain real editor/user gap. No production refactor follows from absence.

## Highest-7 — Whole-operation complexity and cost model was difficult to explain

- Historical metadata: **First clearly identified:** V1
- Historical metadata: **Affected:** V1–V4
- Historical metadata: **Current status:** **Partially resolved**
- Historical metadata: **Change type:** **Documentation / research**, with historical refactors

- **Current action priority:** Low.
- **Current status/category:** Partially resolved; no universal operation theorem.
- **Root/impact:** Structure, allocation, comparators, callbacks, copying and OS costs differ.
- **Evidence and reproducibility:** E5/E6/E8: separated measurements and current structural model; not a proof.
- **Change/dependencies/disposition:** Retain research only; optional documentation/formal analysis, not a latency contract change.

## Highest-8 — Core feature-boundary expansion into database / CRDT / persistent identity

- Historical metadata: **Nature:** long-term product risk
- Historical metadata: **Current status:** **Intentionally not solved**
- Historical metadata: **Change type:** **Feature reduction / non-goal**

- **Current action priority:** None.
- **Current status/category:** Deliberate non-goal.
- **Root/impact:** Database/CRDT/permanent identity semantics are not local order ownership.
- **Evidence and reproducibility:** E1/E3/E8: app owns IDs, files, resolver; no unsupported identity guarantee invoked.
- **Change/dependencies/disposition:** Retain boundary; do not implement adjacent systems.

## High-1 — Per-resident allocation / allocator and RSS overhead

- Historical metadata: **Introduced as a clear V4 tradeoff:** V4 Preview.1
- Historical metadata: **Current status:** **Unresolved**
- Historical metadata: **Change type if addressed:** **Medium-to-large refactor**

- **Current action priority:** Low.
- **Current status/category:** Confirmed accepted allocation traffic; RSS cause not fully attributed.
- **Root/impact:** Separately allocated residents preserve addresses and atomic preparation; deployment allocator costs vary.
- **Evidence and reproducibility:** E5/E6: 103124 grow allocations, 500k matched churn pairs at 100k; requested cleanup zero.
- **Change/dependencies/disposition:** Retain/update measured cost; merge traffic/locality hypotheses under resident storage, not snapshot history. Conditional medium/large private refactor only.

## High-2 — Full snapshot/export cost is O(N+A)

- Historical metadata: **Introduced intentionally:** V4 Preview.3
- Historical metadata: **Current status:** **Unresolved, accepted architectural cost**
- Historical metadata: **Change type if addressed:** **New feature + potentially large refactor**

- **Current action priority:** Medium.
- **Current status/category:** Confirmed accepted full export cost.
- **Root/impact:** Each capture copies rows and associations; frequent capture can dominate edits.
- **Evidence and reproducibility:** E3/E5/E6: 16 retained captures and 8 restores, full serialized sizes.
- **Change/dependencies/disposition:** Retain/update; evaluate app cadence before feature addition/large snapshot refactor. Related Medium-2, independent of High-1.

## High-3 — V1 front/reverse insertion caused child-array shifting/sibling growth

- Historical metadata: **Affected:** V1
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V2
- Historical metadata: **Change type:** **Large refactor**

- **Current action priority:** None.
- **Current status/category:** Resolved V2, superseded V4.
- **Root/impact:** V1 unbounded sibling arrays caused reverse/front movement. Current blocks are bounded.
- **Evidence and reproducibility:** E1/E5/E6: front/relative edits; bounded blocks inspected.
- **Change/dependencies/disposition:** Archive original issue; bounded local shifts remain Medium-1, not recurrence.

## High-4 — Linear equal-run successor scanning

- Historical metadata: **Affected:** V1/V2-era managed ordering paths
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Final resolution:** V4 Preview.2
- Historical metadata: **Change type:** **Medium refactor**

- **Current action priority:** None.
- **Current status/category:** Resolved in final V4 managed implementation.
- **Root/impact:** Walking equal successors was replaced by block-max/local binary bounds.
- **Evidence and reproducibility:** E2/E5/E6: stable equality, duplicates and locate pass; bound code inspected.
- **Change/dependencies/disposition:** Archive scan debt; comparator cost remains accepted; no retuning.

## High-5 — Comparator invariant/API footgun

- Historical metadata: **Most visible:** V2
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution path:** V3 then V4
- Historical metadata: **Change type:** **API refactor + medium refactor**

- **Current action priority:** Low.
- **Current status/category:** Resolved facade binding; caller data discipline remains.
- **Root/impact:** A retained comparator cannot remain valid if its comparison fields/context mutate incompatibly.
- **Evidence and reproducibility:** E2/E4: bound context/reentry and remove-edit-reinsert including failed reinsertion/retry.
- **Change/dependencies/disposition:** Do not reopen old binding ambiguity; supplement app recovery guidance, related Medium-6, not new core defect.

## High-6 — Missing first-class move semantics

- Historical metadata: **Affected:** V1/V2/V3
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V4 Preview.2
- Historical metadata: **Change type:** **New feature + medium refactor**

- **Current action priority:** None.
- **Current status/category:** Resolved V4 Preview.2.
- **Root/impact:** First-class relative movement was absent in earlier coordinate API.
- **Evidence and reproducibility:** E1/E5: all four moves/no-ops, stable handles, distant/local drags.
- **Change/dependencies/disposition:** Archive missing-move debt; no new helper required by consumer.

## High-7 — Failure atomicity under allocation failure

- Historical metadata: **Present since:** V1
- Historical metadata: **Current status:** **Resolved as a core contract**
- Historical metadata: **Change type:** **Medium/large refactor + validation infrastructure**

- **Current action priority:** Low.
- **Current status/category:** Core invariant supported by fresh finite tests.
- **Root/impact:** Fallible preparation must not expose partial structure. Multi-call application edits are separate.
- **Evidence and reproducibility:** E4/E7: 279 OOM sweep points, callback failure and rollback; no leak observed in scoped requests.
- **Change/dependencies/disposition:** Retain invariant/gates, not an open reproduced defect; all future storage changes depend on preserving it.

## High-8 — Path coordinate vs persistent identity semantics were unclear

- Historical metadata: **First explicit concern:** V1
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V2 clarified; V4 structurally separated
- Historical metadata: **Change type:** **API/architecture refactor**

- **Current action priority:** None.
- **Current status/category:** Resolved conceptual boundary.
- **Root/impact:** Coordinate/handle mistaken for business identity creates lifetime and persistence errors.
- **Evidence and reproducibility:** E1/E3: app occurrence IDs and fresh restore handles; no numerical handle ordering.
- **Change/dependencies/disposition:** Archive ambiguity; retain app identity discipline, no persistent-handle feature.

## High-9 — No independent historical snapshot model

- Historical metadata: **Affected:** V1–V3
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V4 Preview.3
- Historical metadata: **Change type:** **New feature + major architectural extension**

- **Current action priority:** None.
- **Current status/category:** Resolved V4 Preview.3.
- **Root/impact:** Historical ownership formerly depended on live coordinate model.
- **Evidence and reproducibility:** E3: copied associations, source destruction, canonical load/restore and historical rows pass.
- **Change/dependencies/disposition:** Archive missing capability; costs tracked separately in High-2/Medium-2.

## High-10 — Immutable Group carried unnecessary Path/Tree machinery

- Historical metadata: **Affected:** V1–V3
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V4 Preview.4/5
- Historical metadata: **Change type:** **Large refactor + feature reduction**

- **Current action priority:** None.
- **Current status/category:** Resolved V4 Preview.4/5.
- **Root/impact:** Immutable Groups previously retained mutable Path/Tree machinery.
- **Evidence and reproducibility:** E2/E6: flat stable Group/Batch, independent merge result survives Batch destruction.
- **Change/dependencies/disposition:** Archive historical representation cost; ordinary sort/merge scratch remains accepted.

## High-11 — Public API contained overlapping generations and excessive conceptual surface

- Historical metadata: **Most visible:** V4 Preview.1–4
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V4 Preview.5
- Historical metadata: **Change type:** **Feature reduction + API refactor**

- **Current action priority:** None.
- **Current status/category:** Resolved V4 Preview.5 freeze.
- **Root/impact:** Overlapping public generations created conceptual ambiguity.
- **Evidence and reproducibility:** E7/E8: 64-function/14-type manifest unchanged, public-only consumer builds.
- **Change/dependencies/disposition:** Archive transition problem; Full complexity is a suitability choice, not license to break 4.x.

## High-12 — No clearly frozen API/wire compatibility contract

- Historical metadata: **Affected:** early versions and V4 Preview phase
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V2 for LK1; V4 Preview.5 for V4 contracts
- Historical metadata: **Change type:** **Documentation / tests / compatibility contract**

- **Current action priority:** None.
- **Current status/category:** Resolved freeze; ongoing compatibility requirement.
- **Root/impact:** Persistent formats and declarations need exact stable meaning.
- **Evidence and reproducibility:** E3/E7: golden/manifest gates pass, strict canonical wire round trips.
- **Change/dependencies/disposition:** Retain gates; archive missing-contract debt; every future change constrained by freeze.

## High-13 — Full API may be too complex for simpler users

- Historical metadata: **Current status:** **Future candidate**
- Historical metadata: **Change type if pursued:** **Feature reduction + medium/large product design**

- **Current action priority:** Low.
- **Current status/category:** Accepted Full scope/suitability friction.
- **Root/impact:** Snapshots, Groups and managed order require concepts simple lists do not.
- **Evidence and reproducibility:** E1/E2/E8: actual consumer mappings/callbacks documented; no demonstrated harmful redundancy.
- **Change/dependencies/disposition:** Reclassify as suitability guidance, not Full defect. Do not evaluate another product or shrink frozen API.

## Medium-1 — B64 block slack and local pointer/location-update work

- Historical metadata: **Current status:** **Unresolved, accepted cost**
- Historical metadata: **Introduced:** V4 block architecture
- Historical metadata: **Change type if addressed:** **Small/medium refactor or tuning**

- **Current action priority:** Low.
- **Current status/category:** Confirmed accepted block policy cost.
- **Root/impact:** Slack, shifts and location work arise from stable bounded blocks.
- **Evidence and reproducibility:** E5/E6: append request decomposition and public timings; no private structural counters consumed.
- **Change/dependencies/disposition:** Retain cost; defer tuning absent a measured budget failure. Separate from record allocation.

## Medium-2 — Retained snapshot historical memory

- Historical metadata: **Current status:** **Unresolved, expected cost**
- Historical metadata: **Change type if addressed:** **None initially; possible small/medium optimization**

- **Current action priority:** Medium.
- **Current status/category:** Confirmed expected historical memory, not leak.
- **Root/impact:** Owning independent row/association copies scales with retained history.
- **Evidence and reproducibility:** E3/E5: 42378632 added requested bytes for 16 histories including marker, then released.
- **Change/dependencies/disposition:** Merge planning with High-2 snapshot parent; first apply application retention/cadence, not deduplication.

## Medium-3 — Defensive stale-handle detection

- Historical metadata: **Current status:** **Unresolved / intentionally deferred**
- Historical metadata: **Change type if addressed:** **New feature + medium/large refactor**

- **Current action priority:** Low.
- **Current status/category:** Deferred optional defense; not a tested defect.
- **Root/impact:** Raw opaque pointer copies expire on removal/destruction. Detection needs extra registry/lifetime state.
- **Evidence and reproducibility:** E1/E2: retired application IDs rejected, live foreign handles checked; no dangling inputs passed.
- **Change/dependencies/disposition:** Retain explicit contract; optional new feature only after real misuse evidence. Depends on resident lifetime design.

## Medium-4 — Portability breadth

- Historical metadata: **V1 status:** primarily MSVC/Windows-focused
- Historical metadata: **Current status:** **Partially resolved**
- Historical metadata: **Change type:** **New platform support**

- **Current action priority:** Low.
- **Current status/category:** Partial portability coverage.
- **Root/impact:** Compiler/CRT/architecture support cannot be inferred from C17 alone.
- **Evidence and reproducibility:** E7: fresh Windows GCC/MSVC/ASan; no fresh Linux/macOS/Win32/UBSan evaluation.
- **Change/dependencies/disposition:** Retain evidence gap; no port failure inferred from unavailable local tooling.

## Medium-5 — Build/integration maturity

- Historical metadata: **V1 status:** limited reusable build integration
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** V2/V3/V4 incremental integration work
- Historical metadata: **Change type:** **New infrastructure**

- **Current action priority:** None.
- **Current status/category:** Supported integrations reproduced successfully.
- **Root/impact:** Third-party projects need public target/header/distribution, not internal includes.
- **Evidence and reproducibility:** E7: add_subdirectory, installed package, official ZIP consumer; baseline consumer tests.
- **Change/dependencies/disposition:** Retain successful integration evidence; recipes separate Low-10 optional work.

## Medium-6 — Ownership/lifetime/callback contracts were historically under-specified

- Historical metadata: **Affected:** V1–V3, then V4 callback expansion
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Final freeze:** V4 Preview.5
- Historical metadata: **Change type:** **Documentation + API contract**

- **Current action priority:** Low.
- **Current status/category:** Explicit contracts usable; consumer recovery friction.
- **Root/impact:** Borrowed items/contexts, expiring handles and copied bytes require distinct app bookkeeping.
- **Evidence and reproducibility:** E1/E2/E4: lifetime checks, recovery, duplicate objects, copied snapshot data versus borrowed Group pointers.
- **Change/dependencies/disposition:** Retain contracts; optional worked recovery guidance; related High-5 and Medium-3, not a new API requirement.

## Medium-7 — Callback reentrancy could observe unsafe source state

- Historical metadata: **Relevant in:** V4 managed/snapshot APIs
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Change type:** **Small/medium refactor + contract**

- **Current action priority:** None.
- **Current status/category:** Reentry guard reproduced.
- **Root/impact:** Callbacks must not access half-committed same-source state. Guard is not a lock.
- **Evidence and reproducibility:** E2/E3: managed comparator and capture callback reentry rejected, source preserved.
- **Change/dependencies/disposition:** Retain invariant; archive unsafe-reentry mechanism; no general locking implied.

## Medium-8 — Snapshot lifetime could accidentally retain/dereference former live source

- Historical metadata: **Relevant in:** V4 Preview.3
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Change type:** **Medium refactor**

- **Current action priority:** None.
- **Current status/category:** Detached snapshot lifetime reproduced.
- **Root/impact:** Former-source pointers would create dangling historical access.
- **Evidence and reproducibility:** E1/E3: source destroyed before history validation; copied data survives edits.
- **Change/dependencies/disposition:** Archive historical lifetime issue; retain source-marker lifetime tests.

## Medium-9 — Restore could become O(N log N) by replaying ordinary insertions

- Historical metadata: **Relevant in:** V4 Preview.3
- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Change type:** **Medium refactor**

- **Current action priority:** None.
- **Current status/category:** Resolved bulk restore structure.
- **Root/impact:** Replaying ordinary insertion would add avoidable searches. Current builder directly constructs balanced blocks.
- **Evidence and reproducibility:** E3/E5/E6: restore executed; source inspection confirms one bulk construction. Timing alone is not a complexity proof.
- **Change/dependencies/disposition:** Archive old restore strategy; resident allocation still High-1, no new restore index.

## Medium-10 — V3 LK1 data migration into V4

- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Change type:** **New compatibility feature**

- **Current action priority:** None.
- **Current status/category:** Migration exercised successfully.
- **Root/impact:** Strict legacy coordinate interpretation must not leak a second live API.
- **Evidence and reproducibility:** E2/E4/E7: valid negative/ZERO/positive import, duplicate rejection, OOM rollback and frozen goldens.
- **Change/dependencies/disposition:** Retain one-way importer and corpus; archive missing-migration gap.

## Medium-11 — Comparator count alone hid structural cost

- Historical metadata: **Identified:** V1
- Historical metadata: **Current status:** **Mostly resolved**
- Historical metadata: **Change type:** **Instrumentation / research**

- **Current action priority:** Low.
- **Current status/category:** Mostly resolved by separated measurements.
- **Root/impact:** Comparator count omits allocations, copying and OS/consumer work.
- **Evidence and reproducibility:** E5: actual operation/phase timings, requested traffic, working set; no invented private counters.
- **Change/dependencies/disposition:** Update evidence; retain attribution gaps and real workload need, not more counters by default.

## Medium-12 — Pathological input distributions lacked sufficient coverage

- Historical metadata: **Identified:** V1
- Historical metadata: **Current status:** **Mostly resolved**
- Historical metadata: **Change type:** **Testing / research**

- **Current action priority:** Low.
- **Current status/category:** Broader generated coverage; distribution evidence still limited.
- **Root/impact:** Finite synthetic distributions cannot cover actual user workflows.
- **Evidence and reproducibility:** E1/E5/E7: seeds, blocks, hotspot/churn/drag/history; current tests remain finite.
- **Change/dependencies/disposition:** Merge remaining evidence demand with Highest-6; do not reopen V3 pathologies.

## Medium-13 — Benchmark/product comparison against ordinary sorting was misleading

- Historical metadata: **Present:** early project positioning
- Historical metadata: **Current status:** **Mostly resolved**
- Historical metadata: **Change type:** **Documentation / product positioning**

- **Current action priority:** Low.
- **Current status/category:** Mostly resolved positioning.
- **Root/impact:** One-shot sort comparison is a poor proxy for editor ordering.
- **Evidence and reproducibility:** E1/E8: workflow-oriented consumer, no competing sort rankings.
- **Change/dependencies/disposition:** Merge continuing product discipline with Highest-5; no algorithm action.

## Medium-14 — Public allocator / no-heap mode absent

- Historical metadata: **Raised:** V1
- Historical metadata: **Current status:** **Future candidate**
- Historical metadata: **Change type if pursued:** **New feature + medium/large design**

- **Current action priority:** None.
- **Current status/category:** Optional capability, not missing correctness.
- **Root/impact:** Public no-heap/custom allocation would create new ownership/capacity contracts.
- **Evidence and reproducibility:** E4: external research wrappers suffice for this evaluation; no public hook needed.
- **Change/dependencies/disposition:** Keep deferred; separate evidence-driven feature design, not automatic pool/public API coupling.

## Medium-15 — General mutable-container thread safety absent

- Historical metadata: **Present:** all versions
- Historical metadata: **Current status:** **Intentionally not solved**
- Historical metadata: **Change type:** **None / feature reduction**

- **Current action priority:** None.
- **Current status/category:** Deliberate caller synchronization boundary.
- **Root/impact:** Marker lifetime atomics do not make mutable order operations synchronized.
- **Evidence and reproducibility:** E1/E8: single-thread editor complies; no race testing of unsupported shared mutation.
- **Change/dependencies/disposition:** Retain non-goal; no internal locking proposal justified.

## Medium-16 — Few bindings/integrations/adapters

- Historical metadata: **Current status:** **Unresolved**
- Historical metadata: **Change type:** **New peripheral features**

- **Current action priority:** Low.
- **Current status/category:** One generated C adapter delivered; external integrations still sparse.
- **Root/impact:** Business identity/resolver/command policies belong in real consuming applications.
- **Evidence and reproducibility:** E1/E7: named-layer editor and public integrations; no third-party adoption claim.
- **Change/dependencies/disposition:** Update progress, retain external demand gap; adapters optional, not a core expansion requirement.

## Low-1 — V1 `lks_path_between(negative, positive)` contract ambiguity

- Historical metadata: **Affected:** V1 historical Path API
- Historical metadata: **Current status:** **Superseded**
- Historical metadata: **Change type:** **None**

- **Current action priority:** None.
- **Current status/category:** Superseded legacy API.
- **Root/impact:** Old negative/positive gap semantics no longer exist as a V4 live function.
- **Evidence and reproducibility:** E8: public header/manifest and historical issue lineage.
- **Change/dependencies/disposition:** Archive; no V4 fix or reproduction attempted.

## Low-2 — “Immediately before/after” wording was mathematically imprecise

- Historical metadata: **Affected:** early Path documentation
- Historical metadata: **Current status:** **Resolved / superseded**
- Historical metadata: **Change type:** **Documentation**

- **Current action priority:** None.
- **Current status/category:** Resolved/superseded wording.
- **Root/impact:** Dense-coordinate immediate adjacency was historically ambiguous; live occurrence adjacency is concrete.
- **Evidence and reproducibility:** E1/E8: before/after/adjacent no-op semantics exercised.
- **Change/dependencies/disposition:** Archive; no API change.

## Low-3 — Local optimized-MSVC executable occasionally disappears after successful link

- Historical metadata: **Observed:** V4 Preview.3/4 local environment
- Historical metadata: **Current status:** **Unresolved, low confidence as repository defect**
- Historical metadata: **Change type:** **Investigation**

- **Current action priority:** Low.
- **Current status/category:** Not reproduced; historical cause unverified.
- **Root/impact:** Executable disappearance could involve environment; repository causality unknown.
- **Evidence and reproducibility:** E7: fresh MSVC optimized consumer and baseline 39/39, ASan pass; executables persist during runs.
- **Change/dependencies/disposition:** Retain low-confidence investigation only if reproducible; no protection changes or production workaround.

## Low-4 — GitHub Topics still contain older positioning terms

- Historical metadata: **Current status:** **Unresolved**
- Historical metadata: **Change type:** **Metadata reduction/replacement**

- **Current action priority:** Low.
- **Current status/category:** Metadata mismatch persists.
- **Root/impact:** Repository topics retain path-keys/sorting/stable-sorting alongside correct dynamic-order description.
- **Evidence and reproducibility:** E9: live repository metadata read; no algorithm impact.
- **Change/dependencies/disposition:** Optional metadata cleanup, merge with Highest-5; no GitHub edits authorized here.

## Low-5 — Chinese documentation was substantially thinner than English

- Historical metadata: **Current status:** **Mostly resolved**
- Historical metadata: **Resolution:** V3/V4 onboarding work + post-v4 documentation convergence
- Historical metadata: **Change type:** **Documentation**

- **Current action priority:** Low.
- **Current status/category:** Documentation breadth improved; user testing absent.
- **Root/impact:** Language coverage and actual beginner comprehension are different evidence.
- **Evidence and reproducibility:** E8: current translated onboarding exists; this English engineering task is not human usability validation.
- **Change/dependencies/disposition:** Retain achieved docs progress; optional real reader review, no broad rewrite.

## Low-6 — Historical roadmap and active roadmap were mixed together

- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Resolution:** post-v4 Stable project-maintenance pass
- Historical metadata: **Change type:** **Documentation/project management**

- **Current action priority:** None.
- **Current status/category:** Resolved governance/history separation.
- **Root/impact:** Historical and active debt previously shared one active issue.
- **Evidence and reproducibility:** E8: #1/#2 closed history; #3 active Full; #4 historical ledger.
- **Change/dependencies/disposition:** Archive missing separation; retain exact issue roles, no updates performed.

## Low-7 — Current documentation still contained development-period wording after Stable

- Historical metadata: **Current status:** **Resolved**
- Historical metadata: **Change type:** **Documentation**

- **Current action priority:** None.
- **Current status/category:** Current-facing Stable wording consistent in reviewed core docs.
- **Root/impact:** Historical freeze/Preview files intentionally preserve their original release state.
- **Evidence and reproducibility:** E8: API/Usage/Architecture/Compatibility/Integration/Validation current Stable; historical labels not defects.
- **Change/dependencies/disposition:** Archive active wording issue; do not rewrite history.

## Low-8 — Mini edition does not exist

- Historical metadata: **Current status:** **Future candidate**
- Historical metadata: **Current importance:** Low
- Historical metadata: **Change type if pursued:** **Feature reduction + medium/large product design**

- **Current action priority:** None.
- **Current status/category:** Outside Full scope; historical future wording already qualified.
- **Root/impact:** Another product line cannot resolve Full allocation/export costs.
- **Evidence and reproducibility:** E8: issue preface explicitly qualifies old entry; no evaluation of that product performed.
- **Change/dependencies/disposition:** Archive from Full active worklist; preserve historical text, no cross-product conclusions.

## Low-9 — Embedded/MCU edition does not exist

- Historical metadata: **Current status:** **Future candidate**
- Historical metadata: **Current importance:** Low
- Historical metadata: **Change type if pursued:** **Major feature reduction + large specialized redesign**

- **Current action priority:** None.
- **Current status/category:** Optional uncommitted product concept.
- **Root/impact:** Resource-constrained requirements would need separate ownership/capacity design.
- **Evidence and reproducibility:** E8: no evidence this Full consumer needs an MCU/no-heap product.
- **Change/dependencies/disposition:** Keep out of Full remediation; no new product work authorized.

## Low-10 — Package-manager-specific distribution is limited

- Historical metadata: **Current status:** **Unresolved but optional**
- Historical metadata: **Change type:** **New peripheral feature**

- **Current action priority:** None.
- **Current status/category:** Optional packaging convenience.
- **Root/impact:** Additional recipes do not affect current supported source/package integration.
- **Evidence and reproducibility:** E7: official ZIP and installed/subdirectory consumption succeeded.
- **Change/dependencies/disposition:** Keep optional demand-driven feature, not a Stable blocker.
