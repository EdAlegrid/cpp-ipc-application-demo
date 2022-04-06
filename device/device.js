'use strict';

const net = require('net');
const { Device } = require('m2m');

const device = new Device(300);

device.connect(() => {

  device.setData('ipc-channel', (data) => {

    let pl = JSON.stringify({type:'random', source:'cpp-server'});

    TcpClient('127.0.0.1', 5300, pl, (err, d) => {
      if(err) return console.error('TcpClient error:', err.message);
      if(d){
        data.send(d);
      }
    });
  });

});

function TcpClient(ip, port, payload, cb){
  const client = new net.Socket();
  client.connect(port, ip);

  client.setEncoding('utf8');

  client.on("connect", () => {
    if(payload){
      client.write(payload);
    }
  });

  client.on('error', (error) => {
    console.log("Tcp client socket error:", error);
    if(error && cb){
      cb(error, null);
    }
    client.destroy();
  });

  client.on("data", (data) => {		
    if(cb){
      setImmediate(cb, null, data);
    }
    client.end();
  });

  client.on("close", (error) => {
    if(error && cb){
      console.log("Tcp client socket is closed:", error);
      cb(error, null);
    }
  });

  client.on("end", (error) => {
    if(error && cb){
      console.log("Tcp client socket connection is terminated:", error);
      cb(error, null);
    }
    client.unref();
  });
};
