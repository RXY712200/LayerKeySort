const fs = require('fs');
const vm = require('vm');
const root = __dirname + '/';
const elements = new Map();
let tasks = new Map();
let nextTimer = 1;
function element(id = '') {
  const classes = new Set();
  return {
    id, value: '', disabled: false, hidden: false, textContent: '', className: '', children: [], attrs: {}, listeners: {},
    classList: { toggle(name, on) { if (on) classes.add(name); else classes.delete(name); }, has(name) { return classes.has(name); } },
    replaceChildren(...children) { this.children = children; },
    append(...children) { this.children.push(...children); },
    setAttribute(name, value) { this.attrs[name] = value; },
    addEventListener(name, handler) { this.listeners[name] = handler; },
    click() { if (!this.disabled) this.listeners.click(); },
    change() { this.listeners.change(); }
  };
}
const ids = ['managed-mode','manual-mode','scenario','step','play','pause','reset','speed','managed-intro','manual-intro','progress','event-title','event-detail','relabel-detail','rows','table-caption','fixture-note'];
for (const id of ids) elements.set(id, element(id));
elements.get('scenario').value = 'mixed';
elements.get('speed').value = '600';
const context = {
  window: {},
  document: { getElementById(id) { if (!elements.has(id)) throw new Error('Missing DOM node ' + id); return elements.get(id); }, createElement() { return element(); } },
  setTimeout(callback) { const id = nextTimer++; tasks.set(id, callback); return id; },
  clearTimeout(id) { tasks.delete(id); }
};
vm.runInNewContext(fs.readFileSync(root+'fixtures.js','utf8'), context, {filename:'fixtures.js'});
vm.runInNewContext(fs.readFileSync(root+'app.js','utf8'), context, {filename:'app.js'});
const get = (id) => elements.get(id);
function assert(ok, message) { if (!ok) throw new Error(message); }
function flush() { const pair = tasks.entries().next().value; if (!pair) return false; tasks.delete(pair[0]); pair[1](); return true; }
assert(get('progress').textContent === '0 / 10', 'initial');
get('step').click();
assert(get('progress').textContent === '1 / 10' && get('rows').children.length === 1, 'mixed step');
get('reset').click();
assert(get('progress').textContent === '0 / 10', 'reset');
get('scenario').value = 'hotspot'; get('scenario').change();
for (let i=0;i<23;i++) get('step').click();
assert(get('progress').textContent === '23 / 25', 'hotspot progress');
assert(get('rows').children.length === 23, 'hotspot row count');
assert(get('relabel-detail').textContent.includes('Existing Paths changed: 8'), 'relabel count');
assert(get('rows').children.filter(row => row.className === 'changed').length === 8, 'relabel highlights');
get('manual-mode').click();
assert(get('progress').textContent === '0 / 5', 'manual reset');
get('play').click();
for (let i=0;i<10 && flush();i++) {}
assert(get('progress').textContent === '5 / 5', 'manual play');
assert(get('rows').children.length === 3 && get('event-detail').textContent.includes('LK1:201FF06D6!'), 'manual output');
get('managed-mode').click(); get('scenario').value='mixed'; get('scenario').change();
get('play').click(); flush(); get('pause').click();
assert(get('progress').textContent === '1 / 10' && tasks.size === 0, 'pause');
get('play').click();
for (let i=0;i<20 && flush();i++) {}
assert(get('progress').textContent === '10 / 10' && get('rows').children.length === 10, 'managed play');
console.log('V3 visualizer interaction checks PASS: step, relabel, reset, mode switch, play, pause');
