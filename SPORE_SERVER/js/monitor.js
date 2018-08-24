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
    this.focus = false;
    this.el.classList.toggle("focused", this.focus)
    this.el.classList.toggle("connected", this.connected)
    // change appearance
  }
  this.batteryLevel = function(batt) {
    this.battery = batt
    this.el.querySelector( ".battery" ).textContent = batt
  }
  this.firmwareVersion = function(fw) {
    this.firmware = fw
    this.el.querySelector( ".firmware" ).textContent = fw
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

      document.body.appendChild( spore.el )

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
      spores[addr].batteryLevel('--')
      spores[addr].firmwareVersion('--')
    }
  }
})

function monitorRefresh() {
  ipcRenderer.send('monitorRefresh')
}

button_refresh.addEventListener('click', monitorRefresh)
