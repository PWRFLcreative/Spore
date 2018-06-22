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
extern const bool SERIAL_DEBUG;


// eg. message types in server
//#define MSG_TYPE_SENSOR_STATE    0
//#define MSG_TYPE_TEST_MODE       1
//#define MSG_TYPE_CHECK_FIRMWARE  2
//#define MSG_TYPE_CALIBRATE       3
//#define MSG_TYPE_CONNECT_INFO    4

enum MsgType : uint8_t  {
  SET_ADDRESS    = 0,
  GET_ADDRESS,             // (send only) = send MAC to server for address request
  SET_MODE,
  CHECK_FIRMWARE,
  CONFIG,
  REBOOT
};


//TODO: periodically attempt to reconnect to server if down. Use websocket ping/pong?
//      I think websocket library does this automagically
bool deserializeJSON(uint8_t * json) {
  StaticJsonBuffer<512> jsonBuffer;     // actually only needs to be about 240 bytes, extra reserved for future use (or just because I have the space..). (http://arduinojson.org/assistant/)
  JsonObject& root = jsonBuffer.parseObject(json);
  if (!root.success()) {
    if (SERIAL_DEBUG) Serial.printf("parseObject() failed\n");
    return false;
  }
  else {
    uint8_t _type = root["type"];
    MsgType type = (MsgType)_type;
    switch(type){
      case SET_ADDRESS: 
        
        break;
      case SET_MODE: 
      
        break;
      case TEST_MODE: 
      
        break;
      case CHECK_FIRMWARE: {
          const char* exampleNestedValue = root["object"]["key"];
          const char* exampleValue = root["key"];
          Serial.printf("[CHECK FIRMWARE] %s%s\n", fwUrlBase.c_str(), fwName.c_str());
          // don't do too much in here, better just to set flags
        }
        break;
      default:
        break;
    }
  }
  return root.success();
}

const uint16_t jsonSendSize = 256;
void serializeJSON_connected(char * json) {
    StaticJsonBuffer<jsonSendSize> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["type"] = MSG_TYPE_CONNECT_INFO;
    // JsonObject& data = root.createNestedObject("data");
    // data["id"] = address;
    // data["firmwareVersion"] = FW_VERSION;
    root["id"] = address;
    root["firmwareVersion"] = FW_VERSION;
    root.printTo(json, jsonSendSize);
    if (SERIAL_DEBUG) Serial.println(json);
}



/*---------- Websocket Event ------------*/
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_ERROR:
      if (SERIAL_DEBUG) Serial.printf("[WSc] ERROR!\n");
      break;
    case WStype_DISCONNECTED:
      if (SERIAL_DEBUG) Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
      if (SERIAL_DEBUG) Serial.printf("[WSc] Connected to url: %s; sending ID + Firmware\n", payload);
      char connJSON[jsonSendSize];
      serializeJSON_connected(connJSON);
      webSocket.sendTXT(connJSON);    // send to server
    }
      break;
    case WStype_TEXT: {
        if (SERIAL_DEBUG) Serial.printf("[WSc] got text: %s\n", payload);
        deserializeJSON(payload);
      }
      break;
    case WStype_BIN:
      if (SERIAL_DEBUG) Serial.printf("[WSc] got binary (length): %zu\n", length);
      hexdump(payload, length);
      break;
    default:
      break;
  }
}



#endif /* SERVER_CONTROL */
