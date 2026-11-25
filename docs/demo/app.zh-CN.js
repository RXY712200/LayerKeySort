// Chinese presentation for the preserved historical demo. Algorithm and Path values follow app.js.
const stateLabels = { completed: "已完成", current: "当前", boundary: "边界", placed: "已放置", queued: "排队" };
const directionLabels = { Zero: "零", Positive: "正向", Negative: "负向" };

const DEFAULT_SHOWCASE = [
  { value: 100, originalIndex: 1, step: 1, position: 1, path: "000", direction: "Zero", depth: 0, relation: "首项" },
  { value: 50, originalIndex: 2, step: 2, position: 1, path: "1A0", direction: "Negative", depth: 1, relation: "位于当前有序区间之前" },
  { value: 150, originalIndex: 3, step: 3, position: 3, path: "0A0", direction: "Positive", depth: 1, relation: "位于当前有序区间之后" },
  { value: 125, originalIndex: 4, step: 4, position: 3, path: "000/A0", direction: "Positive", depth: 1, relation: "位于 100 和 150 之间" },
  { value: 75, originalIndex: 5, step: 5, position: 2, path: "1A0/A0", direction: "Negative", depth: 2, relation: "位于 50 和 100 之间" },
  { value: 25, originalIndex: 6, step: 6, position: 1, path: "1A1", direction: "Negative", depth: 1, relation: "位于当前有序区间之前" },
  { value: 175, originalIndex: 7, step: 7, position: 7, path: "0A0/A0", direction: "Positive", depth: 2, relation: "位于当前有序区间之后" },
  { value: 160, originalIndex: 8, step: 8, position: 7, path: "0A0//A0", direction: "Positive", depth: 2, relation: "位于 150 和 175 之间" },
  { value: 170, originalIndex: 9, step: 9, position: 8, path: "0A0//A1", direction: "Positive", depth: 2, relation: "位于 160 和 175 之间" },
  { value: 165, originalIndex: 10, step: 10, position: 8, path: "0A0//A0///A0", direction: "Positive", depth: 3, relation: "位于 160 和 170 之间" },
  { value: 200, originalIndex: 11, step: 11, position: 11, path: "0A0/A0//A0", direction: "Positive", depth: 3, relation: "位于当前有序区间之后" },
  { value: 0, originalIndex: 12, step: 12, position: 1, path: "1A2", direction: "Negative", depth: 1, relation: "位于当前有序区间之前" },
  { value: 180, originalIndex: 13, step: 13, position: 12, path: "0A0/A0///A0", direction: "Positive", depth: 3, relation: "位于 175 和 200 之间" },
  { value: 110, originalIndex: 14, step: 14, position: 6, path: "000//A0", direction: "Positive", depth: 1, relation: "位于 100 和 125 之间" },
  { value: 145, originalIndex: 15, step: 15, position: 8, path: "000/A1", direction: "Positive", depth: 1, relation: "位于 125 和 150 之间" },
  { value: 155, originalIndex: 16, step: 16, position: 10, path: "0A0///A0", direction: "Positive", depth: 2, relation: "位于 150 和 160 之间" }
];

const COVERAGE_FORMS = [
  { id: "zero", label: "零", test: (item) => item.direction === "Zero" },
  { id: "positive", label: "正向", test: (item) => item.direction === "Positive" },
  { id: "negative", label: "负向", test: (item) => item.direction === "Negative" },
  { id: "same", label: "同层", test: (item, seen) => seen.rootSlots.size > 1 },
  { id: "child", label: "子级 /", test: (item) => item.path.includes("/") && !item.path.includes("//") },
  { id: "skipped", label: "跳级 //", test: (item) => item.path.includes("//") },
  { id: "deeper", label: "更深 ///", test: (item) => item.path.includes("///") }
];
const chart = document.getElementById("chart");
const pathStrip = document.getElementById("path-strip");
const stepStatus = document.getElementById("step-status");
const currentValue = document.getElementById("current-value");
const originalIndex = document.getElementById("original-index");
const locationText = document.getElementById("location");
const assignedPath = document.getElementById("assigned-path");
const directionDepth = document.getElementById("direction-depth");
const placement = document.getElementById("placement");
const pathsChanged = document.getElementById("paths-changed");
const pathPolicy = document.getElementById("path-policy");
const coverageNote = document.getElementById("coverage-note");
const coverageChips = document.getElementById("coverage-chips");
const completion = document.getElementById("completion");
const randomizeButton = document.getElementById("randomize");
const stepButton = document.getElementById("step");
const playButton = document.getElementById("play");
const pauseButton = document.getElementById("pause");
const resetButton = document.getElementById("reset");
const speedControl = document.getElementById("speed");

