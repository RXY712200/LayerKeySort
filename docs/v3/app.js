"use strict";

// Recorded C snapshots are data, never a browser Path allocator.
const fixtures = window.LKS_V3_FIXTURES;
const manualSteps = [
  { title: "Initial manual coordinates", detail: "Insert Background, Player, and HUD at application-chosen Paths.", order: [[101, "Background"], [202, "Player"], [303, "HUD"]] },
  { title: "Insert Effects between layers", detail: "The application calls lks_path_between(), then lks_tree_insert().", order: [[101, "Background"], [404, "Effects"], [202, "Player"], [303, "HUD"]] },
  { title: "Move HUD without changing its ID", detail: "The application chooses a new coordinate and calls lks_tree_rekey(). HUD remains item ID 303.", order: [[101, "Background"], [404, "Effects"], [303, "HUD"], [202, "Player"]] },
  { title: "Remove Effects", detail: "Exact-Path removal returns the borrowed item pointer. The application still owns Effects.", order: [[101, "Background"], [303, "HUD"], [202, "Player"]] },
  { title: "Persist one current coordinate", detail: "The public C example prints HUD id=303, display=02bR, key=LK1:201FF06D6! and verifies an LK1 round trip.", order: [[101, "Background"], [303, "HUD"], [202, "Player"]] }
];

const byId = (id) => document.getElementById(id);
const controls = {
  managed: byId("managed-mode"), manual: byId("manual-mode"), scenario: byId("scenario"),
  step: byId("step"), play: byId("play"), pause: byId("pause"), reset: byId("reset"), speed: byId("speed")
};
let mode = "managed";
let step = 0;
let timer = null;

function scenario() {
  const match = fixtures.scenarios.find((candidate) => candidate.id === controls.scenario.value);
  if (!match) throw new Error("Unknown recorded scenario");
  return match;
}
function maxStep() { return mode === "managed" ? scenario().steps.length : manualSteps.length; }
function stop() {
  if (timer !== null) clearTimeout(timer);
  timer = null;
  controls.play.disabled = false;
  controls.pause.disabled = true;
}
function cell(row, value, isCode = false) {
  const td = document.createElement("td");
  if (isCode) {
    const code = document.createElement("code");
    code.textContent = value;
    td.append(code);
  } else td.textContent = value;
  row.append(td);
}
function renderManaged() {
  const rows = byId("rows");
  const replay = scenario();
  const current = step === 0 ? null : replay.steps[step - 1];
  const previous = step < 2 ? null : replay.steps[step - 2];
  rows.replaceChildren();
  byId("table-caption").textContent = "Current logical order · numeric comparator, then insertion order for ties";
  byId("fixture-note").textContent = `Exact Path snapshots from production C at ${fixtures.sourceCommit}. The browser does not allocate or compare Paths; this candidate leaves production source unchanged.`;
  if (!current) {
    byId("event-title").textContent = "Ready to insert";
    byId("event-detail").textContent = `Recorded sequence: ${replay.title}. Choose Step or Play.`;
    byId("relabel-detail").textContent = "Existing Paths changed: 0";
    return;
  }
  const insertedId = step - 1;
  const values = replay.steps.slice(0, step).map((entry) => entry.value);
  const sortedIds = values.map((_, id) => id).sort((a, b) => values[a] - values[b] || a - b);
  const changedIds = previous ? sortedIds.filter((id) => id !== insertedId && previous.paths[id] !== current.paths[id]) : [];
  if (changedIds.length !== current.changed || current.paths.length !== step) throw new Error("Recorded fixture consistency failure");
  const rank = sortedIds.indexOf(insertedId) + 1;
  byId("event-title").textContent = `Inserted item #${insertedId + 1} · value ${current.value}`;
  byId("event-detail").textContent = `Comparator position ${rank} of ${step}. ${current.value === values[sortedIds[rank - 2]] ? "Equal values retain insertion order. " : ""}Assigned display Path: ${current.paths[insertedId]}.`;
  byId("relabel-detail").textContent = `Existing Paths changed: ${changedIds.length}${changedIds.length ? ` · items ${changedIds.map((id) => "#" + (id + 1)).join(", ")}` : ""}`;
  for (const [index, id] of sortedIds.entries()) {
    const row = document.createElement("tr");
    const isNew = id === insertedId;
    const isChanged = changedIds.includes(id);
    if (isNew) row.className = "new";
    else if (isChanged) row.className = "changed";
    cell(row, String(index + 1));
    cell(row, `#${id + 1}`);
    cell(row, String(values[id]));
    cell(row, current.paths[id], true);
    cell(row, isNew ? "New coordinate" : isChanged ? `${previous.paths[id]} → ${current.paths[id]}` : "Unchanged");
    rows.append(row);
  }
}
function renderManual() {
  const rows = byId("rows");
  rows.replaceChildren();
  byId("table-caption").textContent = "Application-controlled layer order · public layer-list example";
  byId("fixture-note").textContent = "Order and the final HUD display/LK1 pair come from examples/layer_list.c. Intermediate exact Paths are deliberately not invented here.";
  byId("relabel-detail").textContent = "Managed relabel: not applicable. Manual Tree coordinates are chosen by the application.";
  if (step === 0) {
    byId("event-title").textContent = "Ready for manual insertion";
    byId("event-detail").textContent = "The application supplies each Path. Choose Step or Play.";
    return;
  }
  const current = manualSteps[step - 1];
  byId("event-title").textContent = current.title;
  byId("event-detail").textContent = current.detail;
  current.order.forEach(([id, name], position) => {
    const row = document.createElement("tr");
    cell(row, String(position + 1));
    cell(row, `${name} · ID ${id}`);
    cell(row, "Caller-defined order");
    cell(row, step === 5 && id === 303 ? "02bR" : "Application-selected", step === 5 && id === 303);
    cell(row, step === 5 && id === 303 ? "LK1:201FF06D6!" : "See C example");
    rows.append(row);
  });
}
function render() {
  byId("managed-intro").hidden = mode !== "managed";
  byId("manual-intro").hidden = mode !== "manual";
  controls.managed.classList.toggle("active", mode === "managed");
  controls.manual.classList.toggle("active", mode === "manual");
  controls.managed.setAttribute("aria-pressed", String(mode === "managed"));
  controls.manual.setAttribute("aria-pressed", String(mode === "manual"));
  byId("progress").textContent = `${step} / ${maxStep()}`;
  controls.step.disabled = step >= maxStep();
  controls.play.disabled = timer !== null || step >= maxStep();
  if (mode === "managed") renderManaged(); else renderManual();
}
function advance() { if (step < maxStep()) { ++step; render(); } if (step >= maxStep()) stop(); }
function schedule() {
  if (timer !== null || step >= maxStep()) return;
  controls.play.disabled = true;
  controls.pause.disabled = false;
  timer = setTimeout(() => { timer = null; advance(); if (step < maxStep()) schedule(); }, Number(controls.speed.value));
}
function reset() { stop(); step = 0; render(); }
function setMode(next) { mode = next; reset(); }

controls.managed.addEventListener("click", () => setMode("managed"));
controls.manual.addEventListener("click", () => setMode("manual"));
controls.scenario.addEventListener("change", reset);
controls.step.addEventListener("click", () => { stop(); advance(); });
controls.play.addEventListener("click", schedule);
controls.pause.addEventListener("click", stop);
controls.reset.addEventListener("click", reset);
controls.speed.addEventListener("change", () => { if (timer !== null) { stop(); schedule(); } });
render();
