// this file should create windows and handl all system events
/*  try electron-reload to monitor changes:
    https://www.npmjs.com/package/electron-reload
*/
// use external config:
// const { host, port } = require('./config');    // config.json
// console.log(`Server will run at http://${host}:${port}`);
// >> "Server will run at http://localhost:8080"
// OR JSON.parse: https://stackoverflow.com/questions/5726729/how-to-parse-json-using-node-js
const {app, BrowserWindow, ipcMain} = require('electron')
const fs = require('fs')
let config = JSON.parse( fs.readFileSync('config.json') )
console.log("[INFO] config:")
console.log(config)
const WebSocket = require('ws')
const wss = new WebSocket.Server({ port: config.websocket_port })
const express = require('express')
const fw = express()
const ip = require('ip')
const dgram = require('dgram')
const OSC = require('osc-js')
const socket = dgram.createSocket('udp4')

socket.on('listening', function(){ socket.setBroadcast(true) })
socket.bind(config.osc_port)


const MSG_TYPE_SET_ADDRESS      = 0
const MSG_TYPE_SET_MODE         = 1
const MSG_TYPE_CHECK_FIRMWARE   = 2
const MSG_TYPE_CONFIG           = 3
const MSG_TYPE_REBOOT           = 4
const MSG_TYPE_SCAN             = 5
const MSG_TYPE_REQUEST_ADDRESS  = 6
const MSG_TYPE_CONNECT_INFO     = 7
const MSG_TYPE_BATTERY          = 8

let win = null            // main windows
let mon = null            // monitor window
let addressCounter = 0


/* -------- ELECTRON-RELOAD (monitor file changes and restart): ---------- */
  const path = require('path')
    require('electron-reload')(__dirname,{
    electron: path.join(__dirname, 'node_modules','.bin', 'electron')
  })


/* --------- ELECTRON --------- */
  function createWindow() {
    // render process:
    win = new BrowserWindow({width: 600, height: 600, x: 50, y: 100, show: false})
    win.loadFile('index.html')
    win.setMenu(null) //Hide window menu

    win.once('ready-to-show', () => {
      win.send('firmware-version', config.firmware_version)
      win.show()
    })

    win.on('closed', () => {
        win = null
    })

    sendServerIP()    // broadcast OSC message w/ server IP to all devices
  }

  function openDeviceMonitor() {
    if (!mon) {
      console.log("[main] opening new monitor window")
      mon = new BrowserWindow({ width: 460, height: 600, show: false })
      mon.loadFile('monitor.html')
      //let _pos = mon.getPosition()
      mon.setPosition( win.getPosition()[0] + win.getSize()[0] + 25, win.getPosition()[1] )

      //updateMonitor(addr, batt, fw)
      mon.once('ready-to-show', () => {
        wss.clients.forEach((_ws) => {
          updateMonitor(_ws.isAlive, _ws.address, _ws.battery, _ws.firmware)
        })
        remoteStatusConsole('devices-connected', wss.clients.size)
        mon.show()
      })
      mon.on('close', () => {
        mon = null
      })
    }
    else {
      mon.setPosition( win.getPosition()[0] + win.getSize()[0] + 25, win.getPosition()[1] )
      mon.focus()
    }
  }
  app.on('ready', createWindow)
  // app.on('window-all-closed', () => {
  //   console.log("all-closed")
  //   shutdownApp()
  // })
  // app.on('will-quit', () => {
  //   console.log("quit")
  //   shutdownApp()
  // })
  //
  // function shutdownApp() {
  //   // wss.clients.forEach((client) => {
  //   //   client.close()
  //   // }
  //   // First sweep, soft close
  //   wss.clients.forEach((_ws) => {
  //     _ws.close()
  //   })
  //   setTimeout(() => {
  //     // Second sweep, hard close
  //     // for everyone who's left
  //     wss.clients.forEach((_ws) => {
  //       if ([_ws.OPEN, _ws.CLOSING].includes(_ws.readyState)) {
  //         _ws.terminate()
  //       }
  //     })
  //     console.log("dead")
  //     app.quit()
  //   }, 2000)
  // }



