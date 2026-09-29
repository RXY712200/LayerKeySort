const DEFAULT_SHOWCASE = [
  { value: 100, originalIndex: 1, step: 1, position: 1, path: "000", direction: "Zero", depth: 0, relation: "First item" },
  { value: 50, originalIndex: 2, step: 2, position: 1, path: "1A0", direction: "Negative", depth: 1, relation: "Before the current ordered range" },
  { value: 150, originalIndex: 3, step: 3, position: 3, path: "0A0", direction: "Positive", depth: 1, relation: "After the current ordered range" },
  { value: 125, originalIndex: 4, step: 4, position: 3, path: "000/A0", direction: "Positive", depth: 1, relation: "Between 100 and 150" },
  { value: 75, originalIndex: 5, step: 5, position: 2, path: "1A0/A0", direction: "Negative", depth: 2, relation: "Between 50 and 100" },
  { value: 25, originalIndex: 6, step: 6, position: 1, path: "1A1", direction: "Negative", depth: 1, relation: "Before the current ordered range" },
  { value: 175, originalIndex: 7, step: 7, position: 7, path: "0A0/A0", direction: "Positive", depth: 2, relation: "After the current ordered range" },
  { value: 160, originalIndex: 8, step: 8, position: 7, path: "0A0//A0", direction: "Positive", depth: 2, relation: "Between 150 and 175" },
  { value: 170, originalIndex: 9, step: 9, position: 8, path: "0A0//A1", direction: "Positive", depth: 2, relation: "Between 160 and 175" },
  { value: 165, originalIndex: 10, step: 10, position: 8, path: "0A0//A0///A0", direction: "Positive", depth: 3, relation: "Between 160 and 170" },
  { value: 200, originalIndex: 11, step: 11, position: 11, path: "0A0/A0//A0", direction: "Positive", depth: 3, relation: "After the current ordered range" },
  { value: 0, originalIndex: 12, step: 12, position: 1, path: "1A2", direction: "Negative", depth: 1, relation: "Before the current ordered range" },
  { value: 180, originalIndex: 13, step: 13, position: 12, path: "0A0/A0///A0", direction: "Positive", depth: 3, relation: "Between 175 and 200" },
  { value: 110, originalIndex: 14, step: 14, position: 6, path: "000//A0", direction: "Positive", depth: 1, relation: "Between 100 and 125" },
  { value: 145, originalIndex: 15, step: 15, position: 8, path: "000/A1", direction: "Positive", depth: 1, relation: "Between 125 and 150" },
  { value: 155, originalIndex: 16, step: 16, position: 10, path: "0A0///A0", direction: "Positive", depth: 2, relation: "Between 150 and 160" }
];

const COVERAGE_FORMS = [
  { id: "zero", label: "Zero", test: (item) => item.direction === "Zero" },
  { id: "positive", label: "Positive", test: (item) => item.direction === "Positive" },
  { id: "negative", label: "Negative", test: (item) => item.direction === "Negative" },
  { id: "same", label: "Same level", test: (item, seen) => seen.rootSlots.size > 1 },
  { id: "child", label: "Child /", test: (item) => item.path.includes("/") && !item.path.includes("//") },
  { id: "skipped", label: "Skipped //", test: (item) => item.path.includes("//") },
  { id: "deeper", label: "Deeper ///", test: (item) => item.path.includes("///") }
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
      ? `Value ${item.value}, original input ${item.originalIndex}, ${item.state}, Path ${item.path}`
      : `Value ${item.value}, original input ${item.originalIndex}, ${item.state}; exact Path not available for randomized data`);
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
    button.title = item.path ? `${item.value}: ${item.path}` : `Value ${item.value}: exact Path not available for randomized data`;
    button.setAttribute("aria-label", item.path
      ? `Inspect value ${item.value}, Path ${item.path}`
      : `Inspect value ${item.value}; Path unavailable`);
    button.append(makeElement("strong", "", String(item.value)));
    button.append(makeElement("code", "", item.path || "Path unavailable"));
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
    const li = makeElement("li", `coverage-chip${encountered.has(form.id) ? " seen" : ""}${!verifiedShowcase ? " unavailable" : ""}`, `${form.label}: ${!verifiedShowcase ? "not verified" : encountered.has(form.id) ? "encountered" : "not seen yet"}`);
    li.id = `coverage-${form.id}`;
    coverageChips.append(li);
  }
  coverageNote.textContent = verifiedShowcase
    ? "A chip activates only after this run places a Path with that verified form."
    : "Path-form coverage is unavailable for randomized data; no category is marked as encountered.";
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
  locationText.textContent = `Position ${position} of ${placedItems.length}` +
    (left && right ? ` · between ${left.value} and ${right.value}` : left ? ` · after ${left.value}` : right ? ` · before ${right.value}` : " · first item");
  assignedPath.textContent = item.path || "Not shown: randomized Path was not production-verified";
  directionDepth.textContent = item.path ? `${item.direction} · depth ${item.depth}` : "Not available for randomized data";
  placement.textContent = item.relation || "Display-only placement; no production Path run";
  pathsChanged.textContent = item.path ? "0" : "Not reported for randomized data";
  stepStatus.textContent = isCurrent
    ? `Step ${item.step} placed value ${item.value} from original input #${item.originalIndex}. The highlighted boundary bars show neighboring items; individual Tree comparison probes are not simulated. Exact Paths, when shown, are historical v1 values.`
    : `Selected placed value ${item.value} from original input #${item.originalIndex}.`;
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
    locationText.textContent = "Not placed yet";
    assignedPath.textContent = "No Path assigned yet";
    directionDepth.textContent = "—";
    placement.textContent = "Waiting in input order";
    pathsChanged.textContent = "—";
    stepStatus.textContent = `Selected queued value ${item.value} from original input #${item.originalIndex}.`;
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
      stepStatus.textContent = "Showcase consistency error: recorded C position does not match the input ordering.";
      stopPlayback();
      return false;
    }
  } else {
    insertIndex = 0;
    while (insertIndex < placedItems.length && placedItems[insertIndex].value < item.value) insertIndex += 1;
  }
  const left = placedItems[insertIndex - 1] || null;
  const right = placedItems[insertIndex] || null;
  if (!verifiedShowcase) item.relation = left && right ? `Display position between ${left.value} and ${right.value}` : left ? `Display position after ${left.value}` : right ? `Display position before ${right.value}` : "First displayed item";
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
    ? `Complete: ${values.join(" · ")}. ${verifiedShowcase ? "Each item has its recorded historical v1 Path." : "Randomized items have no verified exact Path values."}`
    : "The display did not reach ascending order.";
  if (ascending && lastEvent) {
    const finalPosition = placedItems.indexOf(lastEvent.item);
    describeItem(lastEvent.item, finalPosition + 1, lastEvent.left, lastEvent.right, true);
    stepStatus.textContent = `Complete. Every bar is placed in ascending order. Last inserted item: ${lastEvent.item.value}, Path ${lastEvent.item.path || "not shown for randomized data"}.`;
  } else if (!ascending) {
    stepStatus.textContent = "Display error: final values are not ascending.";
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
  locationText.textContent = "Not started";
  assignedPath.textContent = "No Path assigned yet";
  directionDepth.textContent = "—";
  placement.textContent = "—";
  pathsChanged.textContent = "—";
  stepStatus.textContent = `Ready. ${currentDataset.length} items are waiting; Step inserts one item.`;
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
