"use strict";

// C 记录快照是数据，不是浏览器 Path 分配器。
const fixtures = window.LKS_V3_FIXTURES;
const manualSteps = [
  { title: "初始手动坐标", detail: "按应用选择的 Path 插入背景、玩家及 HUD。", order: [[101, "背景"], [202, "玩家"], [303, "HUD"]] },
  { title: "在图层间插入效果", detail: "应用调用 lks_path_between()，再调用 lks_tree_insert()。", order: [[101, "背景"], [404, "效果"], [202, "玩家"], [303, "HUD"]] },
  { title: "移动 HUD，保持 ID", detail: "应用选择新坐标并调用 lks_tree_rekey()，HUD 仍为 ID 303。", order: [[101, "背景"], [404, "效果"], [303, "HUD"], [202, "玩家"]] },
  { title: "删除效果", detail: "精确 Path 删除返回借用对象指针，效果对象仍归应用。", order: [[101, "背景"], [303, "HUD"], [202, "玩家"]] },
  { title: "保存一个当前坐标", detail: "公开 C 示例打印 HUD id=303、display=02bR、key=LK1:201FF06D6!，并验证 LK1 往返。", order: [[101, "背景"], [303, "HUD"], [202, "玩家"]] }
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
  if (!match) throw new Error("未知记录场景");
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
  byId("table-caption").textContent = "当前逻辑顺序 · 数字比较器，相等时按插入顺序";
  byId("fixture-note").textContent = `精确 Path 快照来自生产 C 提交 ${fixtures.sourceCommit}。浏览器不分配或比较 Path；V3.0.0 Stable 未改变该生产源码。`;
  if (!current) {
    byId("event-title").textContent = "准备插入";
    byId("event-detail").textContent = `记录序列：${replay.id === "mixed" ? "混合值与稳定等值顺序" : "内部热点与重标记"}。选择单步或播放。`;
    byId("relabel-detail").textContent = "既有 Path 变化数： 0";
    return;
  }
  const insertedId = step - 1;
  const values = replay.steps.slice(0, step).map((entry) => entry.value);
  const sortedIds = values.map((_, id) => id).sort((a, b) => values[a] - values[b] || a - b);
  const changedIds = previous ? sortedIds.filter((id) => id !== insertedId && previous.paths[id] !== current.paths[id]) : [];
  if (changedIds.length !== current.changed || current.paths.length !== step) throw new Error("记录数据一致性失败");
  const rank = sortedIds.indexOf(insertedId) + 1;
  byId("event-title").textContent = `插入对象 #${insertedId + 1} · 值 ${current.value}`;
  byId("event-detail").textContent = `比较器位置 ${rank} / ${step}。 ${current.value === values[sortedIds[rank - 2]] ? "相等值保留插入顺序。" : ""}分配的显示 Path： ${current.paths[insertedId]}.`;
  byId("relabel-detail").textContent = `既有 Path 变化数： ${changedIds.length}${changedIds.length ? ` · 对象 ${changedIds.map((id) => "#" + (id + 1)).join(", ")}` : ""}`;
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
    cell(row, isNew ? "新坐标" : isChanged ? `${previous.paths[id]} → ${current.paths[id]}` : "未变");
    rows.append(row);
  }
}
function renderManual() {
  const rows = byId("rows");
  rows.replaceChildren();
  byId("table-caption").textContent = "应用控制图层顺序 · 公开 layer-list 示例";
  byId("fixture-note").textContent = "顺序与最终 HUD display/LK1 来自 examples/layer_list.c，这里不虚构中间精确 Path。";
  byId("relabel-detail").textContent = "托管重标记不适用；手动树坐标由应用选择。";
  if (step === 0) {
    byId("event-title").textContent = "准备手动插入";
    byId("event-detail").textContent = "应用提供每个 Path。选择单步或播放。";
    return;
  }
  const current = manualSteps[step - 1];
  byId("event-title").textContent = current.title;
  byId("event-detail").textContent = current.detail;
  current.order.forEach(([id, name], position) => {
    const row = document.createElement("tr");
    cell(row, String(position + 1));
    cell(row, `${name} · ID ${id}`);
    cell(row, "调用方定义顺序");
    cell(row, step === 5 && id === 303 ? "02bR" : "应用选择", step === 5 && id === 303);
    cell(row, step === 5 && id === 303 ? "LK1:201FF06D6!" : "见 C 示例");
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
