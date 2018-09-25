#ifndef OTA_FIRMWARE_H
#define OTA_FIRMWARE_H

#include "settings.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

bool checkForNewFirmware(uint16_t newFWVersion, String fwUrlBase, String fwName) {
  String fwURL = String(fwUrlBase);
  fwURL.concat(fwName);
  String fwVersionURL = fwURL;
  fwVersionURL.concat(".version");    // version number file (use this number to check if update available)
  // always print these since we don't need to be measuring the
  Serial.println( "\n[OTA] Checking for firmware updates." );
  Serial.print( "[OTA] FW Name: " );
  Serial.println( fwName );
  Serial.print( "[OTA] Firmware version URL: " );
  Serial.println( fwVersionURL );


  /* check available firmware version: */
  Serial.print("[OTA] Current firmware version: ");
  Serial.println(FW_VERSION);
  Serial.print("[OTA] Available firmware version: ");
  Serial.println(newFWVersion);

  /* see if available firmware is newer: */
  if (newFWVersion > FW_VERSION) {
    Serial.println("[OTA] Preparing to update...");
    String fwImageURL = fwURL;
    fwImageURL.concat(".bin");
    t_httpUpdate_return ret = ESPhttpUpdate.update(fwImageURL);   // OTA updater
    /* in case the updater fails: */
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("[OTA] HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[OTA] HTTP_UPDATE_NO_UPDATES");
        break;
    }
  }
  else {
    Serial.println("[OTA] Already using latest version");
  }
  return false;   // if the update succeeds then we never get this far! 
}


#endif /* OTA_FIRMWARE_H */

