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
  form.addEventListener('submit', e => { e.preventDefault(); try { profile = fromForm(); update(); download(new Blob([JSON.stringify(profile, null, 2)], {type:'application/json'}), 'pebble-pet-profile.json'); notice('Profile downloaded. Send it over Bluetooth, or keep it for a custom build.'); } catch (error) { notice(error.message); } });
  $('import-profile').addEventListener('change', async e => {
    const file = e.target.files[0]; if (!file) return;
    try { if (file.size > 2048) throw new Error('Choose a pet profile smaller than 2 KB.'); profile = PebbleProfile.parse(await file.text()); fillForm(); notice('Welcome back, ' + profile.name + '.'); } catch (error) { notice('Could not import: ' + error.message); } finally { e.target.value = ''; }
  });
  $('reset-profile').addEventListener('click', () => { profile = PebbleProfile.defaultProfile(); reaction = 'happy'; syncReactions(); fillForm(); notice('Back to Milo.'); });
  $('copy-profile').addEventListener('click', async () => { try { profile = fromForm(); update(); await navigator.clipboard.writeText(JSON.stringify(profile)); notice('Profile copied. Paste it into the firmware workflow input.'); } catch { document.querySelector('details').open = true; notice('Select and copy the profile shown below.'); } });
  function syncReactions() { document.querySelectorAll('[data-reaction]').forEach(b => b.setAttribute('aria-pressed', String(b.dataset.reaction === reaction))); $('preview-caption').textContent = reaction === 'love' ? 'Oh. You’re my favorite.' : reaction === 'sleep' ? 'A little nap, a little dream.' : 'A little focus goes a long way.'; }
  document.querySelectorAll('[data-reaction]').forEach(b => b.addEventListener('click', () => { reaction = b.dataset.reaction; syncReactions(); }));
  canvas.addEventListener('click', () => { reaction = 'love'; syncReactions(); });
  function drawFace(g,cx,cy,scale,time,expression=reaction) {
    const color=PebbleProfile.colors[profile.theme], eyes=profile.eyes, reducedMotion=reduceMotion;
    g.save(); g.translate(cx, cy); g.scale(scale, scale);
    const bob = reducedMotion ? 0 : Math.sin(time / 850) * 2.5;
    const asleep = expression === 'sleep', loved = expression === 'love';
    const blink = !reducedMotion && time % 4600 > 4450;
    g.translate(0, bob + (loved && !reducedMotion ? Math.sin(time / 140) * 2 : 0));
    function pill(x,y,w,h,r,fill) { g.fillStyle=fill; g.beginPath(); g.roundRect(x,y,w,h,r); g.fill(); }
    // Soft ears, tiny paws and a cream belly, matching the native pet.
    pill(-74,-81,37,65,19,color); pill(37,-81,37,65,19,color);
    pill(-65,-71,19,39,10,'#f6d7cc'); pill(46,-71,19,39,10,'#f6d7cc');
    pill(-65,51,42,23,13,color); pill(23,51,42,23,13,color);
    pill(-101,-11,38,45,20,color); pill(63,-11,38,45,20,color);
    pill(-82,-57,164,118,57,color); pill(-46,14,92,42,22,'#f6f1de');
    const height=asleep||blink?5:eyes==='sleepy'?15:29;
    for(const x of [-31,31]) {
      if(loved && !asleep) {
        g.save(); g.translate(x,-13); g.scale(.56,.56); g.fillStyle='#23332b';
        g.beginPath(); g.moveTo(0,20); g.bezierCurveTo(-44,-5,-18,-33,0,-12); g.bezierCurveTo(18,-33,44,-5,0,20); g.fill(); g.restore();
      } else {
        pill(x-11,-13-height/2,22,height,eyes==='pixel'?4:Math.min(11,height/2),'#23332b');
        if(!asleep&&!blink&&eyes!=='sleepy') pill(x-7,-23,6,8,3,'#fff');
      }
    }
    pill(-62,7,23,12,6,loved?'#ff8f9e':'#f1a99f'); pill(39,7,23,12,6,loved?'#ff8f9e':'#f1a99f');
    g.strokeStyle='#23332b'; g.lineWidth=2.5; g.lineCap='round'; g.beginPath(); g.moveTo(-8,10); g.quadraticCurveTo(0,22,8,10); g.stroke();
    if(expression==='yum') { pill(-5,10,10,8,5,'#23332b'); }
    if(asleep) { g.fillStyle='#d0d7b6';g.textAlign='center';g.font='17px sans-serif';g.fillText('z',84,-57);g.font='12px sans-serif';g.fillText('z',103,-75); }
    if(loved&&!reducedMotion) { g.fillStyle='#f8b7b2';g.textAlign='center';g.font='16px sans-serif';g.fillText('♥',94,-40-Math.sin(time/300)*5); }
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
  const linkStatus=$('link-status'), connectButton=$('connect-pet');
  const deviceButtons=[$('send-pet'),$('sync-clock'),$('pat-device'),$('disconnect-pet')];
  let transferring=false;
  const companion=PebbleCompanion.createClient(navigator.bluetooth,()=>{linkStatus.textContent='Disconnected. Enable Bluetooth on your Pebble to reconnect.'; updateLink();});
  const usbButton = $('connect-usb');
  const usb = createPebbleUsb(navigator.serial, () => { linkStatus.textContent='USB disconnected.'; updateLink(); });
  function updateLink() {
    const bluetooth = companion.connected(), online = bluetooth || usb.connected();
    deviceButtons.forEach(b => b.disabled = !online || transferring);
    $('sync-clock').disabled = $('pat-device').disabled = !bluetooth || transferring;
    connectButton.disabled = transferring || online || !navigator.bluetooth;
    usbButton.disabled = transferring || online || !navigator.serial;
  }
  async function transfer(action,success) {
    transferring=true; updateLink();
    try { await action(); linkStatus.textContent=success; notice(success); }
    catch(error) { linkStatus.textContent=error.message; }
    finally { transferring=false; updateLink(); }
  }
  connectButton.addEventListener('click',()=>transfer(async()=>{linkStatus.textContent='Choose Pebble in the Bluetooth dialog…'; await companion.connect();},'Connected to Pebble. Choose what to send.'));
  usbButton.addEventListener('click', () => transfer(async () => {
    linkStatus.textContent = 'Choose the USB download port (A)…';
    await usb.connect();
  }, 'Connected to Pebble over USB. Choose Send this design.'));
  $('send-pet').addEventListener('click',()=>transfer(async()=>{profile=fromForm();update();await (usb.connected() ? usb : companion).send(PebbleCompanion.profileCommands(profile),(done,total)=>{linkStatus.textContent=`Sending your design: ${done}/${total} confirmed…`;});},'Design applied on Pebble. Wait two seconds before switching it off.'));
  $('sync-clock').addEventListener('click',()=>transfer(()=>companion.send(PebbleCompanion.clockCommands()),'Clock and timezone synced from this computer.'));
  $('pat-device').addEventListener('click',()=>transfer(()=>companion.send(['P']),'A little pat, delivered.'));
  $('disconnect-pet').addEventListener('click',async()=>{transferring=true; updateLink(); try { if(usb.connected()) await usb.disconnect(); else companion.disconnect(); linkStatus.textContent='Disconnected.'; } finally {transferring=false; updateLink();}});
  if(!navigator.bluetooth && !navigator.serial) linkStatus.textContent='Open Pet Studio in desktop Chrome or Edge to connect.';
  updateLink();
  fillForm();syncReactions();requestAnimationFrame(frame);
})();
