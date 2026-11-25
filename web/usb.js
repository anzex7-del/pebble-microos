'use strict';
function createPebbleUsb(serial, onDisconnect = () => {}) {
  let session;
  const encoder = new TextEncoder();
  function close(s) {
    if (!s) return;
    if (s.closing) return s.closing;
    s.ready = false;
    s.closing = Promise.resolve().then(async () => {
      s.pending?.reject(new Error('USB disconnected.'));
      try { await s.reader?.cancel(); } catch {}
      try { await s.readLoop; } catch {}
      try { await s.writer?.abort(); } catch {}
      try { s.reader?.releaseLock(); } catch {}
      try { s.writer?.releaseLock(); } catch {}
      try { await s.port.close(); } catch {}
      if (session === s) session = undefined;
      onDisconnect();
    });
    return s.closing;
  }
  async function read(s) {
    let line = '', discard = false;
    try {
      while (!s.closing) {
        const {value, done} = await s.reader.read();
        if (done) break;
        for (const byte of value) {
          if (byte === 10) {
            if (!discard && s.pending) {
              if (line === s.pending.expected) s.pending.resolve();
              else if (line === 'PEBBLE ERR') s.pending.reject(new Error('Pebble rejected the setting.'));
            }
            line = ''; discard = false;
          } else if (byte < 32 || byte > 126 || line.length >= 64) discard = true;
          else if (!discard) line += String.fromCharCode(byte);
        }
      }
    } finally {
      if (!s.closing) {
        s.pending?.reject(new Error('Pebble was unplugged.'));
        queueMicrotask(() => close(s));
      }
    }
  }
  async function exchange(s, line, expected, timeout) {
    if (!s || s.closing || s.pending) throw new Error('USB connection unavailable or busy.');
    let timer;
    const reply = new Promise((resolve, reject) => {
      s.pending = {expected, resolve, reject};
      timer = setTimeout(() => reject(new DOMException('No reply from Pebble. Select download port A and close other serial tools.', 'TimeoutError')), timeout);
    });
    try {
      await Promise.all([s.writer.write(encoder.encode(line + '\n')), reply]);
    } finally { clearTimeout(timer); s.pending = undefined; }
  }
  async function connect() {
    if (!serial || session) throw new Error('Use desktop Chrome or Edge with a USB data cable.');
    const port = await serial.requestPort({filters:[{usbVendorId:0x1a86}]});
    await port.open({baudRate:115200, flowControl:'none'});
    const s = {port};
    try {
      s.reader = port.readable.getReader();
      s.writer = port.writable.getWriter();
      session = s;
      s.readLoop = read(s).catch(() => {});
      // Some bridge drivers reset the board on open. Probe the same open
      // port while it boots. Settings are never automatically retried.
      for (let attempt = 1; ; ++attempt) {
        try { await exchange(s, 'PEBBLE HELLO 1', 'PEBBLE READY 1', 1200); break; }
        catch (error) { if (error.name !== 'TimeoutError' || attempt >= 6 || s.closing) throw error; }
      }
      s.ready = true;
    } catch (error) { await close(s); throw error; }
  }
  async function send(commands, progress) {
    const s = session;
    if (!s?.ready) throw new Error('Connect Pebble over USB first.');
    try {
      // Renew the firmware handshake when the user returns to the editor.
      await exchange(s, 'PEBBLE HELLO 1', 'PEBBLE READY 1', 3000);
      for (let i = 0; i < commands.length; ++i) {
        const command = commands[i];
        await exchange(s, 'PEBBLE SET ' + command, 'PEBBLE OK ' + command, 4000);
        progress(i + 1, commands.length);
      }
    } catch (error) { await close(s); throw error; }
  }
  return {connect, send, connected:() => Boolean(session?.ready), disconnect:() => close(session)};
}
if (typeof module !== 'undefined' && module.exports) module.exports = createPebbleUsb;
