#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include "esp_bt.h"

// WiFi AP Configuration
const char* ssid = "ESP32Chat";  // Tên ngắn hơn, đơn giản hơn
const char* password = "";  // Không dùng password để tránh lỗi xác thực

// Web Server và WebSocket
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");  // WebSocket on same port 80, path /ws

// DNS Server for captive portal
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Cấu trúc lưu thông tin người dùng
struct User {
  uint32_t clientId;
  String username;
};

// Danh sách người dùng online
std::vector<User> onlineUsers;

// Message buffer để tránh overload SPIFFS
#define MAX_MESSAGES 50
struct Message {
  String username;
  String text;
  unsigned long timestamp;
};
std::vector<Message> messageBuffer;

// Chat log file path
const char* chatLogFile = "/chatlog.txt";

// Giới hạn kích thước file log (10KB)
#define MAX_LOG_SIZE 10240

// Function prototypes
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
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n=== ESP32 Chat Room Starting ===");
  
  // Đăng ký WiFi event handler
  WiFi.onEvent(onWiFiEvent);
  
  // Khởi tạo SPIFFS
  initSPIFFS();
  
  // Khởi tạo WiFi AP
  initWiFi();
  
  // Khởi tạo WebSocket
  initWebSocket();
  
  // Khởi tạo Web Server
  initWebServer();
  
  Serial.println("=== Setup Complete ===");
  Serial.print("Connect to WiFi: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("Open browser and go to: http://");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // Xử lý DNS requests (quan trọng cho kết nối ổn định)
  dnsServer.processNextRequest();
  
  // Kiểm tra và khởi động lại AP nếu bị tắt (failsafe)
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 10000) {  // Kiểm tra mỗi 10s
    lastCheck = millis();
    
    if (WiFi.getMode() != WIFI_AP) {
      Serial.println("[ERROR] WiFi AP stopped! Restarting...");
      initWiFi();
    } else {
      // In thông tin trạng thái
      Serial.printf("[OK] WiFi: %d clients, WebSocket: %d clients, Free Heap: %d bytes\n", 
                    WiFi.softAPgetStationNum(), ws.count(), ESP.getFreeHeap());
    }
  }
  
  // Cleanup WebSocket clients không active
  ws.cleanupClients();
  
  delay(10);
}

void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  
  // Kiểm tra và tạo file log nếu chưa có
  if (!SPIFFS.exists(chatLogFile)) {
    File file = SPIFFS.open(chatLogFile, FILE_WRITE);
    if (file) {
      file.println("=== Chat Log Started ===");
      file.close();
      Serial.println("Created new chat log file");
    }
  }
  
  // Hiển thị thông tin SPIFFS
  Serial.printf("SPIFFS Total: %d bytes\n", SPIFFS.totalBytes());
  Serial.printf("SPIFFS Used: %d bytes\n", SPIFFS.usedBytes());
}

void onWiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("[WiFi] AP Started");
      Serial.println("[WiFi] Starting DNS Server...");
      dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("[WiFi] AP Stopped");
      dnsServer.stop();
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("[WiFi] ✓ Client Connected");
      Serial.printf("[WiFi] Total clients: %d\n", WiFi.softAPgetStationNum());
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("[WiFi] ✗ Client Disconnected");
      Serial.printf("[WiFi] Total clients: %d\n", WiFi.softAPgetStationNum());
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("[WiFi] ✓ Client IP Assigned");
      break;
    default:
      break;
  }
}

void initWiFi() {
  // Tắt Bluetooth để tránh xung đột với WiFi (cùng băng tần 2.4GHz)
  btStop();
  esp_bt_controller_deinit();
  esp_bt_mem_release(ESP_BT_MODE_BTDM);
  
  // Tắt tất cả WiFi trước để reset hoàn toàn
  WiFi.mode(WIFI_OFF);
  delay(500);
  
  // Cấu hình WiFi mode
  WiFi.mode(WIFI_AP);
  delay(100);
  
  // Tắt power save để tăng độ ổn định
  WiFi.setSleep(false);
  WiFi.persistent(true);  // Lưu cấu hình vào flash
  
  // Đẳt TX power THẤP NHẤT để ổn định tối đa (khoảng cách ngắn hơn nhưng ổn định)
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  // Công suất thấp nhất
  
  // Cấu hình IP tĩnh cho AP
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
    Serial.println("[ERROR] Failed to configure AP");
    return;
  }
  
  // Khởi động Access Point với các tham số đầy đủ
  // WiFi.softAP(ssid, password, channel, hidden, max_connection)
  // Channel 1 = đơn giản, ít xử lý, ổn định hơn
  // Max 4 clients để tránh quá tải
  bool apStarted = WiFi.softAP(ssid, password, 1, false, 4);
  
  if (apStarted) {
    Serial.println("[OK] Access Point started successfully");
  } else {
    Serial.println("[ERROR] Failed to start Access Point!");
    return;
  }
  
  delay(500);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("\n=== WiFi AP Information ===");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(IP);
  Serial.print("MAC Address: ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.println("Channel: 1");
  Serial.println("Max Connections: 4");
  Serial.println("TX Power: 8.5dBm (LOW - Most Stable)");
  Serial.println("Bluetooth: DISABLED");
  Serial.println("============================\n");
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  Serial.println("WebSocket initialized at path /ws (port 80)");
}

