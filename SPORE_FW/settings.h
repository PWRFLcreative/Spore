#ifndef SETTINGS_H
#define SETTINGS_H


/* -- network settings -- */
const char* AP_SSID = "SPORE";
String deviceName = "Spore_";                 // used for DHCP


/* -- firmware settings -- */
#define HW_VERSION "0.0.3"
#define HW_PHASE   "-beta"
const int FW_VERSION = 100;                   // 0.1.00, convention: 1.2.10 = 1210, 0.5.9 = 509, no leading 0s or it is interpreted as octal.. learned that the hard way!
#define FW_PHASE   "-alpha"
float fwCheckButtonTime = 2000.0f;            // how long to hold button down.
//const char* fwUrlBase = "http://10.0.3.100/";
//const char* fwName = "spore_fw";



#endif /* SETTINGS_H */
