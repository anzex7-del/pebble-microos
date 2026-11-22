'use strict';
const PebbleProfile = (() => {
  const themes = Object.freeze(['moss', 'sky', 'peach', 'lilac']);
  const eyes = Object.freeze(['round', 'sleepy', 'pixel']);
  const colors = Object.freeze({moss:'#bfe6a0', sky:'#a4d6f2', peach:'#fac6b1', lilac:'#d0bbf3'});
  const defaultProfile = () => ({schemaVersion:1, name:'Milo', theme:'moss', eyes:'round', focusMinutes:25});
  function validate(p) {
    if (!p || typeof p !== 'object' || Array.isArray(p) || Object.keys(p).sort().join(',') !== 'eyes,focusMinutes,name,schemaVersion,theme') throw new Error('This is not a Pebble pet profile.');
    if (p.schemaVersion !== 1) throw new Error('This profile uses an unsupported version.');
    if (typeof p.name !== 'string' || !/^[A-Za-z0-9][A-Za-z0-9 _-]{0,15}$/.test(p.name) || p.name !== p.name.trim()) throw new Error('Use 1–16 English letters or numbers, with spaces, hyphens or underscores inside the name.');
    if (!themes.includes(p.theme) || !eyes.includes(p.eyes)) throw new Error('Unknown pet color or eye style.');
    if (!Number.isInteger(p.focusMinutes) || p.focusMinutes < 5 || p.focusMinutes > 60) throw new Error('Choose a focus session between 5 and 60 minutes.');
    return {schemaVersion:1, name:p.name, theme:p.theme, eyes:p.eyes, focusMinutes:p.focusMinutes};
  }
  function parse(raw) {
    if (typeof raw !== 'string' || new TextEncoder().encode(raw).length > 2048) throw new Error('Pet profiles must be smaller than 2 KB.');
    return validate(JSON.parse(raw));
  }
  return {themes, eyes, colors, defaultProfile, validate, parse};
})();
if (typeof module !== 'undefined' && module.exports) module.exports = PebbleProfile;
