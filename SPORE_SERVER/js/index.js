const {ipcRenderer} = require('electron')
const ip = require('ip')
const tt = require('electron-tooltip')
tt({
    position: 'top',
    width: 250,
    style: {
        backgroundColor: '#333333',
        borderRadius: '4px'
    }
})

let status_console = document.getElementById('status-console')
let button_fw      = document.getElementById('checkFirmwareButton')
let button_sendip  = document.getElementById('sendIPButton')
let button_scan    = document.getElementById('scanButton')
let button_mode0   = document.getElementById('modeButton0')
let button_mode1   = document.getElementById('modeButton1')
let button_mode2   = document.getElementById('modeButton2')
let button_monitor = document.getElementById('openMonitorButton')



window.onload = () => {
  statusConsole("ready and waiting with bated breath")
}


function openDeviceMonitor() {
  ipcRenderer.send('openDeviceMonitor')
  //statusConsole("opening monitor")
}


function clearStatusConsole() {
    status_console.innerHTML = "&nbsp;"
}

function statusConsole(msg) {
    status_console.innerHTML += msg + "<br>"
    status_console.scrollTop = status_console.scrollHeight
}

function checkFirmware() {
    ipcRenderer.send('checkFirmware')
    statusConsole("devices checking for new firmware..")
}

function sendServerIP() {
  ipcRenderer.send('sendServerIP')
  statusConsole("sending my IP (" + ip.address() + ") to the spores")
}

function scanDevices() {
    ipcRenderer.send('scanDevices')
    statusConsole("scanning devices (and configure addresses)..")
}

function setMode(arg) {
  ipcRenderer.send('setMode', arg)
  statusConsole("set mode to: " + arg)
}


/* -------- MESSAGING (main <--> render processes) -------- */
ipcRenderer.on('clear-status-console', (event) => {
    clearStatusConsole()
})
ipcRenderer.on('devices-connected', (event, arg) => {
  statusConsole(arg + " device(s) connected")
})
ipcRenderer.on('print-message', (event, arg) => {
  statusConsole(arg)
})
ipcRenderer.on('firmware-version', (event, arg) => {
  button_fw.innerHTML += '(' + arg + ')'
  console.log(arg)
})



/* ------ EVENT LISTENERS ------ */
button_fw.addEventListener('click', checkFirmware)    // maybe try dblclick!
button_scan.addEventListener('click', scanDevices)
button_sendip.addEventListener('click', sendServerIP)
button_monitor.addEventListener('click', openDeviceMonitor)
button_mode0.addEventListener('click', () => {setMode(0)})
button_mode1.addEventListener('click', () => {setMode(1)})
button_mode2.addEventListener('click', () => {setMode(2)})
