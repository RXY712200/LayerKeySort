# V4 compatibility

[V4 freeze](V4_FREEZE.md) defines 4.x source/layout and LS1/LKS4SNP1 persistent
compatibility. Source rebuild supported; no universal cross-toolchain/CRT/architecture
binary ABI. V3 and earlier Preview source compatibility deliberately breaks at
Preview.5. See [migration](MIGRATION_V3_V4.md). V3.1.0 remains separately available.

## V4.0.0 Stable symbol model

CMake explicitly builds a STATIC library; no shared DLL/SO ABI is shipped, even
when BUILD_SHARED_LIBS is set. The public header/manifest defines the 64 supported
functions. An archive symbol listing is not a public API listing: private
allocator/sort/snapshot helpers and historical Path codecs needed by strict LK1
migration have ordinary object linkage. On the reviewed GCC build, nm reports
64 public and 43 private lks_* text symbols. No live legacy Tree/OrderedTree,
Path-gap or legacy Group symbols occur in the ordinary archive. Historical
regression libraries deliberately contain more code. Installed consumers must
not declare or call private symbols. A future shared-library integration must
choose export visibility explicitly; universal dynamic ABI is not promised.

V4.0.0 preserves the Preview.5 source and wire freeze after independent RC review.
See [Stable record](V4_STABLE.md) and [RC validation record](V4_RC1.md).
