const {ipcRenderer} = require('electron')

let status_console = document.getElementById('status-console')
let button_fw      = document.getElementById('checkFirmwareButton')
let button_scan    = document.getElementById('scanButton')
let button_mode0   = document.getElementById('modeButton0')
let button_mode1   = document.getElementById('modeButton1')
let button_mode2   = document.getElementById('modeButton2')



window.onload = () => {
  statusConsole("ready and waiting with bated breath")
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
  statusConsole(arg + " devices connected")
})
ipcRenderer.on('print-message', (event, arg) => {
  statusConsole(arg)
})



/* ------ EVENT LISTENERS ------ */
button_fw.addEventListener('click', checkFirmware)    // maybe try dblclick!
button_scan.addEventListener('click', scanDevices)
button_mode0.addEventListener('click', () => {setMode(0)})
button_mode1.addEventListener('click', () => {setMode(1)})
button_mode2.addEventListener('click', () => {setMode(2)})
