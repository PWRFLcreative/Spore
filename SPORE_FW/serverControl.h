#ifndef SERVER_CONTROL_H
#define SERVER_CONTROL_H

/*
  websocket terminal client:
  > npm install -g wscat
  > wscat -c <serverIP>:8080
*/

#include <ArduinoJson.h>
#include <Hash.h>
#include "OSCMessage.h"



// eg. message types in server
//const MSG_TYPE_SET_ADDRESS      = 0
//const MSG_TYPE_SET_MODE         = 1
//const MSG_TYPE_CHECK_FIRMWARE   = 2
//const MSG_TYPE_CONFIG           = 3
//const MSG_TYPE_REBOOT           = 4
//const MSG_TYPE_SCAN             = 5
//const MSG_TYPE_REQUEST_ADDRESS  = 6
//const MSG_TYPE_CONNECT_INFO     = 7
//const MSG_TYPE_BATTERY          = 8

enum MsgType : uint8_t  {
  SET_ADDRESS = 0,
  SET_MODE,
  CHECK_FIRMWARE,
  CONFIG,
  REBOOT,
  SCAN,
  REQUEST_ADDRESS,
  CONNECT_INFO,
  BATTERY
};
volatile uint8_t testStepper = 0;      // for the test fade cycle


/* ---- OSC Callback (server IP): ---- */
void updateServerIP(OSCMessage &msg, int addressOffset) {
  bool _serverChanged = false;
  for (int i = 0; i < 4; i++) {
    if (msg.isInt(i)) {
      serverIP[i] = msg.getInt(i);
      if (serverIP[i] != EEPROM[i+1]) {
        EEPROM[i+1] = serverIP[i];
        _serverChanged = true;
      }
    }
  }
  if (_serverChanged) {
    EEPROM.commit();
    webSocket.begin(serverIP.toString(), wsPort, "/");
    Serial.printf("[osc] New Websocket server at: %s:%u\n", serverIP.toString().c_str(), wsPort);
    //delay(10);
    yield();
    ESP.restart();
  }
  else {
    Serial.printf("[osc] websocket server unchanged\n");
  }
}
/* ---- OSC Callback (restart): ---- */
void restartOSC(OSCMessage &msg, int addressOffset) {
  if (msg.isInt(0)) {
    if (msg.getInt(0)) {
      // reserved for future use
    }
  }
  Serial.printf("[osc] received OSC-based restart (probably broadcast, probably because some of us are not behaving)\n");
  ESP.restart();
}


/* ---- websocket<-->JSON functions ---- */
const uint16_t jsonSendSize = 256;
void serializeJSON_connected(char * json) {
  StaticJsonBuffer<jsonSendSize> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = (uint8_t)CONNECT_INFO;
  JsonObject& data = root.createNestedObject("data");
  data["address"] = (uint8_t)address;
  data["firmwareVersion"] = FW_VERSION;
  root.printTo(json, jsonSendSize);
  Serial.println(json);
}

void serializeJSON_scan(char * json) {
  StaticJsonBuffer<jsonSendSize> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = (uint8_t)REQUEST_ADDRESS;
  root["data"] = WiFi.macAddress();
  root.printTo(json, jsonSendSize);
  Serial.println(json);
}

void serializeJSON_battery(char *json) {
  StaticJsonBuffer<jsonSendSize> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = (uint8_t)BATTERY;
  root["data"] = batteryVoltage;
  root.printTo(json, jsonSendSize);
  //Serial.println(json);
}

bool deserializeJSON(uint8_t * json) {
  StaticJsonBuffer<512> jsonBuffer;     // actually only needs to be about 240 bytes, extra reserved for future use (or just because I have the space..). (http://arduinojson.org/assistant/)
  JsonObject& root = jsonBuffer.parseObject(json);
//const char* exampleNestedValue = root["object"]["key"];
//const char* exampleValue = root["key"];
  if (!root.success()) {
    Serial.printf("[ws] JSON parseObject() failed.\n");
    return false;
  }
  else {
    uint8_t _type = root["type"];
    MsgType type = (MsgType)_type;
    switch (type) {
      case SET_ADDRESS: {
          uint8_t _addr = root["data"];                     // ** placeholder **
          address = _addr;
          EEPROM.write(0, address);
          EEPROM.commit();
          Serial.printf("[ws] <SET_ADDRESS>: %u\n", address);
        }
        break;
      case SET_MODE: {
          uint8_t _mode = root["data"];                     // ** placeholder **
          Serial.printf("[ws] <SET_MODE>: ");
          currentMode = (Mode)_mode;
          switch (currentMode) {
            case NORMAL: {
                Serial.printf("NORMAL\n");
              }
              break;
            case TEST: {
                testStepper = 0;
                Serial.printf("TEST\n");
              }
              break;
            case SLEEP: {
                Serial.printf("SLEEP\n");
              }
            break;
            default:
              Serial.printf("err: (unknown mode)\n");
              break;
          }
//          Serial.printf("[ws] <SET_MODE>: %u\n", _mode);
        }
        break;
      case CHECK_FIRMWARE: {
          uint16_t newFirmwareVersion = root["data"]["version"];
          fwUrlBase = root["data"]["url"].as<String>();
          fwFilename = root["data"]["filename"].as<String>();
          Serial.printf("[ws] <CHECK_FIRWMARE>: %s%s, current version/new version: %u/%u\n", fwUrlBase.c_str(), fwFilename.c_str(), FW_VERSION, newFirmwareVersion);

          // set flag and do check in the loop, otherwise http-client fails to connect (maybe a buffer overrun from async)
          if (newFirmwareVersion > FW_VERSION) {
            webSocket.disconnect();                 // THIS IS PROBABLY NOT NECESSARY
            checkForFW = true;
          }
          else {
            Serial.println("[OTA] Already using latest version");
          }
        }
        break;
      case CONFIG: {
          // example
          // (not 100% sure yet if/how we'll use config)
          uint8_t _newSSID = root["data"]["ssid"];          // ** placeholder **
          uint8_t _newPass = root["data"]["password"];      // ** placeholder **
          uint8_t startMode = root["data"]["startmode"];
          EEPROM[5] = startMode;
          EEPROM.commit();
          Serial.printf("[ws] <CONFIG>: %u, %u, %u\n", _newSSID, _newPass, startMode);
        }
        break;
      case REBOOT:
        Serial.printf("\n\n[ws] <REBOOT> Rebooting now!");
        ESP.restart();
        break;
      case SCAN:
        Serial.printf("[ws] <SCAN> sending REQUEST_ADDRESS\n");
        char scanJSON[jsonSendSize];
        serializeJSON_scan(scanJSON);
        webSocket.sendTXT(scanJSON);        // send to server
        break;
      default:
        Serial.printf("[ws] err: Unrecognized Message Type\n");
        break;
    }
  }
  return root.success();
}



/*---------- Websocket Event ------------*/
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_ERROR:
      Serial.printf("[WSe] ERROR!\n");
      break;
    case WStype_DISCONNECTED:
      currentMode = NORMAL;
      Serial.printf("[WSe] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
        Serial.printf("[WSe] Connected to url: %s; sending ID + Firmware\n", payload);
        char connJSON[jsonSendSize];
        serializeJSON_connected(connJSON);
        webSocket.sendTXT(connJSON);        // send to server
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSe] got text: %s\n", payload);
        deserializeJSON(payload);
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSe] got binary (length): %zu\n", length);
      hexdump(payload, length);
      break;
    default:
      break;
  }
}



#endif /* SERVER_CONTROL */
