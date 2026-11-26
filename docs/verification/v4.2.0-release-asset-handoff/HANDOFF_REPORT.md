# LayerKeySort v4.2.0 Original Asset Verification Handoff

**Result: VERIFIED**

Pinned final main: `f751e1ee2afd682874281d6dc495969f9eca49eb`.
Pinned tree: `18261736d37cba995a20d9a3132313d954ff3ef1`.

Task document: `D:\LayerKeySort\.local\tasks\2026-11-27-v420-original-release-asset-verification.md`.
Handoff directory: `D:\LayerKeySort\.local\handoff\v4.2.0-original-assets-20261127-d869d5d7`.

## Originals and byte-identical staged copies

- **LayerKeySort-4.2.0-amalgamation.zip**
  - Original: `D:\LayerKeySort\.local\full-v4.2.0-final-f751e1e\build\v4.2.0-publication\assets\LayerKeySort-4.2.0-amalgamation.zip`.
  - Copy: `D:\LayerKeySort\.local\handoff\v4.2.0-original-assets-20261127-d869d5d7\LayerKeySort-4.2.0-amalgamation.zip`.
  - Size: 35696 bytes.
  - SHA-256: `1b02e7c8b5137bc4cae7d2c3e9859edf3158802bc02ba1aad593ebd9d1ba3bdb`.
  - Byte-for-byte comparison: True.

- **LayerKeySort-4.2.0-SHA256SUMS.txt**
  - Original: `D:\LayerKeySort\.local\full-v4.2.0-final-f751e1e\build\v4.2.0-publication\assets\LayerKeySort-4.2.0-SHA256SUMS.txt`.
  - Copy: `D:\LayerKeySort\.local\handoff\v4.2.0-original-assets-20261127-d869d5d7\LayerKeySort-4.2.0-SHA256SUMS.txt`.
  - Size: 102 bytes.
  - SHA-256: `36e06a53ad41f0a01ee0435e5c81ec81b8611b61df2351f5cf322ea1acd6f68e`.
  - Byte-for-byte comparison: True.

- **PUBLICATION.json**
  - Original: `D:\LayerKeySort\.local\full-v4.2.0-final-f751e1e\build\v4.2.0-publication\PUBLICATION.json`.
  - Copy: `D:\LayerKeySort\.local\handoff\v4.2.0-original-assets-20261127-d869d5d7\PUBLICATION.json`.
  - Size: 2762 bytes.
  - SHA-256: `c3fd0015eb2e1ee31a15f028e1a3489fcf1dbb8388bc4b170a30e51431a74b33`.
  - Byte-for-byte comparison: True.

## ZIP member inventory

| Exact member | Uncompressed bytes | Compressed bytes | CRC32 | SHA-256 |
|---|---:|---:|---|---|
| `LayerKeySort-4.2.0-amalgamation/example.c` | 806 | 401 | `a38ec1a0` | `305f5e981fd079bf09d6530b0d65b0d9e1d700377b460365d10bcebbc9e556e7` |
| `LayerKeySort-4.2.0-amalgamation/layerkeysort.c` | 127600 | 27848 | `cf537472` | `ea40c37b222577784a7974471cce25a11d6bcb68395c1c5c46fae5aa0778cc10` |
| `LayerKeySort-4.2.0-amalgamation/layerkeysort.h` | 16784 | 5379 | `2f675a9c` | `9c755fa05ea1ed0e9d0a00860afb5b2854315c3dd10c5ac5d53b6a9e87a91c39` |
| `LayerKeySort-4.2.0-amalgamation/LICENSE` | 1066 | 631 | `dfa13558` | `114b7a8bc6e367d27ccad97352e71f998e8b8f933db28f0f769ca3cac38a3df5` |
| `LayerKeySort-4.2.0-amalgamation/README.txt` | 1081 | 607 | `223c07b5` | `b64dbf9edcfaaf8572be58bc7a508d963e893efc56a2f16783628fed27818cca` |

## Provenance and verification method

