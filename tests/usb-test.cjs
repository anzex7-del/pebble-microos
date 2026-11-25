'use strict';
const assert = require('node:assert/strict');
const createClient = require('../web/usb.js');

module.exports = async function checkUsb() {
  let input, respond = true, closed = 0;
  const writes = [];
  const port = {
    async open(options) {
      assert.deepEqual(options, {baudRate:115200, flowControl:'none'});
      this.readable = new ReadableStream({start(controller) { input = controller; }});
      this.writable = new WritableStream({write(bytes) {
        const cmd = new TextDecoder().decode(bytes).trimEnd();
        writes.push(cmd);
        if (writes.length === 1) return; // Boot discarded the initial probe.
        const ack = cmd === 'PEBBLE HELLO 1' ? 'PEBBLE READY 1' :
          respond ? cmd.replace('PEBBLE SET ', 'PEBBLE OK ') : 'PEBBLE ERR';
        for (const part of [ack.slice(0,4), ack.slice(4)+'\n']) input.enqueue(new TextEncoder().encode(part));
      }});
    },
    async close() { assert(!this.readable.locked && !this.writable.locked); closed++; }
  };
  const client = createClient({requestPort: async () => port});
  await client.connect();
  assert(client.connected());
  const progress = [];
  await client.send(['NMilo','C2'], n => progress.push(n));
  assert.deepEqual(progress,[1,2]);
  assert.deepEqual(writes, ['PEBBLE HELLO 1','PEBBLE HELLO 1','PEBBLE HELLO 1','PEBBLE SET NMilo','PEBBLE SET C2']);
  respond = false;
  await assert.rejects(client.send(['E2'], () => {}), /rejected/);
  assert(!client.connected() && closed === 1);
  await client.connect();
  input.close();
  await new Promise(resolve => setTimeout(resolve, 0));
  assert(!client.connected() && closed === 2);
  console.log('PASS: USB identification, fragmented replies, applied settings, rejection and unplug');
};
