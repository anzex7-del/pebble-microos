'use strict';
const PebbleCompanion = (() => {
  const serviceId = 0xfd50;
  const writeId = '00000001-0000-1001-8001-00805f9b07d0';
  const readId = '00000003-0000-1001-8001-00805f9b07d0';
  function profileCommands(input) {
    const p = PebbleProfile.validate(input);
    return ['N' + p.name, 'C' + PebbleProfile.themes.indexOf(p.theme), 'E' + PebbleProfile.eyes.indexOf(p.eyes), 'F' + p.focusMinutes];
  }
  function clockCommands(date = new Date()) {
    const epoch = Math.floor(date.getTime() / 1000), offset = -date.getTimezoneOffset();
    if (!Number.isInteger(epoch) || epoch < 1704067200 || epoch > 4102444799 || offset < -720 || offset > 840 || offset % 15) throw new Error('Check the time and timezone on this computer.');
    return ['Z' + offset, 'T' + epoch];
  }
  function createClient(bluetooth, onDisconnect = () => {}, options = {}) {
    let device, writer, reader, busy = false;
    const delay = options.delay || (ms => new Promise(resolve => setTimeout(resolve, ms)));
    const encoder = new TextEncoder(), decoder = new TextDecoder();
    const connected = () => Boolean(device?.gatt?.connected && writer && reader);
    function clear() { writer = reader = null; onDisconnect(); }
    async function acknowledge(expected) {
      for (let i = 0; i < 40; i++) {
        if (!connected()) throw new Error('Pebble disconnected. Enable its Bluetooth window and reconnect.');
        const value = decoder.decode(await reader.readValue()).replace(/\0+$/, '');
        if (value === expected) return;
        if (value === 'ERR!') throw new Error('Pebble rejected the setting. Check the profile and retry.');
        await delay(100);
      }
      throw new Error('Pebble did not confirm the change. Stop any Wi-Fi scan, then reconnect and retry.');
    }
    async function connect() {
      if (busy) throw new Error('Wait for the current transfer to finish.');
      if (!bluetooth?.requestDevice) throw new Error('Use Chrome or Edge with Web Bluetooth. Safari and this embedded preview cannot connect.');
      busy = true;
      try {
        disconnect();
        device = await bluetooth.requestDevice({filters:[{namePrefix:'Pebble', services:[serviceId]}]});
        device.addEventListener('gattserverdisconnected', clear);
        const server = await device.gatt.connect();
        const service = await server.getPrimaryService(serviceId);
        writer = await service.getCharacteristic(writeId);
        reader = await service.getCharacteristic(readId);
        await acknowledge('OK:V');
        return device.name || 'Pebble';
      } catch (error) { disconnect(); throw error; }
      finally { busy = false; }
    }
    async function send(commands, onProgress = () => {}) {
      if (busy) throw new Error('Wait for the current transfer to finish.');
      if (!connected()) throw new Error('Connect to your Pebble first.');
      // Validate the entire batch before any write.
      const encoded = commands.map(command => {
        if (typeof command !== 'string' || !/^(P|N[A-Za-z0-9][A-Za-z0-9 _-]{0,15}|[CEFTZ]-?\d+)$/.test(command)) throw new Error('Invalid companion command.');
        const data = encoder.encode(command);
        if (data.length > 20) throw new Error('Companion command is too long.');
        return data;
      });
      busy = true;
      let applied = 0;
      try {
        for (let i = 0; i < commands.length; i++) {
          if (!connected()) throw new Error('Pebble disconnected.');
          if (writer.properties.write && writer.writeValueWithResponse) await writer.writeValueWithResponse(encoded[i]);
          else await writer.writeValueWithoutResponse(encoded[i]);
          await acknowledge('OK:' + commands[i]);
          applied++; onProgress(applied, commands.length);
        }
      } catch (error) {
        throw new Error(`${error.message} ${applied} of ${commands.length} settings confirmed; reconnect and send again.`);
      } finally { busy = false; }
    }
    function disconnect() {
      if (device) { device.removeEventListener('gattserverdisconnected', clear); if (device.gatt?.connected) device.gatt.disconnect(); }
      device = writer = reader = null;
    }
    return {connect, disconnect, connected, send};
  }
  return {profileCommands, clockCommands, createClient, serviceId, writeId, readId};
})();
if (typeof module !== 'undefined' && module.exports) module.exports = PebbleCompanion;
