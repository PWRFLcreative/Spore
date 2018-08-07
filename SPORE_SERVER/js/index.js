const {ipcRenderer} = require('electron')

let status_console = document.getElementById('status-console')
let button_fw      = document.getElementById('checkFirmwareButton')
let button_scan    = document.getElementById('scanButton')



window.onload = () => {
  statusConsole("chillin'..")
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



/* -------- MESSAGING (main <--> render processes) -------- */
ipcRenderer.on('clear-status-console', (event) => {
  clearStatusConsole()
})
ipcRenderer.on('devices-connected', (event, arg) => {
  statusConsole(arg + " devices connected.")
})
ipcRenderer.on('print-message', (event, arg) => {
  statusConsole(arg)
})



/* ------ EVENT LISTENERS ------ */
button_fw.addEventListener('click', checkFirmware)    // maybe try dblclick!
button_scan.addEventListener('click', scanDevices)
