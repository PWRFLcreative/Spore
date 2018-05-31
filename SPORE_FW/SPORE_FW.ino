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

extern "C" {
#include "user_interface.h"           // allows wifi_set_sleep_type
}
#include "settings.h"                   // local settings!
//#include "FS.h"                       // File System / SPIFFS
#include <NeoPixelBus.h>
#include <WiFiManager.h>                // Captive portal wifi setup
//#include <ESP8266WiFiMulti.h>         // alternate to wifimanager - no portal but remembers multiple access points
#include <E131.h>                       // sACN (e1.31) library

#define LED_BUILTIN   2                 // not correctly mapped for ESP-12x
#define BOOTLOAD_PIN  0                 // BOOTLOAD button

const uint16_t PixelCount = 16;                                   // DO NOT CHANGE!!
NeoGamma<NeoGammaTableMethod> colorGamma;                         // for any fade animations, best to correct gamma
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> pixels(PixelCount);   // pin 3 is DMA anyway so the value is actuallly ignored..

WiFiManager wifiManager;
E131 e131;





void setup() {
  wifi_set_sleep_type(NONE_SLEEP_T);          // disable sleep (added this when trying to fix analogRead issue.. )
  Serial.begin(115200);
  Serial.printf("\n\n\nSPORE\n");
  Serial.printf("FW: %u%s   ", FW_VERSION, FW_PHASE);
  Serial.printf("HW: %s%s\n", HW_VERSION, HW_PHASE);
  Serial.printf("(c)2018 PWRFL\n\n");         // pick open source license

  if (!wifiManager.autoConnect(AP_SSID)) {    // no password for captive portal
    Serial.println("no one connected and I timed out. I'll try to connect (in STA) again!"); delay(3000);
    ESP.reset(); delay(5000);
  }
  //address = WiFi.localIP()[3];              // get address
  address = 1;                                // TESTING - just set to 1 manually for now
  deviceName.concat(address);

  /* init LED PIN */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);             // low is on

  /* LEDs */
  delay(100);
  pixels.Begin();
  pixels.Show();

  /* sACN
     The following method creates
     the UDP port and joins the
     multicast group for you.
  */
  e131.begin(E131_MULTICAST, 1, 1);           // (E131_UNICAST/E131_MULTICAST, universeNumber,number of universes)

  /* who am I this time?  */
  delay(100);
  Serial.printf("\nWiFi connected.\n");
  Serial.printf("\nIP address:  ");
  //Serial.println(DMX_AP ? WiFi.softAPIP() : WiFi.localIP());
  Serial.println(WiFi.localIP());
  //if (ENABLE_WEBSOCKET) Serial.printf("Websocket looking for server at: %s:%u\n", serverIP.toString().c_str(), wsPort);
  Serial.printf("%s (%s) ready. \n", deviceName.c_str(), WiFi.macAddress().c_str());
  Serial.printf("\n---\n\n");
  yield();
}



void loop() {
  if (e131.parsePacket()) {
    uint8_t r = e131.data[address];
    uint8_t g = e131.data[address+1];
    uint8_t b = e131.data[address+2];
    RgbColor col = colorGamma.Correct(RgbColor(r, g, b));
    for (int i = 0; i < PixelCount; i++) {
      pixels.SetPixelColor(i, col);
    }
  }
//  uint16_t numChannels = e131.parsePacket();
//  if (numChannels > 0) {
//    Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n",
//                  e131.universe,              // The Universe for this packet
//                  numChannels,                // Number of channels in this packet
//                  e131.stats.num_packets,     // Packet counter
//                  e131.stats.packet_errors,   // Packet error counter
//                  e131.data[0]);              // Dimmer data for Channel 1
//  }
}
