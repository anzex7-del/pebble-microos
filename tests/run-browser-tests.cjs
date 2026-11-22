const fs = require('node:fs');
const vm = require('node:vm');
const context = vm.createContext({});
for (const path of ['tests/browser_mock.js', 'preview/app.js', 'tests/browser_test.js']) {
  vm.runInContext(fs.readFileSync(path, 'utf8'), context, {filename:path, timeout:10000});
}
console.log('PASS: browser prototype logic (mock DOM, not visual QA)');
