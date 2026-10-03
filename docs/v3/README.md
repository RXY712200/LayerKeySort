# V3 visualizer fixture provenance

The default Pages destination is [index.html](index.html). The browser does
not run C or implement Path allocation. `fixtures.js` contains complete Path
snapshots for two deterministic `LksOrderedTree` insertion sequences:

- mixed values `20, 10, 20, 30, 15, 20, 11, 12, 13, 14`;
- an interior hotspot: `0, 100`, followed by 23 items of value `50`.

The snapshots were recorded with [record_fixtures.c](record_fixtures.c)
linked against the exact published RC.1 production library at
`e7508550533f7c5f7f74b6c873fd8800576db954`. The unreleased Stable
candidate changes no `src/*.c` file. The recorder traverses public physical
navigation only to collect every item-to-Path association after each insert;
it separately checks every pair against `lks_path_compare()` and the numeric
comparator's stable equal-item order. It compares consecutive snapshots to
count existing coordinates changed. The hotspot's 23rd insertion relabels
eight existing Paths in this recorded run.

The browser sorts the recorded items by numeric comparator value and original
insertion index, then displays the exact recorded Path for each item. It
asserts each snapshot's changed count at runtime. This is a finite replay,
not a claim that a JavaScript implementation matches future private Path
policy. Exact generated coordinates are not a stable application identity or
a 3.x compatibility promise. The manual mode takes its order and final HUD
display/LK1 pair from the public [layer-list example](../../examples/layer_list.c);
it intentionally does not invent intermediate Path outputs.

The [historical V1 showcase](../demo/index.html) is retained separately and
does not describe V3 Path assignment.

`node test_replay.cjs` checks Step, Reset, Play, Pause, mode changes, and
the eight changed-coordinate highlights without a browser or network access.
The script exercises the page logic with a small DOM stand-in; visual browser
layout and browser-specific runtime behavior require separate review.
