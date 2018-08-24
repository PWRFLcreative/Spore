// http://youmightnotneedjquery.com/
const {ipcRenderer} = require('electron')

window.onload = () => {
  initView()
}

// Spore constructor:
function Spore(addr) {
  this.addr = addr
  this.el = document.getElementById("spore-template").cloneNode(true)
  this.el.setAttribute( "id", "spore-" + addr )
  this.battery = this.el.querySelector( ".battery" )
  this.firmware = this.el.querySelector( ".firmware" )
  // querySelectorAll ??

  this.init = function() {
      this.onDisconnect();
      this.el.querySelector( ".address" ).textContent = addr
  }
  this.onConnect = function() {
    // change appearance
  }
  this.onDisconnect = function() {
    // change appearance
  }
  this.batteryLevel = function() {
    // display battery level
  }
  this.firmwareVersion = function() {
    // display firmware version
  }
  // on click, send test mode?
}

function initView() {
  let numSpores = 100
  let numPerRow = 10
  let numPerCol = numSpores / numPerRow
  let itemWidth = 40
  let itemSpacing = 2

  let spores = []

  let col = 0
  let row = 0

  for ( let i = 0; i < numSpores; ++i ) {
      let spore = new Spore( i+1 )
      spore.init()
      spores[i] = spore
      //gridItem.$el.css( { "width": itemWidth, "margin": "0 " + itemSpacing + "px " + (itemSpacing / 2)+ "px 0" } );
      //gridItem.$el.css( { "width": itemWidth, "margin": "0 " + itemSpacing + "px " + (itemSpacing * 1.5)+ "px 0" } );
      spore.el.style.width = itemWidth + "px"
      spore.el.style.margin = itemSpacing + "px"

      document.body.appendChild( spore.el )

      col += 1

      if ( col == numPerRow ) {
          col = 0
          ++row
      }
  }
}