void initWebServer() {
  // Serve static files from SPIFFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET / - Serving index.html");
    request->send(SPIFFS, "/index.html", "text/html");
  });
  
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /style.css");
    request->send(SPIFFS, "/style.css", "text/css");
  });
  
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /script.js");
    request->send(SPIFFS, "/script.js", "text/javascript");
  });
  
  // API để lấy lịch sử chat (tùy chọn)
  server.on("/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[HTTP] GET /history");
    if (SPIFFS.exists(chatLogFile)) {
      request->send(SPIFFS, chatLogFile, "text/plain");
    } else {
      request->send(200, "text/plain", "No chat history");
    }
  });
  
  // Captive Portal - redirect tất cả request không tìm thấy về trang chủ
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP] NOT FOUND: %s - Redirecting to /\n", request->url().c_str());
    request->redirect("/");
  });
  
  server.begin();
  Serial.println("HTTP server started on port 80");
  Serial.println("WebSocket available at ws://192.168.4.1:80/ws");
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  Serial.printf("[WebSocket] Event type: %d from client #%u\n", type, client->id());
  
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("[WebSocket] ✓ Client #%u CONNECTED from %s\n", client->id(), client->remoteIP().toString().c_str());
      Serial.printf("[WebSocket] Total clients: %d\n", ws.count());
      break;
      
    case WS_EVT_DISCONNECT:
      Serial.printf("[WebSocket] ✗ Client #%u DISCONNECTED\n", client->id());
      {
        String username = getUsernameById(client->id());
        if (username.length() > 0) {
          removeUserFromList(client->id());
          
          // Broadcast system message
          StaticJsonDocument<200> doc;
          doc["type"] = "system";
          doc["text"] = username + " đã rời khỏi phòng";
          
          String output;
          serializeJson(doc, output);
          broadcastMessage(output);
          
          broadcastUserList();
        }
      }
      break;
      
    case WS_EVT_DATA:
      Serial.printf("[WebSocket] DATA received from client #%u, length: %d\n", client->id(), len);
      handleWebSocketMessage(arg, data, len, client->id());
      break;
      
    case WS_EVT_PONG:
      Serial.printf("[WebSocket] PONG from client #%u\n", client->id());
      break;
    case WS_EVT_ERROR:
      Serial.printf("[WebSocket] ERROR from client #%u\n", client->id());
      break;
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    Serial.println("[DEBUG] Received message: " + message);
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.println("JSON parse error");
      return;
    }
    
    String type = doc["type"].as<String>();
    
    if (type == "join") {
      String username = doc["username"].as<String>();
      
      addUserToList(clientId, username);
      
      // Broadcast system message
      StaticJsonDocument<200> sysDoc;
      sysDoc["type"] = "system";
      sysDoc["text"] = username + " đã tham gia phòng";
      
      String output;
      serializeJson(sysDoc, output);
      broadcastMessage(output);
      
      broadcastUserList();
      
      Serial.printf("User '%s' joined (client #%u)\n", username.c_str(), clientId);
      
    } else if (type == "message") {
      String username = doc["username"].as<String>();
      String text = doc["text"].as<String>();
      
      // Lưu tin nhắn vào SPIFFS
      saveMessageToSPIFFS(username, text);
      
      // Broadcast tin nhắn đến tất cả clients
      StaticJsonDocument<512> msgDoc;
      msgDoc["type"] = "message";
      msgDoc["username"] = username;
      msgDoc["text"] = text;
      
      String output;
      serializeJson(msgDoc, output);
      broadcastMessage(output);
      
      Serial.printf("[%s]: %s\n", username.c_str(), text.c_str());
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
  // Kiểm tra xem user đã tồn tại chưa
  for (auto& user : onlineUsers) {
    if (user.clientId == clientId) {
      user.username = username;
      return;
    }
  }
  
  // Thêm user mới
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
  // Kiểm tra kích thước file trước khi ghi
  File file = SPIFFS.open(chatLogFile, FILE_READ);
  if (file) {
    size_t fileSize = file.size();
    file.close();
    
    // Nếu file quá lớn, trim nó
    if (fileSize > MAX_LOG_SIZE) {
      Serial.println("Log file too large, trimming...");
      trimLogFile();
    }
  }
  
  // Ghi tin nhắn mới
  file = SPIFFS.open(chatLogFile, FILE_APPEND);
  if (file) {
    String timestamp = getFormattedTime();
    file.printf("[%s] %s: %s\n", timestamp.c_str(), username.c_str(), text.c_str());
    file.close();
    
    // Thêm vào buffer
    Message msg;
    msg.username = username;
    msg.text = text;
    msg.timestamp = millis();
    messageBuffer.push_back(msg);
    
    // Giới hạn buffer
    if (messageBuffer.size() > MAX_MESSAGES) {
      messageBuffer.erase(messageBuffer.begin());
    }
  } else {
    Serial.println("Failed to open log file for writing");
  }
}

void trimLogFile() {
  // Đọc file hiện tại
  File file = SPIFFS.open(chatLogFile, FILE_READ);
  if (!file) {
    return;
  }
  
  String content = "";
  std::vector<String> lines;
  
  while (file.available()) {
    String line = file.readStringUntil('\n');
    lines.push_back(line);
  }
  file.close();
  
  // Giữ lại 50% số dòng cuối
  size_t keepFrom = lines.size() / 2;
  
  // Ghi đè file với nội dung mới
  file = SPIFFS.open(chatLogFile, FILE_WRITE);
  if (file) {
    file.println("=== Log trimmed ===");
    for (size_t i = keepFrom; i < lines.size(); i++) {
      file.println(lines[i]);
    }
    file.close();
    Serial.println("Log file trimmed successfully");
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
