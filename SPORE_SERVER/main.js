// this file should create windows and handl all system events
/*  try electron-reload to monitor changes:
    https://www.npmjs.com/package/electron-reload
*/
// use external config:
// const { host, port } = require('./config');    // config.json
// console.log(`Server will run at http://${host}:${port}`);
// >> "Server will run at http://localhost:8080"
// OR JSON.parse: https://stackoverflow.com/questions/5726729/how-to-parse-json-using-node-js
const {
    app, BrowserWindow, ipcMain
} = require('electron')
const fs = require('fs')
let config = JSON.parse(fs.readFileSync('config.json'))
console.log("[INFO] config:")
console.log(config)
const WebSocket = require('ws')
const wss = new WebSocket.Server({
    port: config.websocket_port
})
const express = require('express')
const fw = express()
const ip = require('ip')
const MSG_TYPE_SET_ADDRESS = 0
const MSG_TYPE_SET_MODE = 1
const MSG_TYPE_CHECK_FIRMWARE = 2
const MSG_TYPE_CONFIG = 3
const MSG_TYPE_REBOOT = 4
const MSG_TYPE_SCAN = 5
const MSG_TYPE_REQUEST_ADDRESS = 6
const MSG_TYPE_CONNECT_INFO = 7
let win = null
let addressCounter = 0
let addressScanning = false
    /* -------- ELECTRON-RELOAD (monitor file changes and restart): ---------- */
const path = require('path')
    // for some reason, __dirname isn't working here (using **/*.* instead):
require('electron-reload')('**/*.*', {
        //electron: path.join(__dirname, 'node_modules', '.bin', 'electron'),
        electron: path.join(__dirname, 'node_modules', 'electron')
        , hardResetMethod: 'exit'
    })
    /* --------- ELECTRON --------- */
function createWindow() {
    // render process:
    win = new BrowserWindow({
        width: 700
        , height: 450
    })
    win.loadFile('index.html')
    win.on('closed', () => {
        win = null
    })
    win.setMenu(null) //Hide window menu
}
app.on('ready', createWindow)
    // macOS close window but not app:
    // app.on('window-all-closed', () => {
    //   console.log("ready to diiieeee!!!")
    //   setTimeout(() => {app.quit()}, 1000)
    // })
    /* ------- WEBSOCKETS: -------- */
    //https://www.npmjs.com/package/ws
    //https://www.npmjs.com/package/ws#how-to-detect-and-close-broken-connections
    // websocket terminal client:
    // > npm install -g wscat
    // > wscat -c <serverIP>:8080
wss.on('connection', (_ws, req) => {
    let remoteIP = req.connection.remoteAddress
    console.log("[wss] %s connected", remoteIP)
    remoteStatusConsole('devices-connected', wss.clients.size) // this might not be accurate (if a device reconnects it returns expected size +1)
    _ws.isAlive = true
    _ws.on('pong', heartbeatWS)
    _ws.on('message', onMessageWS)
})

function heartbeatWS() {
    this.isAlive = true // 'this' refers to the device that sent the pong
        //remoteStatusConsole('devices-connected', wss.clients.size)
}
let prevClientsSize = 0

function pingWS() {
    wss.clients.forEach((_ws) => {
            if (_ws.isAlive === false) return _ws.terminate()
            _ws.isAlive = false
            _ws.ping(() => {}) //pass noop function
        })
        //console.log("[wss] ping")
    if (wss.clients.size != prevClientsSize) {
        remoteStatusConsole('devices-connected', wss.clients.size)
    }
    prevClientsSize = wss.clients.size
}
setInterval(() => {
    pingWS()
}, config.ping_interval)

function onMessageWS(_msg) {
    const msg = 0;
    if (_msg) {
        try {
            msg = JSON.parse(_msg)
        }
        catch (e) {
            console.log("[wss] not JSON: ", _msg)
            remoteStatusConsole('print-message', 'JSON error')
            return;
        }
        if (msg.type != undefined) {
            if (msg.type == MSG_TYPE_CONNECT_INFO) {
                if (msg.data != undefined) {
                    console.log("[wss] device %s connected. FW:%s", msg.data.address, msg.data.firmwareVersion)
                }
            }
            else if (msg.type == MSG_TYPE_REQUEST_ADDRESS) {
                if (msg.data != undefined) {
                    console.log("[wss] address requested from: %s", msg.data)
                    let response = {
                        type: MSG_TYPE_SET_ADDRESS
                        , data: addressCounter
                    }
                    let addr = (addressCounter < 10) ? (addressCounter + " ") : addressCounter // align MACs for debug
                    console.log("[wss] address: %s    sent to: %s", addr, msg.data)
                    addressCounter++;
                    this.send(JSON.stringify(response))
                }
            }
            else {
                console.log("[wss] message type: %s", msg.type)
            }
        }
        else {
            console.log("[wss] received empty message")
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
        type: MSG_TYPE_CHECK_FIRMWARE
        , data: {
            version: config.firmware_version
            , url: "http://" + ip.address() + ":" + config.firmware_server_port + "/"
            , filename: config.firmware_filename
        }
    };
    broadcastWSS(JSON.stringify(msg))
    console.log("[ipc] check Firmware")
})
ipcMain.on('scanDevices', (event) => {
    // [ADDR] clear address array
    addressScanning = true
    addressCounter = 0
    let msg = {
        type: MSG_TYPE_SCAN
    }
    broadcastWSS(JSON.stringify(msg))
    console.log("[ipc] scan devices")
})

function remoteStatusConsole(msg, data) {
    win.send(msg, data)
}
/* --------- FIRMWARE (EXPRESS SERVER): --------- */
// maybe switch this to ES6 import/export (do I need babel?) and put in another file.
// for now get the same styling as root index:
fw.use('/fonts', express.static('fonts'))
fw.use('/css', express.static('css'))
fw.use(express.static('firmware'))
fw.listen(config.firmware_server_port, () => console.log('[fw] firmware server listening at ' + ip.address() + ":" + config.firmware_server_port))