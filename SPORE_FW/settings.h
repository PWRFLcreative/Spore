#ifndef SETTINGS_H
#define SETTINGS_H

#include "wifi_config.h"   
// to track and stop tracking config.h: 
// git update-index --assume-unchanged FILE_NAME 
// git update-index --no-assume-unchanged FILE_NAME


/* -- network settings -- */
const char* AP_SSID  = "SPORE-TEST";
const char* ssid     = WIFI_SSID;        // ssid from wifi_config.h
const char* password = WIFI_PASSWORD;    // password from wifi_config.h
String deviceName    = "Spore_";         // used for DHCP



/* -- firmware settings -- */
#define HW_VERSION "0.0.3"
#define HW_PHASE   "-beta"
const int FW_VERSION = 100;              // 0.1.00, convention: 1.2.10 = 1210, 0.5.9 = 509, no leading 0s or it is interpreted as octal.. learned that the hard way!
#define FW_PHASE   "-alpha"
//float fwCheckButtonTime = 2000.0f;     // how long to hold button down.


/* -- general settings -- */
#define CHAN_PER_FIXTURE 3               // number of channels per fixture
uint8_t address = 99;




#endif /* SETTINGS_H */
