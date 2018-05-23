/*
 *  SPORE FIRMWARE
 *  Copyright (C) 2018 PWRFL
 *  
 *  @authors Leó Stefánsson, Tim Rolls, and Brendan Matkin
 *
 *  This sketch contains the firmware for the "Spore" hardware.
 *  The intent is to create an installation that periodically remaps wireless LEDs ("Spores")
 *  and fascilitates an interactive element.
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

extern "C" {
  #include "user_interface.h"   // allows wifi_set_sleep_type
}
#include "settings.h"
#include "FS.h"
#include <NeoPixelBus.h>
#include <WiFiManager.h>

#define LED_BUILTIN   2                 // not correctly mapped for ESP-12x
#define BOOTLOAD_PIN  0                 // BOOTLOAD button

const uint16_t PixelCount = 16;                                   // DO NOT CHANGE!!
NeoGamma<NeoGammaTableMethod> colorGamma;                         // for any fade animations, best to correct gamma
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, 3);// pin 3 is DMA anyway so the value is actuallly ignored..

WiFiManager wifiManager;







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

  /* init LED PIN */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);             // low is on

  /* LEDs */
  delay(100);
  strip.Begin();
  strip.Show();

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
  
}
