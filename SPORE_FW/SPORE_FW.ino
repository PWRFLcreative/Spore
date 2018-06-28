/*
    SPORE FIRMWARE
    Copyright (C) 2018 PWRFL

    @authors Leó Stefánsson, Tim Rolls, and Brendan Matkin

    This sketch contains the firmware for the "Spore" hardware.
    The intent is to create an installation that periodically remaps wireless LEDs ("Spores")
    and fascilitates an interactive element.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
 * USE ESP8266 Arduino Core v2.4.0-rc2 (https://github.com/esp8266/Arduino)
 * Board: NodeMCU v1.0
 * Flash Size: 4M (1M SPIFFS)
 * Upload Speed: 230400   --this works consistentnly with classic FTDI. Newer chips may work at 921600
*/


extern "C" {
  #include "user_interface.h"           // allows wifi_set_sleep_type
}

#include "settings.h"                   // local settings!
#include "otaFirmware.h"                // over the air firmware updates!
#include "serverControl.h"
//#include "FS.h"                       // File System / SPIFFS
#include <NeoPixelBus.h>
//#include <ESP8266WiFiMulti.h>         // alternate to wifimanager - no portal but remembers multiple access points
#include <E131.h>                       // sACN (e1.31) library

#define LED_BUILTIN   2                 // not correctly mapped for ESP-12x
#define BOOTLOAD_PIN  0                 // BOOTLOAD button



const uint16_t PixelCount = 16;                                   // DO NOT CHANGE!!
NeoGamma<NeoGammaTableMethod> colorGamma;                         // for any fade animations, best to correct gamma
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(PixelCount);  // This is DMA so uses GPIO 3 (alawys same physical pin as U0RXD)

E131 e131;                              // sACN library




/******** SETUP ********/

void setup() {
  wifi_set_sleep_type(NONE_SLEEP_T);          // disable wifi sleep (added this when trying to fix analogRead issue.. is it still needed?)
  Serial.begin(115200);
  Serial.printf("\n\n\nSPORE\n");
  Serial.printf("FW: %u%s   ", FW_VERSION, FW_PHASE);
  Serial.printf("HW: %s%s\n", HW_VERSION, HW_PHASE);
  Serial.printf("(c)2018 PWRFL\n\n");         // pick open source license

  EEPROM.begin(512);
  //EEPROM.get(0, address);
  address = EEPROM.read(0);
  //address = WiFi.localIP()[3];              // get address
  //address = 0;                                // TESTING - just set to 1 manually for now
  //String macAddress = WiFi.macAddress();    
  deviceName.concat(address);
  
    /* LEDs */
  delay(100);
  pixels.Begin();
  for (int i = 0; i < PixelCount; i++) {
    pixels.SetPixelColor(i, RgbColor(0));
  }
  pixels.Show();

  /* connect to WiFi */
  Serial.printf("[INFO] Connecting to: ");
  Serial.print(ssid);
  //WiFi.persistent(false);                   // don't re-write ssid/password to flash every time (avoid degredation)
  WiFi.mode(WIFI_STA);                        // without this ESP will be station and AP at the same time - this can give you headaches!!!
  WiFi.hostname(deviceName);                  // DHCP Hostname    -- does this even work?!
  //WiFi.config(staticIP, gateway, subnet);   // set static IP, defaults to DHCP if config not called
  WiFi.begin(ssid, password);                 // connect to your existing network
  int restartCounter = 0;
  while (!WiFi.isConnected()) {               // auto reset if it's not connecting (occasionally hangs otherwise)
    delay(100); Serial.print("."); 
    restartCounter++;
    if (restartCounter > 50) ESP.restart();   // if it takes more than 5 (50x100ms) seconds to connect, restart!
  }
  Serial.printf("  connected.\n");            // yay it worked!

  /* init LED PIN */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);             // low is on


  /* sACN
     The e131.begin() method creates
     the UDP port and joins the
     multicast group for you.
  */
  Serial.printf("Starting sACN: ");
  e131.begin(E131_MULTICAST, 1, 1);           // (E131_UNICAST <or> E131_MULTICAST, universeNumber, number of universes)

  /* setup websockets */
  webSocket.begin(serverIP.toString(), wsPort, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);       // TODO: Do I need to manually ping/pong?? 
  
  /* who am I this time?  */
  delay(100);
  Serial.printf("\nWiFi connected.\n");
  Serial.printf("\nIP address:  ");
  Serial.println(WiFi.localIP());
  Serial.printf("sACN address: %u\n", address);
  Serial.printf("Looking for Websocket server at: %s:%u\n", serverIP.toString().c_str(), wsPort);
  Serial.printf("%s (%s) ready. \n", deviceName.c_str(), WiFi.macAddress().c_str());
  Serial.printf("\n---\n\n");
  yield();
}




/******** LOOP ********/

void loop() {
  webSocket.loop();
  if (e131.parsePacket()) {
    /* NOTE: ADDRESSES SHOULD PROBABLY START AT 0 IF WE ARE NOT DERIVING THEM FROM OR ASSIGNING THEM TO IP ADDRESSES */
    uint8_t r = e131.data[address*CHAN_PER_FIXTURE];           // address starts at 0 
    uint8_t g = e131.data[address*CHAN_PER_FIXTURE+1];
    uint8_t b = e131.data[address*CHAN_PER_FIXTURE+2];
    RgbColor col = colorGamma.Correct(RgbColor(r, g, b));
    for (int i = 0; i < PixelCount; i++) {
      pixels.SetPixelColor(i, col);
    }
    digitalWrite(LED_BUILTIN, HIGH);            // (low is on)
  }
  pixels.Show();
  
  digitalWrite(LED_BUILTIN, HIGH);              // turn off LED at the end of every loop (low is on)
  yield();
}


/*
 e131 sample parse: 
  uint16_t numChannels = e131.parsePacket();
  if (numChannels > 0) {
    Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
                  e131.universe,              // The Universe for this packet
                  numChannels,                // Number of channels in this packet
                  e131.stats.num_packets,     // Packet counter
                  e131.stats.packet_errors,   // Packet error counter
                  e131.data[0]);              // Dimmer data for Channel 1
  }
*/
