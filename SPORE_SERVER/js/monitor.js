// http://youmightnotneedjquery.com/
const {ipcRenderer} = require('electron')

let button_refresh = document.getElementById('monitorRefreshButton')

window.onload = () => {
  initView()
}

// Spore constructor:
function Spore(addr) {
  this.addr = addr
  this.focus = false
  this.connected = false
  this.lowBatt = false

  this.el = document.getElementById("spore-template").cloneNode(true)
  this.el.setAttribute( "id", "spore-" + addr )

  this.battery = 0
  this.firmware = 0
  // querySelectorAll ??

  this.init = function() {
    this.onDisconnect();
    this.el.querySelector( ".address" ).textContent = addr
    //console.log(addr)
  }

  this.onConnect = function() {
    this.connected = true
    this.el.classList.toggle("connected", this.connected)
    console.log("connect %s", this.addr)
  }

  this.onDisconnect = function() {
    this.connected = false
    this.focus = false
    this.lowBatt = false
    this.el.classList.toggle("focused", this.focus)
    this.el.classList.toggle("connected", this.connected)
    this.el.classList.toggle("lowBatt", this.lowBatt)
    this.el.querySelector( ".battery" ).textContent = '--'
    this.el.querySelector( ".firmware" ).textContent = '--'
    // change appearance
  }

  this.batteryLevel = function(batt) {
    //this.battery = batt
    this.lowBatt = false

    if (batt > 0) {
      this.battery = (batt-2.75)/1.45*100     // convert to % (battery range 2.75 to 4.2)
      let battIcon = 'fa-battery-full'
      if (this.battery < 75) {
        battIcon = 'fa-battery-three-quarters'
        if (this.battery < 50) {
          battIcon = 'fa-battery-half'
          if (this.battery < 25) {
            battIcon = 'fa-battery-quarter'
            if (this.battery < 20) {
              battIcon = 'fa-battery-empty'
              this.lowBatt = true
            }
          }
        }
      }
      //this.battery = batt
      this.el.classList.toggle("lowBatt", this.lowBatt)
      this.el.querySelector( ".battery" ).innerHTML = '<span style="font-size:1.0em" class="fas ' + battIcon + '"></span> ' + this.battery.toFixed(0) + '%'
    }
    else {
      this.battery = 0
      this.el.querySelector( ".battery" ).textContent = '<span class="fas fa-battery-slash"></span>'
    }
  }

  this.firmwareVersion = function(fw) {
    this.firmware = fw
    // let fw_formatted = fw
    // if (fw < 1000) {
    //   fw_formatted = 0 + '.' + Math.floor(fw/100%10) + '.'
    // }
    // else {
    //   fw_formatted = Math.floor(fw/1000%10) + '.' + Math.floor(fw/100%10) + '.'
    // }
    // if (fw%100 < 10) {
    //   fw_formatted += '0' + fw%100
    // }
    // else {
    //   fw_formatted += fw%100
    // }
    let _fw = fw.toString()
    if (fw < 1000) {  // 0.x.xx
        _fw = 0 + '.' + _fw.charAt(0) + '.' + _fw.charAt(1) + _fw.charAt(2)
    }
    else {            // x.x.xx
        _fw = _fw.charAt(0) + '.' + _fw.charAt(1) + '.' + _fw.charAt(2) + _fw.charAt(3)
    }
    this.el.querySelector( ".firmware" ).textContent = _fw
  }

  this.toggleFocus = function() {
    if (this.connected) {
      this.focus = !this.focus
      this.el.classList.toggle("focused", this.focus)
      console.log("%s %s", this.el.id, this.focus)
    }
  }

  this.el.addEventListener('click', () => {this.toggleFocus()})
  // on click, send test mode?
}

let spores = []
let numSpores = 100
function initView() {
  let numPerRow = 10
  let numPerCol = numSpores / numPerRow
  let itemWidth = 40
  let itemSpacing = 2

  // let col = 0
  // let row = 0

  for ( let i = 0; i < numSpores; ++i ) {
      let spore = new Spore( i )
      spore.init()
      spores[i] = spore
      //gridItem.$el.css( { "width": itemWidth, "margin": "0 " + itemSpacing + "px " + (itemSpacing / 2)+ "px 0" } );
      //gridItem.$el.css( { "width": itemWidth, "margin": "0 " + itemSpacing + "px " + (itemSpacing * 1.5)+ "px 0" } );
      spore.el.style.width = itemWidth + "px"
      spore.el.style.margin = itemSpacing + "px"

      let container = document.getElementById('spore-container')
      container.appendChild( spore.el )
      // document.body.appendChild( spore.el )

      // col += 1
      // if ( col == numPerRow ) {
      //     col = 0
      //     ++row
      // }
  }
}

ipcRenderer.on('update-spores', (event, conn, addr, batt, fw) => {
  if (addr < numSpores) {
    console.log("connectado: %s, %s, %s, %s", conn, addr, batt, fw)
    if (conn) {
      spores[addr].onConnect()
      spores[addr].batteryLevel(batt)
      spores[addr].firmwareVersion(fw)
    }
    else {
      spores[addr].onDisconnect()
      // //spores[addr].batteryLevel(0)
      // spores[addr].firmwareVersion('--')
    }
  }
})

function monitorRefresh() {
  ipcRenderer.send('monitorRefresh')
}

button_refresh.addEventListener('click', monitorRefresh)
