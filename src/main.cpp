#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "esp_bt.h"

const char* ssid = "ESP32Chat";
const char* password = "12341234";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

DNSServer dnsServer;
const byte DNS_PORT = 53;

struct User {
  uint32_t clientId;
  String username;
};

std::vector<User> onlineUsers;

#define MAX_MESSAGES 50
struct Message {
  String username;
  String text;
  unsigned long timestamp;
};
std::vector<Message> messageBuffer;

const char* chatLogFile = "/chatlog.txt";
#define MAX_LOG_SIZE 10240

void initSPIFFS();
void initWiFi();
void initWebServer();
void initWebSocket();
void onWiFiEvent(WiFiEvent_t event);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
void broadcastMessage(const String& message);
void broadcastUserList();
void addUserToList(uint32_t clientId, const String& username);
void removeUserFromList(uint32_t clientId);
String getUsernameById(uint32_t clientId);
void saveMessageToSPIFFS(const String& username, const String& text);
void trimLogFile();
String getFormattedTime();

void setup() {
  WiFi.onEvent(onWiFiEvent);
  initSPIFFS();
  initWiFi();
  initWebSocket();
  initWebServer();
}

void loop() {
  dnsServer.processNextRequest();
  ws.cleanupClients();
  delay(10);
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    return;
  }
  
  if (!SPIFFS.exists(chatLogFile)) {
    File file = SPIFFS.open(chatLogFile, FILE_WRITE);
    if (file) {
      file.println("=== Chat Log Started ===");
      file.close();
    }
  }
}

void onWiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      dnsServer.stop();
      break;
    default:
      break;
  }
}

void initWiFi() {
  btStop();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_AP);
  delay(100);
  WiFi.setSleep(false);
  WiFi.persistent(true);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password, 1, false, 10);
  delay(500);
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void initWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });
  
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (SPIFFS.exists(chatLogFile)) {
      request->send(SPIFFS, chatLogFile, "text/plain");
    } else {
      request->send(200, "text/plain", "No chat history");
    }
  });
  
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });
  
  server.begin();
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      break;
    case WS_EVT_DISCONNECT:
      {
        String username = getUsernameById(client->id());
        if (username.length() > 0) {
          removeUserFromList(client->id());
          StaticJsonDocument<200> doc;
          doc["type"] = "system";
          doc["text"] = username + " left";
          String output;
          serializeJson(doc, output);
          broadcastMessage(output);
          broadcastUserList();
        }
      }
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client->id());
      break;
    default:
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      return;
    }
    
    String type = doc["type"].as<String>();
    
    if (type == "join") {
      String username = doc["username"].as<String>();
      addUserToList(clientId, username);
      
      StaticJsonDocument<200> sysDoc;
      sysDoc["type"] = "system";
      sysDoc["text"] = username + " joined";
      
      String output;
      serializeJson(sysDoc, output);
      broadcastMessage(output);
      broadcastUserList();
      
    } else if (type == "message") {
      String username = doc["username"].as<String>();
      String text = doc["text"].as<String>();
      
      saveMessageToSPIFFS(username, text);
      
      StaticJsonDocument<512> msgDoc;
      msgDoc["type"] = "message";
      msgDoc["username"] = username;
      msgDoc["text"] = text;
      
      String output;
      serializeJson(msgDoc, output);
      broadcastMessage(output);
    }
  }
}

void broadcastMessage(const String& message) {
  ws.textAll(message);
}

void broadcastUserList() {
  StaticJsonDocument<1024> doc;
  doc["type"] = "userlist";
  JsonArray users = doc.createNestedArray("users");
  
  for (const auto& user : onlineUsers) {
    users.add(user.username);
  }
  
  String output;
  serializeJson(doc, output);
  ws.textAll(output);
}

void addUserToList(uint32_t clientId, const String& username) {
  for (auto& user : onlineUsers) {
    if (user.clientId == clientId) {
      user.username = username;
      return;
    }
  }
  
  User newUser;
  newUser.clientId = clientId;
  newUser.username = username;
  onlineUsers.push_back(newUser);
}

void removeUserFromList(uint32_t clientId) {
  for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
    if (it->clientId == clientId) {
      onlineUsers.erase(it);
      return;
    }
  }
}

String getUsernameById(uint32_t clientId) {
  for (const auto& user : onlineUsers) {
    if (user.clientId == clientId) {
      return user.username;
    }
  }
  return "";
}

void saveMessageToSPIFFS(const String& username, const String& text) {
  File file = SPIFFS.open(chatLogFile, FILE_READ);
  if (file) {
    size_t fileSize = file.size();
    file.close();
    if (fileSize > MAX_LOG_SIZE) {
      trimLogFile();
    }
  }
  
  file = SPIFFS.open(chatLogFile, FILE_APPEND);
  if (file) {
    String timestamp = getFormattedTime();
    file.printf("[%s] %s: %s\n", timestamp.c_str(), username.c_str(), text.c_str());
    file.close();
    
    Message msg;
    msg.username = username;
    msg.text = text;
    msg.timestamp = millis();
    messageBuffer.push_back(msg);
    
    if (messageBuffer.size() > MAX_MESSAGES) {
      messageBuffer.erase(messageBuffer.begin());
    }
  }
}

void trimLogFile() {
  File file = SPIFFS.open(chatLogFile, FILE_READ);
  if (!file) {
    return;
  }
  
  std::vector<String> lines;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    lines.push_back(line);
  }
  file.close();
  
  size_t keepFrom = lines.size() / 2;
  
  file = SPIFFS.open(chatLogFile, FILE_WRITE);
  if (file) {
    file.println("=== Log trimmed ===");
    for (size_t i = keepFrom; i < lines.size(); i++) {
      file.println(lines[i]);
    }
    file.close();
  }
}

String getFormattedTime() {
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  char timeStr[20];
  sprintf(timeStr, "%02lu:%02lu:%02lu", hours % 24, minutes % 60, seconds % 60);
  return String(timeStr);
}