Python hashlib SHA-256 and PowerShell Get-FileHash (.NET) independently hashed all originals and staged copies. Expected asset hashes and the canonical one-line checksum contents were checked.
Exact membership comes from the pinned verifier AST and packaging scripts. ZIP decompression/CRC, member paths, encryption flags, sizes and member SHA-256 hashes were inspected.
Header, example and license were compared to exact pinned Git source with the packager UTF-8/BOM/newline normalization. README was compared to the complete literal in pinned packaging code. Existing amalgamated C wrappers, private-header dependency order and every production-source section were compared to pinned inputs and the local-include transformation. No asset generator was invoked; no replacement ZIP or source member was written.
Checked 24 existing amalgamation sections; source/input blob IDs and per-section hashes are in verification.json.
PUBLICATION.json target commit/tree, asset metadata, member order, Stable release flags and annotated-tag plan were checked. Recorded CI provenance was checked for internal SHA consistency only; no remote CI was rerun or independently revalidated in this task.

## Source identities

| Pinned input | Git blob | Raw SHA-256 |
|---|---|---|
| `tools/package_release.py` | `eb70dc1b017ce94e3c1a367bec82025ea3e4539d` | `c4e201ac115b8b8858d51ca9e4f938e8761e2d4598874baf1f900d6f3db0fc4e` |
| `tools/amalgamate.py` | `d1e42c5f7621a31c5a4c02639163a4e97083b101` | `b35e188eefcbb62336c01dd0e1fbebf42a53a2768a1fcf232f2f1a39794812af` |
| `tools/verify_release_assets.py` | `f334dbdc1b27c37282added3f696463ff96b8da1` | `5645030d262456b79cb1e38e5f1c11d181f286449018cf7deb7a942238bc780d` |
| `include/layerkeysort.h` | `c3d56175500e051d8a90c6ddd482865a86b7ac45` | `9c755fa05ea1ed0e9d0a00860afb5b2854315c3dd10c5ac5d53b6a9e87a91c39` |
| `CMakeLists.txt` | `def4b74ad2732d104a2eb0f902dd197381d874c8` | `3e3330c00bcc57f70c98d7a5628411e8ced07bb08b7f531e897889263385c52f` |
| `cmake/ProductionSources.cmake` | `c7d827f1e3f200f03b1ca06e65ed592ee1595f43` | `88b7e9c2198d3ec6f7bd7e4de7fd9bb07e27d84c47924fabf2417e35941da4f6` |
| `LayerKeySort.vcxproj` | `5885e5a2119679750140a37e976ae28e1a9e3b52` | `3f979283a0f03450007c2e5d5baa85ad46022b7fa2a2b4a1a1997f742c4dbf19` |
| `LayerKeySort.vcxproj.filters` | `110e145e8b33c7043f95cf136bfb2fedab7452b3` | `9720b3066ccaba23030a48259d3c7b33a6a0f6160f08d1772e7cb062ea40f24f` |
| `src/lks_alloc.c` | `f082212f0755aafae3c1da7b95b918e992f1d8ad` | `90ac678759746beaf78571ef36ed0c53ed791b3b18fbd9223914b08efa23b752` |
| `src/lks_alloc_internal.h` | `c48f1b673da08df685607c3930ca1dc0346292d8` | `b19087b4f944cde30bf1aa004744277748911d46b24213ecbd948b9c3a291921` |
| `src/lks_base.c` | `df125c22797228820535697cc56e694f1de90465` | `f802214ac98c488b7e91098a544faed6363d322c009ca1cecdd7d6b89a8ebc55` |
| `src/path.c` | `94637e8da351baa1b3e4ad0b5c24135dc9cbd37e` | `d4f0b24c31afb38d37d1401c1335e9c39b2cf806f0382ecfaa3ade98e23c7d98` |
| `src/lks_legacy_internal.h` | `f97f2ae001226d15238cfdb171eb002f6d764e44` | `6564eb3d595b2c60e7d9cd09520339935e8babd01770c356a78b9a2c24b361d5` |
| `src/lks_path_internal.h` | `1f1449b1090b13c6e9fc1e112a4ef064d1eca1b4` | `d409b46a400c29cc57305499de17944fc7317dc0ea5883798e61362e430f41fb` |
| `src/path_text.c` | `3a170cade21483286e4ff7d3bbaa078ea2237518` | `1ae6bd37fb291776d4195dea12a5868ad2cc9a7b8fea783f332bc13fdb5460c9` |
| `src/lks_slot_codec_internal.h` | `7432ddb391fa9cfd9ae435102a716d5086d152c7` | `b74580819010a42bb0b965a76ed076fdf00bed61912e75601c2d8d3120f767a7` |
| `src/path_order_key.c` | `19f9a39f74210365795dd57015a47fb788c5b38e` | `95eebfa3c92fcb1caf9bdd43e4562b9163ea72421eef0ee662ec8f127095f1c0` |
| `src/slot_codec.c` | `ec02b027470e8efbdb922a934018019e47b4a212` | `9cf0403472aaa7fe98264254ccd76f50ed92ca037eba313fc49b2b3342975cd9` |
| `src/path_compare.c` | `06665905c8443b9fa39fcc21207ed12fc7622c20` | `c2c8c0a04cc87216b439421451f8c05d4839333874fa5e575de6e9275d73b218` |
| `src/order.c` | `cde7f4f7a349a8a228c03a6191ebcce7e702e35a` | `b4855df70bd56aebfe80c266a4eadf411ead2c810d03332c6bc50372aa899fed` |
| `src/lks_order_internal.h` | `e9d3631d109a2f64c06f42c383182277c3c3b5a0` | `e8510ccce6a5f53516e7ab468fa37c18749accac24f0eb6247c44796052185c5` |
| `src/managed_order.c` | `194df6ce65525f9370d1827595c3750727d8220e` | `d03e428b294c6b75533551e06fa5dcc070e319614ee8104760f960e339964247` |
| `src/snapshot.c` | `643619d7f608383311aa07e4acbace8d13af6522` | `f0808ef169cb28c248e0119061fe7b4ac1373c50aa6e2c1055807697c9bd32e2` |
| `src/lks_snapshot_internal.h` | `2b41aa9765a60a9883724557efa62ba80ae4a942` | `7fe64c2b59b724ae2c753a846e8fcbd89459d84f6ed5efe006f72ae99e50f5bf` |
| `src/snapshot_key.c` | `2f4a848ace40b0553de1e1be44f4cc2c8f661193` | `30ef08cfe89d78002bbbce1ef0558658c50fb2f921e8cf1cd838d7e8719b0721` |
| `src/snapshot_wire.c` | `66edbca124562ee49660e4a7d2b0d6209b8cfc87` | `fd1d6f90874ce401db00471b93207dcffa4d808842657d0123e371b05f181c26` |
| `src/snapshot_restore.c` | `1ac5e3c5c1d72addb8a5b0ecc9846e4240cae4fd` | `cfadf461e578673e377b195f28dc649fc3d6fa48582793592849142f56f8da1b` |
| `src/order_bulk.c` | `32f8983686224770be825da81fa77ed89ec7bd50` | `24c7f2b95a7ea4839f54971bff518ccf2ddf8379700d5e4aeab40e33e8187448` |
| `src/immutable_group.c` | `0ad62923e49074349b5dd76f882c8426e9684630` | `6886a8b28a53cbe183d68ec5dcb1bbb2feb4937c745335ea0fc02014e1ae09da` |
| `src/lks_immutable_group_internal.h` | `45bbaaede6919278fd8a07d36bc9b76f4f7492f2` | `96aa0546e514d8d4cf835e35079205baf8b29da7ad0db12dbcd335156108f179` |
| `src/lks_sort_internal.h` | `859653d2b9c66e9494d842bf3aecf3399cb6b07b` | `f79d33fe3ad151a12e3a747fa185fe5f91af4793a17f10b8e4c434e806de0159` |
| `src/sort.c` | `4214d1d5f1eb64eff865df4f4672ab46d04d3b21` | `0d870990f456c50fc30b360889e5e9888100565c700f449ef4957b62b3c32208` |
| `examples/basic.c` | `997a831ded6414823bbd02a1451bf59871fd1ae5` | `305f5e981fd079bf09d6530b0d65b0d9e1d700377b460365d10bcebbc9e556e7` |
| `LICENSE` | `82dbe349e2440d2d4ca4f17924ecb50d83b9e6d5` | `114b7a8bc6e367d27ccad97352e71f998e8b8f933db28f0f769ca3cac38a3df5` |