/* ------- WEBSOCKETS: -------- */
  //https://www.npmjs.com/package/ws
  //https://www.npmjs.com/package/ws#how-to-detect-and-close-broken-connections
  // websocket terminal client:
  // > npm install -g wscat
  // > wscat -c <serverIP>:8080

  // BACKUP PLAN for client tracking: https://medium.com/@willrigsbee/how-to-keep-track-of-clients-with-websockets-1a018c23bbfc
  wss.clients.clear()
  wss.on('connection', (_ws, req) => {
    let remoteIP = req.connection.remoteAddress
    _ws.remoteIP = remoteIP
    let counter = 0
    let double = false
    wss.clients.forEach((client) => {
        if (client.remoteIP == _ws.remoteIP) counter++
        if (counter > 1) {                                                // if the device already exists
          if (client.address == undefined || client.address < 0) {        // and the previous one did not configure properly
            client.terminate()                                            // terminate the connection immediately (vs. client.close())
            wss.clients.delete(client)                                    // delete it from the set
          }
          else {
            console.log("[wss] repeat connection, ", _ws.address)
          }
          double = true
        }
    })
    if (double) { // do not finish setup if it's a double device!
      return 0
    }
    console.log("[wss] %s connected. %s", remoteIP, _ws.readyState)

    //prevClientsSize = wss.clients.size
    remoteStatusConsole('devices-connected', wss.clients.size)    // notify GUI if # clients changes

    _ws.isAlive = true
    _ws.address = -1
    _ws.battery = 0
    _ws.firmware = 0
    _ws.on('pong', heartbeatWS)
    _ws.on('message', onMessageWS)
    _ws.on('error', (err) => console.log('[_ws] error: ' + err))
    //_ws.on( 'close', (errCode, errReason) =>) {
    _ws.on( 'close', (errCode, errReason) => {
        this.isAlive = false
        //this.terminate()    // TODO close and terminate throw exceptions here
        console.log( "Device disconnected, id: " + this.address + " reason: " + errReason )
    })
  })

  let printDeviceNumber = false
  let prevClientsSize = 0
  function heartbeatWS() {
      //console.log ("[wss] received pong on " + this.address + ", state: " + this.readyState)
      this.isAlive = true     // 'this' refers to the device that sent the pong
      //remoteStatusConsole('devices-connected', wss.clients.size)
      if (wss.clients.size != prevClientsSize) {
      //if (printDeviceNumber) {
        remoteStatusConsole('devices-connected', wss.clients.size)    // notify GUI if # clients changes
        printDeviceNumber = false
      }
      prevClientsSize = wss.clients.size
  }

  function pingWS() {
    wss.clients.forEach((_ws) => {
      if(_ws.isAlive === false) {
        //return _ws.terminate()
        return
      }
      _ws.isAlive = false
      if (_ws.readyState === WebSocket.OPEN) {
        _ws.ping( '', false, true )   // alt: _ws.ping(()=>{})  //pass noop function
      }
    })
    printDeviceNumber = true
  }
  setInterval(() => { pingWS() }, config.ping_interval)

  function onMessageWS(_msg) {
    let msg = 0
    if (_msg) {
      try {
        msg = JSON.parse(_msg)            // check for a valid JSON payload
      }
      catch(e) {
        // this might need some cleanup still? -
        console.log("[wss] JSON parse error: ", e)
        remoteStatusConsole('print-message', 'JSON error')
        return
      }

      if (msg.type != undefined) {                  // check for "message type" key (should always have one)
      //this.isAlive = true
        switch (msg.type) {

          case MSG_TYPE_CONNECT_INFO:
            if (msg.data != undefined) {
              this.address = msg.data.address
              this.firmware = msg.data.firmwareVersion
              updateMonitor(this.isAlive, this.address, this.battery, this.firmware)
              console.log("[wss] device %s connected. FW:%s", msg.data.address, msg.data.firmwareVersion)
            }
            break

          case MSG_TYPE_REQUEST_ADDRESS:
            if (msg.data != undefined) {
              console.log("[wss] address requested from: %s", msg.data)
              let response = {
                type: MSG_TYPE_SET_ADDRESS,
                data: addressCounter
              }
              let addr = (addressCounter < 10) ? (addressCounter+ " ") : addressCounter    // align MACs for debug
              console.log("[wss] address: %s    sent to: %s", addr, msg.data)
              this.address = addressCounter     // 'this' is _ws ... aka device that sent the message
              addressCounter++
              if (wss.clients.size == addressCounter) {
                console.log("[wss] addressing complete (%s devices)", addressCounter)
                remoteStatusConsole('print-message', "addressing " + addressCounter + " device(s) complete")
                // update monitor after scanning addresses:
                wss.clients.forEach((_ws) => {
                  updateMonitor(_ws.isAlive, _ws.address, _ws.battery, _ws.firmware)
                })
              }
              this.send(JSON.stringify(response))
            }
            break

          case MSG_TYPE_BATTERY:
            if (msg.data != undefined) {
              this.battery = msg.data
              updateMonitor(this.isAlive, this.address, this.battery, this.firmware)
            }
            break

          default:
            console.log("[wss] message type: %s", msg.type)
            break
        }
      }
      else {
        console.log("[wss] invalid message: no message type")
      }
    }
  }

  function broadcastWSS(msg) {
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(msg)
        }
    })
  }