let currentDataset = cloneItems(DEFAULT_SHOWCASE);
let verifiedShowcase = true;
let inputItems = cloneItems(currentDataset);
let placedItems = [];
let timer = null;
let lastEvent = null;
let selectedOriginalIndex = null;
let encountered = new Set();
let previousRootSlots = { Positive: new Set(), Negative: new Set() };
let newlySeenTimers = [];

function cloneItems(items) { return items.map((item) => ({ ...item })); }
function isComplete() { return inputItems.length === 0; }
function makeElement(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = text;
  return node;
}
function rootSlot(path) {
  const match = path.match(/^[01]?0?0?([A-Z][0-9])/);
  return match ? match[1] : null;
}

function drawBars() {
  const maxValue = Math.max(...currentDataset.map((item) => item.value), 1);
  const boundaryIndexes = new Set();
  if (lastEvent && lastEvent.left) boundaryIndexes.add(placedItems.indexOf(lastEvent.left));
  if (lastEvent && lastEvent.right) boundaryIndexes.add(placedItems.indexOf(lastEvent.right));
  const barData = [
    ...placedItems.map((item, index) => ({
      ...item,
      state: isComplete() ? "completed" : lastEvent && lastEvent.item.originalIndex === item.originalIndex
        ? "current" : boundaryIndexes.has(index) ? "boundary" : "placed"
    })),
    ...inputItems.map((item) => ({ ...item, state: "queued" }))
  ];
  chart.replaceChildren();
  for (const item of barData) {
    const button = makeElement("button", `bar-slot ${item.state}`);
    button.type = "button";
    button.setAttribute("aria-label", item.path
      ? `值 ${item.value}，原输入 ${item.originalIndex}，${stateLabels[item.state]}，Path ${item.path}`
      : `值 ${item.value}，原输入 ${item.originalIndex}，${stateLabels[item.state]}；随机输入无确切 Path`);
    const bar = makeElement("span", "bar");
    bar.style.setProperty("--bar-height", `${Math.max(12, Math.round(item.value / maxValue * 210))}px`);
    bar.setAttribute("aria-hidden", "true");
    const value = makeElement("span", "bar-value", String(item.value));
    const index = makeElement("span", "bar-index", `#${item.originalIndex}`);
    const path = makeElement("span", `bar-path${item.path ? "" : " empty"}`, item.path || "—");
    path.setAttribute("aria-hidden", "true");
    button.append(bar, value, index, path);
    button.addEventListener("click", () => inspectItem(item.originalIndex));
    chart.append(button);
  }
  drawPathStrip();
}

function drawPathStrip() {
  pathStrip.replaceChildren();
  for (const item of placedItems) {
    const li = makeElement("li");
    const button = makeElement("button", `path-entry${item.path ? "" : " unavailable"}`);
    button.type = "button";
    button.title = item.path ? `${item.value}: ${item.path}` : `值 ${item.value}：随机输入无确切 Path`;
    button.setAttribute("aria-label", item.path
      ? `查看值 ${item.value}，Path ${item.path}`
      : `查看值 ${item.value}；无可用 Path`);
    button.append(makeElement("strong", "", String(item.value)));
    button.append(makeElement("code", "", item.path || "无可用 Path"));
    button.addEventListener("click", () => inspectItem(item.originalIndex));
    li.append(button);
    pathStrip.append(li);
  }
}

