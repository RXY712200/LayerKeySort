# V4 architecture

Contextual live order owns occurrence records in locally repaired B64 blocks and
balanced block index. Handles address occurrences, not coordinates; moves preserve
handles. ManagedOrder binds comparison semantics to the same core. No second
engine, public rank/physical navigation or live Path.

Group owns flat borrowed-pointer sequence. Shared snapshot capture prepares copied
namespace/association rows, then publishes provenance; atomic marker lifetime does
not supply locking. Snapshots are historical, independent of live/application data.
Private strict Path/LK1 migration remains. Legacy live Tree/Group/gap/bulk modules
are historical regression/benchmark infrastructure, not production/amalgamation.
No private header installed. [Freeze](V4_FREEZE.md) is implementation-independent.
