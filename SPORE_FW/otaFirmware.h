#ifndef OTA_FIRMWARE_H
#define OTA_FIRMWARE_H

#include "settings.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

bool checkForNewFirmware(String fwUrlBase, String fwFilename) {
  String fwURL = String(fwUrlBase);
  fwURL.concat(fwFilename);
  String fwVersionURL = fwURL;
  fwVersionURL.concat(".version");    // version number file (use this number to check if update available)
  // always print these since we don't need to be measuring the
  Serial.printf( "\n[OTA] Checking for firmware updates...\n" );
  String fwImageURL = fwURL;
  fwImageURL.concat(".bin");
  Serial.printf(" %s\n", fwImageURL.c_str());
  t_httpUpdate_return ret = ESPhttpUpdate.update(fwImageURL);   // OTA updater
  /* in case the updater fails: */
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] HTTP_UPDATE_NO_UPDATES\n");
      break;
  }
  return false;   // if the update succeeds then we never get this far! 
}

#endif /* OTA_FIRMWARE_H */