function updateCoverage(newItem = null) {
  const newlyEncountered = [];
  const markEncountered = (id) => {
    if (!encountered.has(id)) newlyEncountered.push(id);
    encountered.add(id);
  };
  if (newItem && verifiedShowcase) {
    if (newItem.direction === "Zero") markEncountered("zero");
    else markEncountered(newItem.direction.toLowerCase());
    if (newItem.path.includes("/") && !newItem.path.includes("//")) markEncountered("child");
    if (newItem.path.includes("//")) markEncountered("skipped");
    if (newItem.path.includes("///")) markEncountered("deeper");
    const slot = rootSlot(newItem.path);
    if (slot && newItem.direction !== "Zero") {
      previousRootSlots[newItem.direction].add(slot);
      if (previousRootSlots[newItem.direction].size > 1) markEncountered("same");
    }
  }
  coverageChips.replaceChildren();
  for (const form of COVERAGE_FORMS) {
    const li = makeElement("li", `coverage-chip${encountered.has(form.id) ? " seen" : ""}${!verifiedShowcase ? " unavailable" : ""}`, `${form.label}: ${!verifiedShowcase ? "未验证" : encountered.has(form.id) ? "已出现" : "尚未出现"}`);
    li.id = `coverage-${form.id}`;
    coverageChips.append(li);
  }
  coverageNote.textContent = verifiedShowcase
    ? "本次运行放置已验证的相应 Path 形态后才点亮标记。"
    : "随机输入不提供 Path 形态覆盖，不标记任何类别为已出现。";
  if (newItem && verifiedShowcase) {
    for (const id of newlyEncountered) {
      const chip = document.getElementById(`coverage-${id}`);
      if (chip) {
        chip.classList.add("newly-seen");
        const timerId = window.setTimeout(() => chip.classList.remove("newly-seen"), 1500);
        newlySeenTimers.push(timerId);
      }
    }
  }
}

function describeItem(item, position, left, right, isCurrent) {
  currentValue.textContent = String(item.value);
  originalIndex.textContent = `#${item.originalIndex}`;
  locationText.textContent = `位置 ${position}，共 ${placedItems.length} 项` +
    (left && right ? ` · 位于 ${left.value} 和 ${right.value} 之间` : left ? ` · 位于 ${left.value} 之后` : right ? ` · 位于 ${right.value} 之前` : " · 首项");
  assignedPath.textContent = item.path || "未显示：随机 Path 未经生产实现验证";
  directionDepth.textContent = item.path ? `${directionLabels[item.direction]} · 深度 ${item.depth}` : "随机输入无此数据";
  placement.textContent = item.relation || "仅展示放置；未运行生产 Path";
  pathsChanged.textContent = item.path ? "0" : "不报告随机输入";
  stepStatus.textContent = isCurrent
    ? `步骤 ${item.step} 放置原输入 #${item.originalIndex} 的值 ${item.value}。突出显示的边界柱代表相邻项；不模拟 Tree 内部各次比较。显示的确切 Path 为历史 v1 值。`
    : `已选放置项：原输入 #${item.originalIndex}，值 ${item.value}。`;
  pathPolicy.hidden = false;
}

function inspectItem(index) {
  const placedIndex = placedItems.findIndex((item) => item.originalIndex === index);
  const item = placedIndex >= 0 ? placedItems[placedIndex] : inputItems.find((candidate) => candidate.originalIndex === index);
  if (!item) return;
  selectedOriginalIndex = index;
  if (placedIndex >= 0) {
    describeItem(item, placedIndex + 1, placedItems[placedIndex - 1] || null, placedItems[placedIndex + 1] || null, false);
  } else {
    currentValue.textContent = String(item.value);
    originalIndex.textContent = `#${item.originalIndex}`;
    locationText.textContent = "尚未放置";
    assignedPath.textContent = "尚未分配 Path";
    directionDepth.textContent = "—";
    placement.textContent = "按输入顺序等待";
    pathsChanged.textContent = "—";
    stepStatus.textContent = `已选排队项：原输入 #${item.originalIndex}，值 ${item.value}。`;
  }
}

