# LLOW trace format 1

Text uses ASCII-compatible UTF-8, tabs as separators and LF or CRLF line endings.
The application normalizes a trailing CR at line end. There are no addresses,
private node details or implementation-dependent order coordinates.

Header: `LLOW<TAB>1<TAB>SOURCE<TAB>MODE`. SOURCE is `human-terminal`,
`codex-interactive`, `script`, `generated` or `unspecified`; MODE is `stdin`,
`file` or `replay`. Terminal provenance is rejected for nonterminal execution.
Operator identity is a caller declaration, not externally authenticated evidence.
When replay is recorded, its new header identifies the current replay execution;
retain the input trace as the original provenance artifact.

Each command record has exactly 12 fields:

| Field | Meaning |
| --- | --- |
| 1 | Sequential processed command number, starting at 1 |
| 2 | Recognized command name, or `invalid` |
| 3 | Input-reader error (`-`, `APP_LINE_LIMIT`, `APP_INPUT_FORMAT`) |
| 4 | Business object ID: payload ID, or newly created object ID; 0 denotes NULL/absent |
| 5 | Occurrence ID: operand or newly inserted occurrence; 0 if absent/failed insertion |
| 6 | Anchor/comparison second occurrence ID; 0 if absent |
| 7 | Pre-command live count |
| 8 | Expected outcome status: `OK`, an application error, or an observed Mini error |
| 9 | Post-command expected live count |
| 10 | Exact comparison relation; 0 for non-comparison/errors |
| 11 | Original normalized command bytes as lowercase hex |
| 12 | Complete expected occurrence sequence as comma-separated IDs, or `-` if empty |

The sequence is produced from the independent array oracle, never from Mini
traversal. Each ID has a payload assignment recorded by its insertion command;
create commands record the object name in the hex command. This is enough to
reconstruct duplicate business references and NULL occurrences independently.

Completion footer: `LLOW-END<TAB>processed-command-count`. Missing/wrong footer,
trailing data, malformed version/fields, out-of-sequence commands or different
statuses/order/IDs are rejected with nonzero exit. A valid earlier trace prefix
can only become a new trace by intentionally writing a new valid footer; this
format is not cryptographic authentication or a persistence format for Mini.

Blank/comment source lines are ignored, not assigned sequence numbers. Invalid
commands are recorded, including their error outcome. Overlong inputs retain only
the first 255 bytes plus `APP_LINE_LIMIT`; NUL-containing inputs omit NUL bytes and
record `APP_INPUT_FORMAT`. This reproduces the rejected operation/result, **not**
the complete original oversized/binary input. Other reader errors are fatal.
An `APP_*` status is not a simulated Mini status. Runtime allocation failure can
be recorded but ordinary replay may fail if the allocation no longer fails;
there is no fault-injection command in the shipped application.

Records are bounded to 32,767 bytes, sufficient for the 2,048-live-ID application
limit. No recorded command accesses a retired handle. Ordinary command files stop
at successful `quit`; replay requires only the footer afterward. A failed I/O or
oracle validation run may leave an incomplete file, which replay rejects.

The checked-in [scenario evidence](../scenarios/evidence/) is original script
execution, and CTest verifies fresh recordings against it and replays both. Test
drivers preserve generated traces/stdout context under build `evidence/`; no
generated 2,048-item workload is presented as real-user editor activity.
