const assert = require('node:assert/strict');
const profile = require('../web/profile.js');
assert.deepEqual(profile.parse(JSON.stringify(profile.defaultProfile())), profile.defaultProfile());
for (const theme of profile.themes) for (const eyes of profile.eyes) assert.equal(profile.validate({...profile.defaultProfile(),theme,eyes}).eyes,eyes);
for (const patch of [{name:'"\n#error x'},{name:'a'.repeat(17)},{name:'x '},{name:'<img>'},{name:''},{focusMinutes:true},{focusMinutes:61},{focusMinutes:1},{schemaVersion:2},{extra:1}]) assert.throws(()=>profile.validate({...profile.defaultProfile(),...patch}));
assert.throws(()=>profile.parse(' '.repeat(2049)));
assert.throws(()=>profile.parse('not json'));
console.log('PASS: web profile validation, supported combinations, hostile input rejection');