/* -------- MESSAGING (main <--> render processes) -------- */
  // ipcMain.on('channel', (event, arg) => {}
  ipcMain.on('checkFirmware', (event) => {
      let msg = {
        type: MSG_TYPE_CHECK_FIRMWARE,
        data: { version: config.firmware_version,
                url: "http://" + ip.address() + ":" + config.firmware_server_port + "/",
                filename: config.firmware_filename
              }
      }
      broadcastWSS(JSON.stringify( msg ))
      console.log("[ipc] check Firmware")
  })

  ipcMain.on('scanDevices', (event) => {
    // [ADDR] clear address array
    addressCounter = 0
    let msg = {
        type: MSG_TYPE_SCAN
    }
    broadcastWSS(JSON.stringify(msg))
    console.log("[ipc] scanning %s devices", wss.clients.size)
  })

  ipcMain.on('restartDevices', (event) => {
    if (config.reset_broadcast) {
      broadcastRestartOSC()
      console.log("[ipc] rebooting ALL devices")
    } else {
      let msg = {
          type: MSG_TYPE_REBOOT
      }
      broadcastWSS(JSON.stringify(msg))
      console.log("[ipc] rebooting %s devices", wss.clients.size)
    }
  })

  ipcMain.on('configDevices', (event) => {
    let msg = {
        type: MSG_TYPE_CONFIG,
        data: { startmode: config.device_startMode,
                ssid: 0,
                password: 0
              }
    }
    broadcastWSS(JSON.stringify(msg))
    console.log("[ipc] sending config to devices. startupMode: ", config.device_startMode)
  })

  ipcMain.on('setMode', (event, arg) => {
    let msg = {
      type: MSG_TYPE_SET_MODE,
      data: arg
    }
    broadcastWSS(JSON.stringify(msg))
    //console.log("[ipc] setMode: %u", arg)
    console.log("[ipc] setMode: " + arg)
  })

  ipcMain.on('sendServerIP', (event) => {
    sendServerIP()
  })

  ipcMain.on('openDeviceMonitor', (event) => {
    openDeviceMonitor()
  })

  ipcMain.on('monitorRefresh', (event) => {
    // update monitor after scanning addresses:
    wss.clients.forEach((_ws) => {
      updateMonitor(_ws.isAlive, _ws.address, _ws.battery, _ws.firmware)
    })
  })

  ipcMain.on('focusState', (event, id, state) => {
    let arg = null
    if (state) arg = 1      // test mode
    else arg = 0            // regular mode
    let msg = {
      type: MSG_TYPE_SET_MODE,
      data: arg
    }
    wss.clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN && client.address == id) {
            client.send(JSON.stringify(msg))
        }
    })
    console.log("[ipc] focusState: " + id + "=>" + state)
  })

  function remoteStatusConsole(msg, data) {
    if (win) win.send(msg, data)
  }

  function updateMonitor(conn, addr, batt, fw) {
    //let arg = [addr, batt, fw]
    if (mon) mon.send('update-spores', conn, addr, batt, fw)
  }



/* --------- OSC MESSAGING --------- */
  // using low level api of OSC lib allows us to send broadcast messages
  function sendServerIP() {
    let buf = Buffer.alloc(4)
    ip.toBuffer(ip.address(), buf, 0)
    //console.log("[osc] my ip: " + buf[0] + "." + buf[1] + "." + buf[2] + "." + buf[3])
    const message = new OSC.Message('/server', buf[0], buf[1], buf[2], buf[3])
    const binary = message.pack()
    //socket.send(new Buffer(binary), 0, binary.byteLength, config.osc_port, config.osc_host)
    socket.send(new Buffer(binary), 0, binary.byteLength, config.osc_port, ip.or(ip.address(), '0.0.0.255'))
    //console.log("[osc] osc sending my IP (" + ip.address() + ") to " + config.osc_host + ":" + config.osc_port)
    console.log("[osc] osc sending my IP (" + ip.address() + ") to " + ip.or(ip.address(), '0.0.0.255') + ":" + config.osc_port)
  }

  function broadcastRestartOSC() {
    const message = new OSC.Message('/restart')
    const binary = message.pack()
    socket.send(new Buffer(binary), 0, binary.byteLength, config.osc_port, ip.or(ip.address(), '0.0.0.255'))
    console.log("[osc] broadcast restart")
  }



/* --------- FIRMWARE (EXPRESS SERVER): --------- */
  // maybe switch this to ES6 import/export (do I need babel?) and put in another file.
  // for now get the same styling as root index:
  fw.use('/fonts', express.static('fonts'))
  fw.use('/css', express.static('css'))
  fw.use(express.static('firmware'))
  fw.listen(config.firmware_server_port, () => console.log('[fw] firmware server listening at ' + ip.address() + ":" + config.firmware_server_port))