function insertNext() {
  if (isComplete()) { finish(); return false; }
  const item = inputItems.shift();
  let insertIndex;
  if (verifiedShowcase) {
    insertIndex = item.position - 1;
    const leftAtPosition = placedItems[insertIndex - 1] || null;
    const rightAtPosition = placedItems[insertIndex] || null;
    if ((leftAtPosition && leftAtPosition.value >= item.value) || (rightAtPosition && rightAtPosition.value <= item.value)) {
      stepStatus.textContent = "演示一致性错误：采集的 C 位置与输入顺序不匹配。";
      stopPlayback();
      return false;
    }
  } else {
    insertIndex = 0;
    while (insertIndex < placedItems.length && placedItems[insertIndex].value < item.value) insertIndex += 1;
  }
  const left = placedItems[insertIndex - 1] || null;
  const right = placedItems[insertIndex] || null;
  if (!verifiedShowcase) item.relation = left && right ? `展示位置位于 ${left.value} 与 ${right.value} 之间` : left ? `展示位置在 ${left.value} 之后` : right ? `展示位置在 ${right.value} 之前` : "首个展示项";
  placedItems.splice(insertIndex, 0, item);
  lastEvent = { item, left, right };
  selectedOriginalIndex = item.originalIndex;
  drawBars();
  updateCoverage(item);
  describeItem(item, insertIndex + 1, left, right, true);
  if (isComplete()) finish();
  return true;
}

function finish() {
  stopPlayback();
  drawBars();
  const values = placedItems.map((item) => item.value);
  const ascending = values.every((value, index) => index === 0 || values[index - 1] <= value);
  completion.hidden = false;
  completion.textContent = ascending
    ? `完成：${values.join(" · ")}。 ${verifiedShowcase ? "各项均有历史 v1 采集 Path。" : "随机项无已验证的确切 Path。"}`
    : "展示结果未达到升序。";
  if (ascending && lastEvent) {
    const finalPosition = placedItems.indexOf(lastEvent.item);
    describeItem(lastEvent.item, finalPosition + 1, lastEvent.left, lastEvent.right, true);
    stepStatus.textContent = `完成。全部柱已按升序放置。最后插入值：${lastEvent.item.value}，Path：${lastEvent.item.path || "随机输入不显示"}。`;
  } else if (!ascending) {
    stepStatus.textContent = "展示错误：最终值并非升序。";
  }
}

function stopPlayback() {
  if (timer !== null) window.clearTimeout(timer);
  timer = null;
  playButton.disabled = false;
  pauseButton.disabled = true;
}
function scheduleNext() {
  if (timer !== null || isComplete()) return;
  playButton.disabled = true;
  pauseButton.disabled = false;
  timer = window.setTimeout(() => {
    timer = null;
    const advanced = insertNext();
    if (advanced && !isComplete()) scheduleNext();
  }, Number(speedControl.value));
}
function reset() {
  stopPlayback();
  newlySeenTimers.forEach((timerId) => window.clearTimeout(timerId));
  newlySeenTimers = [];
  inputItems = cloneItems(currentDataset);
  placedItems = [];
  lastEvent = null;
  selectedOriginalIndex = null;
  encountered = new Set();
  previousRootSlots = { Positive: new Set(), Negative: new Set() };
  completion.hidden = true;
  currentValue.textContent = "—";
  originalIndex.textContent = "—";
  locationText.textContent = "尚未开始";
  assignedPath.textContent = "尚未分配 Path";
  directionDepth.textContent = "—";
  placement.textContent = "—";
  pathsChanged.textContent = "—";
  stepStatus.textContent = `就绪。${currentDataset.length} 项等待中，单步一次插入一项。`;
  drawBars();
  updateCoverage();
  pathPolicy.hidden = !verifiedShowcase;
}

stepButton.addEventListener("click", () => { stopPlayback(); insertNext(); });
playButton.addEventListener("click", scheduleNext);
pauseButton.addEventListener("click", stopPlayback);
resetButton.addEventListener("click", reset);
randomizeButton.addEventListener("click", () => {
  const values = [];
  while (values.length < DEFAULT_SHOWCASE.length) {
    const value = 10 + Math.floor(Math.random() * 90);
    if (!values.includes(value)) values.push(value);
  }
  const ascending = values.every((value, index) => index === 0 || values[index - 1] < value);
  const descending = values.every((value, index) => index === 0 || values[index - 1] > value);
  if (ascending || descending) values.push(values.shift());
  currentDataset = values.map((value, index) => ({ value, originalIndex: index + 1, verified: false }));
  verifiedShowcase = false;
  reset();
});
speedControl.addEventListener("change", () => {
  if (timer !== null) { stopPlayback(); scheduleNext(); }
});
reset();
