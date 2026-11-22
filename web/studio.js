'use strict';
(() => {
  const $ = id => document.getElementById(id);
  const form = $('profile-form'), canvas = $('pet-canvas'), ctx = canvas.getContext('2d');
  const storageKey = 'pebble.studio.profile.v1';
  let profile = PebbleProfile.defaultProfile(), reaction = 'happy', storage = true, noticeTimer;
  const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
  try { const raw = localStorage.getItem(storageKey); if (raw) profile = PebbleProfile.parse(raw); } catch { /* Invalid or unavailable storage must not prevent first use. */ }
  function notice(message) { $('notice').textContent = message; $('notice').classList.add('show'); clearTimeout(noticeTimer); noticeTimer = setTimeout(() => $('notice').classList.remove('show'), 3500); }
  function fromForm() { return PebbleProfile.validate({schemaVersion:1, name:$('pet-name-input').value, theme:form.elements.theme.value, eyes:form.elements.eyes.value, focusMinutes:Number($('focus-minutes').value)}); }
  function fillForm() { $('pet-name-input').value = profile.name; form.elements.theme.value = profile.theme; form.elements.eyes.value = profile.eyes; $('focus-minutes').value = profile.focusMinutes; update(); }
  function update() {
    $('preview-name').textContent = profile.name;
    $('focus-value').textContent = profile.focusMinutes + ' min';
    $('profile-json').textContent = JSON.stringify(profile, null, 2);
    canvas.setAttribute('aria-label', `Live preview of ${profile.name}, a ${profile.theme} pet with ${profile.eyes} eyes`);
    try { localStorage.setItem(storageKey, JSON.stringify(profile)); storage = true; } catch { storage = false; }
    $('save-status').textContent = storage ? 'Saved on this browser' : 'Session only · download to keep';
  }
  form.addEventListener('input', () => { try { profile = fromForm(); $('pet-name-input').setCustomValidity(''); update(); } catch { $('save-status').textContent = 'Check the name before saving'; } });
  function download(blob, name) { const url = URL.createObjectURL(blob), a = document.createElement('a'); a.href = url; a.download = name; document.body.append(a); a.click(); a.remove(); setTimeout(() => URL.revokeObjectURL(url), 2000); }
  form.addEventListener('submit', e => { e.preventDefault(); try { profile = fromForm(); update(); download(new Blob([JSON.stringify(profile, null, 2)], {type:'application/json'}), 'pebble-pet-profile.json'); notice('Pet profile downloaded. See “Your device” to build it into firmware.'); } catch (error) { notice(error.message); } });
  $('import-profile').addEventListener('change', async e => {
    const file = e.target.files[0]; if (!file) return;
    try { if (file.size > 2048) throw new Error('Choose a pet profile smaller than 2 KB.'); profile = PebbleProfile.parse(await file.text()); fillForm(); notice('Welcome back, ' + profile.name + '.'); } catch (error) { notice('Could not import: ' + error.message); } finally { e.target.value = ''; }
  });
  $('reset-profile').addEventListener('click', () => { profile = PebbleProfile.defaultProfile(); reaction = 'happy'; syncReactions(); fillForm(); notice('Back to Milo.'); });
  $('copy-profile').addEventListener('click', async () => { try { profile = fromForm(); update(); await navigator.clipboard.writeText(JSON.stringify(profile)); notice('Profile copied. Paste it into the firmware workflow input.'); } catch { document.querySelector('details').open = true; notice('Select and copy the profile shown below.'); } });
  function syncReactions() { document.querySelectorAll('[data-reaction]').forEach(b => b.setAttribute('aria-pressed', String(b.dataset.reaction === reaction))); $('preview-caption').textContent = reaction === 'love' ? 'Oh. You’re my favorite.' : reaction === 'sleep' ? 'A little nap, a little dream.' : 'A little focus goes a long way.'; }
  document.querySelectorAll('[data-reaction]').forEach(b => b.addEventListener('click', () => { reaction = b.dataset.reaction; syncReactions(); }));
  canvas.addEventListener('click', () => { reaction = 'love'; syncReactions(); });
  function drawFace(g, cx, cy, scale, time, expression = reaction) {
    g.save(); g.translate(cx, cy); g.scale(scale, scale); const color = PebbleProfile.colors[profile.theme];
    const blink = !reduceMotion && time % 5000 > 4750;
    g.translate(0, reduceMotion ? 0 : Math.sin(time / 900) * 3);
    g.fillStyle = color;
    for (const x of [-57, 57]) {
      if (expression === 'love') { g.font = '65px sans-serif'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.fillText('♥', x, -12); }
      else { const asleep = expression === 'sleep' || blink; const h = asleep ? 9 : profile.eyes === 'sleepy' ? 38 : 76; g.beginPath(); g.roundRect(x-26, -h/2-15, 52, h, profile.eyes === 'pixel' ? 4 : Math.min(26,h/2)); g.fill(); }
    }
    g.strokeStyle = color; g.lineWidth = 5; g.beginPath(); g.arc(0,35,14,0,Math.PI); g.stroke();
    g.fillStyle = '#ce9d8060'; for (const x of [-83,83]) { g.beginPath(); g.arc(x,38,8,0,Math.PI*2); g.fill(); }
    if(expression === 'sleep'){g.font='23px sans-serif';g.fillStyle='#88a275';g.fillText('z',85,-75);g.font='16px sans-serif';g.fillText('z',110,-95);}
    g.restore();
  }
  function frame(time) {
    ctx.fillStyle = '#09100b'; ctx.fillRect(0,0,466,466);
    ctx.fillStyle='#9eb18f';ctx.textAlign='center';ctx.textBaseline='middle';ctx.font='14px sans-serif';ctx.fillText('HELLO, I’M '+profile.name.toUpperCase(),233,83);
    drawFace(ctx,233,228,1.2,time);
    ctx.fillStyle='#7f9872';ctx.font='13px sans-serif';ctx.fillText(profile.focusMinutes+' MINUTES, TOGETHER',233,369);
    requestAnimationFrame(frame);
  }
  $('portrait-button').addEventListener('click', () => {
    const c=document.createElement('canvas');c.width=1080;c.height=1350;const g=c.getContext('2d');
    g.fillStyle='#eef4e7';g.fillRect(0,0,1080,1350);g.textAlign='center';g.fillStyle='#385132';g.font='23px sans-serif';g.fillText('A LITTLE LIFE IN MY POCKET',540,140);g.font='78px sans-serif';g.fillText(profile.name,540,260);
    g.fillStyle='#09100b';g.beginPath();g.arc(540,650,300,0,Math.PI*2);g.fill();drawFace(g,540,650,2.3,0);
    g.fillStyle='#385132';g.font='29px sans-serif';g.fillText(profile.focusMinutes+' minutes, together.',540,1080);g.font='23px sans-serif';g.fillText('pebble microOS · pet studio',540,1210);
    c.toBlob(blob=>{if(blob){download(blob,'my-pebble.png');notice('Portrait saved. Say hello to the world.');}else notice('Portrait export failed. Try again in another browser.');},'image/png');
  });
  fillForm();syncReactions();requestAnimationFrame(frame);
})();