## Storage and preservation

D:/LayerKeySort is a workspace container, not a Git root. .local/tasks and .local/handoff are outside all listed Git worktrees and cannot enter their Git inventories. No ignore rule is created or changed. The original build/ directory is ignored by the pinned worktree existing rule.
Original byte hashes, file sizes, modification and creation timestamps were checked again at completion. Reading may affect OS-managed access times; no original content or timestamp setter was used.
Git state and local ref snapshots are included in verification.json. No commits, pushes, PR changes, merges, tags, Releases, uploads, builds, benchmarks, production edits, governance edits or ignore-rule changes were performed.

## Discrepancies

No asset integrity, source-correspondence or publication-metadata discrepancy found. Storage-layout clarification: the workspace-container .local/tasks and .local/handoff directories are outside Git worktrees rather than covered by a Git ignore rule.

## Executed checks

- PASS: Pinned object is a Git commit
- PASS: D:\LayerKeySort\.local\tasks\2026-11-27-v420-original-release-asset-verification.md is outside every Git worktree
- PASS: D:\LayerKeySort\.local\handoff\v4.2.0-original-assets-20261127-d869d5d7 is outside every Git worktree
- PASS: Original publication build directory is already Git-ignored
- PASS: Original is a regular non-symlink file: LayerKeySort-4.2.0-amalgamation.zip
- PASS: Original SHA-256 matches supplied expectation: LayerKeySort-4.2.0-amalgamation.zip
- PASS: Staged copy is byte-for-byte identical: LayerKeySort-4.2.0-amalgamation.zip
- PASS: Original is a regular non-symlink file: LayerKeySort-4.2.0-SHA256SUMS.txt
- PASS: Original SHA-256 matches supplied expectation: LayerKeySort-4.2.0-SHA256SUMS.txt
- PASS: Staged copy is byte-for-byte identical: LayerKeySort-4.2.0-SHA256SUMS.txt
- PASS: Original is a regular non-symlink file: PUBLICATION.json
- PASS: Original SHA-256 matches supplied expectation: PUBLICATION.json
- PASS: Staged copy is byte-for-byte identical: PUBLICATION.json
- PASS: independent_original_hashes: Python hashlib and .NET agree for LayerKeySort-4.2.0-amalgamation.zip
- PASS: independent_original_hashes: Python hashlib and .NET agree for LayerKeySort-4.2.0-SHA256SUMS.txt
- PASS: independent_original_hashes: Python hashlib and .NET agree for PUBLICATION.json
- PASS: independent_staged_hashes: Python hashlib and .NET agree for LayerKeySort-4.2.0-amalgamation.zip
- PASS: independent_staged_hashes: Python hashlib and .NET agree for LayerKeySort-4.2.0-SHA256SUMS.txt
- PASS: independent_staged_hashes: Python hashlib and .NET agree for PUBLICATION.json
- PASS: Canonical checksum file names the exact ZIP and independently verified hash
- PASS: Pinned public header and CMake identify Stable 4.2.0
- PASS: Exact five-member manifest identified in pinned verification script AST
- PASS: Pinned canonical manifest has 16 unique production modules
- PASS: LayerKeySort.vcxproj compile membership matches canonical manifest plus four legacy regression modules
- PASS: LayerKeySort.vcxproj.filters compile membership matches canonical manifest plus four legacy regression modules
- PASS: ZIP contains exactly the five packaging-manifest members, with no duplicates
- PASS: Every ZIP member decompresses successfully and passes CRC verification
- PASS: Safe, unencrypted regular ZIP member: LayerKeySort-4.2.0-amalgamation/example.c
- PASS: Declared uncompressed size matches actual bytes: LayerKeySort-4.2.0-amalgamation/example.c
- PASS: Safe, unencrypted regular ZIP member: LayerKeySort-4.2.0-amalgamation/layerkeysort.c
- PASS: Declared uncompressed size matches actual bytes: LayerKeySort-4.2.0-amalgamation/layerkeysort.c
- PASS: Safe, unencrypted regular ZIP member: LayerKeySort-4.2.0-amalgamation/layerkeysort.h
- PASS: Declared uncompressed size matches actual bytes: LayerKeySort-4.2.0-amalgamation/layerkeysort.h
- PASS: Safe, unencrypted regular ZIP member: LayerKeySort-4.2.0-amalgamation/LICENSE
- PASS: Declared uncompressed size matches actual bytes: LayerKeySort-4.2.0-amalgamation/LICENSE
- PASS: Safe, unencrypted regular ZIP member: LayerKeySort-4.2.0-amalgamation/README.txt
- PASS: Declared uncompressed size matches actual bytes: LayerKeySort-4.2.0-amalgamation/README.txt
- PASS: layerkeysort.h exactly matches normalized pinned include/layerkeysort.h
- PASS: example.c exactly matches normalized pinned examples/basic.c
- PASS: LICENSE exactly matches normalized pinned LICENSE
- PASS: README.txt exactly matches complete bilingual packaging literal in pinned tools/amalgamate.py
- PASS: Existing amalgamated C has exact canonical two-line preamble
- PASS: Existing amalgamation section order/wrapper: src/lks_alloc_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_alloc_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_legacy_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_legacy_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_path_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_path_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_slot_codec_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_slot_codec_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_order_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_order_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_snapshot_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_snapshot_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_immutable_group_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_immutable_group_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_sort_internal.h
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_sort_internal.h
- PASS: Existing amalgamation section order/wrapper: src/lks_alloc.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_alloc.c
- PASS: Existing amalgamation section order/wrapper: src/lks_base.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/lks_base.c
- PASS: Existing amalgamation section order/wrapper: src/path.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/path.c
- PASS: Existing amalgamation section order/wrapper: src/path_text.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/path_text.c
- PASS: Existing amalgamation section order/wrapper: src/path_order_key.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/path_order_key.c
- PASS: Existing amalgamation section order/wrapper: src/slot_codec.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/slot_codec.c
- PASS: Existing amalgamation section order/wrapper: src/path_compare.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/path_compare.c
- PASS: Existing amalgamation section order/wrapper: src/order.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/order.c
- PASS: Existing amalgamation section order/wrapper: src/managed_order.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/managed_order.c
- PASS: Existing amalgamation section order/wrapper: src/snapshot.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/snapshot.c
- PASS: Existing amalgamation section order/wrapper: src/snapshot_key.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/snapshot_key.c
- PASS: Existing amalgamation section order/wrapper: src/snapshot_wire.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/snapshot_wire.c
- PASS: Existing amalgamation section order/wrapper: src/snapshot_restore.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/snapshot_restore.c
- PASS: Existing amalgamation section order/wrapper: src/order_bulk.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/order_bulk.c
- PASS: Existing amalgamation section order/wrapper: src/immutable_group.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/immutable_group.c
- PASS: Existing amalgamation section order/wrapper: src/sort.c
- PASS: Existing amalgamation section content equals transformed pinned source: src/sort.c
- PASS: No extra unverified bytes or sections in existing amalgamated C
- PASS: No unresolved repository-local includes after amalgamation preamble
- PASS: PUBLICATION.json pins exact final main commit and Git tree
- PASS: PUBLICATION.json asset names and hashes match actual originals and supplied expectations
- PASS: PUBLICATION.json exact ZIP member list and order match original archive
- PASS: PUBLICATION.json annotated-tag plan is correct and explicitly unexecuted
- PASS: PUBLICATION.json Stable release identity/flags are correct and explicitly unexecuted
- PASS: Recorded original asset-generation directory agrees with original final-main worktree
- PASS: Recorded five CI metadata entries consistently identify the pinned commit (not a new CI execution)
- PASS: Original bytes, size, modification and creation timestamps unchanged: LayerKeySort-4.2.0-amalgamation.zip
- PASS: Original bytes, size, modification and creation timestamps unchanged: LayerKeySort-4.2.0-SHA256SUMS.txt
- PASS: Original bytes, size, modification and creation timestamps unchanged: PUBLICATION.json
- PASS: Git HEAD, branch, tracked/untracked state and all local refs unchanged

The copied PUBLICATION.json is preserved exactly, including its original generation paths. It has not been rewritten to refer to the handoff copies. This local verification does not authorize publication.
