#ifndef SERVER_CONTROL_H
#define SERVER_CONTROL_H

/*
  websocket terminal client:
  > npm install -g wscat
  > wscat -c <serverIP>:8080
*/

#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <Hash.h>

WebSocketsClient webSocket;


// eg. message types in server
//#define MSG_TYPE_SET_ADDRESS      0
//#define MSG_TYPE_SET_MODE         1
//#define MSG_TYPE_CHECK_FIRMWARE   2
//#define MSG_TYPE_CONFIG           3
//#define MSG_TYPE_REBOOT           4
//#define MSG_TYPE_REQUEST_ADDRESS  5

enum MsgType : uint8_t  {
  SET_ADDRESS = 0,
  SET_MODE,
  CHECK_FIRMWARE,
  CONFIG,
  REBOOT,
  REQUEST_ADDRESS             // (send only) = send MAC to server for address request
};


//TODO: periodically attempt to reconnect to server if down. Use websocket ping/pong?
//      I think websocket library does this automagically
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
          Serial.printf("[ws] <SET_ADDRESS>: %u\n", _addr);
        }
        break;
      case SET_MODE: {
          uint8_t _mode = root["data"];                     // ** placeholder **
          Serial.printf("[ws] <SET_MODE>: %u\n", _mode);
        }
        break;
      case CHECK_FIRMWARE: {
          uint16_t newFirmwareVersion = root["data"]["version"];
          String fwUrlBase = root["data"]["url"].as<String>();
          String fwName = root["data"]["filename"].as<String>();
          Serial.printf("[ws] <CHECK_FIRWMARE>: %s%s, new version: %u\n", fwUrlBase.c_str(), fwName.c_str(), newFirmwareVersion);
          checkForNewFirmware(newFirmwareVersion, fwUrlBase, fwName);
          // don't do too much in here, MAYBE better just to set flags and check in loop
        }
        break;
      case CONFIG: {
          // example
          // (not 100% sure yet if/how we'll use config)
          uint8_t _newSSID = root["data"]["ssid"];          // ** placeholder **
          uint8_t _newPass = root["data"]["password"];      // ** placeholder **
          Serial.printf("[ws] <CONFIG>: %u, %u\n", _newSSID, _newPass);
        }
        break;
      case REBOOT:
        Serial.printf("\n\n[ws] <REBOOT> Rebooting now!");
        ESP.restart();
        break;
      default:
        Serial.printf("[ws] err: Unrecognized Message Type\n");
        break;
    }
  }
  return root.success();
}

const uint16_t jsonSendSize = 256;
void serializeJSON_connected(char * json) {
  StaticJsonBuffer<jsonSendSize> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
//  root["type"] = MSG_TYPE_CONNECT_INFO;
//  // JsonObject& data = root.createNestedObject("data");
//  // data["id"] = address;
//  // data["firmwareVersion"] = FW_VERSION;
//  root["id"] = address;
//  root["firmwareVersion"] = FW_VERSION;
  root.printTo(json, jsonSendSize);
  Serial.println(json);
}



/*---------- Websocket Event ------------*/
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_ERROR:
      Serial.printf("[WSe] ERROR!\n");
      break;
    case WStype_DISCONNECTED:
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
