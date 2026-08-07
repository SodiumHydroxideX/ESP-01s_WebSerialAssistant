#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>

// ==================== 配置 ====================
const char *apSsid = "ESP-01S";
const char *apPassword = "12345678";

// EEPROM 布局：[0..63] STA SSID，[64..127] STA Password，[128] 有效标志(0xAB)
#define EEPROM_SIZE 256
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASS_ADDR 64
#define EEPROM_FLAG_ADDR 128
#define EEPROM_VALID_FLAG 0xAB

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);

bool waitingResponse = false;
String serialInBuffer = "";
int activeClient = -1; // 当前活跃的 WebSocket 客户端编号，-1 表示无连接
#include "index_html.h"

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                    size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] 断开连接\n", num);
    if (activeClient == (int)num) {
      activeClient = -1; // 清除活跃客户端，避免向死连接发送数据
    }
    break;
  case WStype_CONNECTED: {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] 新连接来自 %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2],
                  ip[3]);
    // 若有旧的活跃连接，先强制断开，确保只保留最新连接
    if (activeClient != -1 && activeClient != (int)num) {
      webSocket.disconnect(activeClient);
    }
    activeClient = (int)num;
    webSocket.sendTXT(num, "Connected to ESP8266");
    break;
  }
  case WStype_TEXT: {
    String cmd = String((char *)payload);
    Serial.println(cmd); // 通过串口转发给主控 MCU（带换行符）
    break;
  }
  default:
    break;
  }
}
// ==================== EEPROM 读写辅助 ====================
void eepromReadStr(int addr, char *buf, int maxLen) {
  for (int i = 0; i < maxLen - 1; i++) {
    buf[i] = EEPROM.read(addr + i);
    if (buf[i] == '\0') break;
  }
  buf[maxLen - 1] = '\0';
}

void eepromWriteStr(int addr, const char *str, int maxLen) {
  int len = (int)strlen(str);
  int end = len < maxLen - 1 ? len : maxLen - 1;
  for (int i = 0; i < end; i++)
    EEPROM.write(addr + i, str[i]);
  EEPROM.write(addr + end, '\0');
}

// ==================== HTTP 请求处理 ====================
void handleRoot() { server.send(200, "text/html", index_html); }

void handleNotFound() { server.send(404, "text/plain", "404: Not Found"); }

// GET /sta-status → JSON: {staEnabled, connected, ip, ssid}
void handleStaStatus() {
  bool hasCfg = (EEPROM.read(EEPROM_FLAG_ADDR) == EEPROM_VALID_FLAG);
  char savedSsid[64] = {};
  if (hasCfg) eepromReadStr(EEPROM_SSID_ADDR, savedSsid, 64);

  String ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  String json = "{\"staEnabled\":" + String(hasCfg ? "true" : "false") +
                ",\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") +
                ",\"ip\":\"" + ip + "\"" +
                ",\"ssid\":\"" + String(savedSsid) + "\"}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// POST /sta-connect  body: ssid=xxx&password=yyy
void handleStaConnect() {
  if (!server.hasArg("ssid")) { server.send(400, "text/plain", "missing ssid"); return; }
  String newSsid = server.arg("ssid");
  String newPass = server.arg("password");

  eepromWriteStr(EEPROM_SSID_ADDR, newSsid.c_str(), 64);
  eepromWriteStr(EEPROM_PASS_ADDR, newPass.c_str(), 64);
  EEPROM.write(EEPROM_FLAG_ADDR, EEPROM_VALID_FLAG);
  EEPROM.commit();

  WiFi.begin(newSsid.c_str(), newPass.c_str());
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}

// POST /sta-forget  清除已保存凭据并断开 STA
void handleStaForget() {
  EEPROM.write(EEPROM_FLAG_ADDR, 0x00);
  EEPROM.commit();
  WiFi.disconnect(false);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}

// ==================== 初始化 ====================
void setup() {
  Serial.begin(115200);
  Serial.println();

  EEPROM.begin(EEPROM_SIZE);

  // AP_STA 混合模式：始终开启热点，同时尝试连接已保存的 STA
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSsid, apPassword);

  if (EEPROM.read(EEPROM_FLAG_ADDR) == EEPROM_VALID_FLAG) {
    char savedSsid[64] = {}, savedPass[64] = {};
    eepromReadStr(EEPROM_SSID_ADDR, savedSsid, 64);
    eepromReadStr(EEPROM_PASS_ADDR, savedPass, 64);
    WiFi.begin(savedSsid, savedPass);
    Serial.printf("正在连接 STA: %s\n", savedSsid);
  }

  // 启动 HTTP 服务
  server.on("/", handleRoot);
  server.on("/sta-status", HTTP_GET, handleStaStatus);
  server.on("/sta-connect", HTTP_POST, handleStaConnect);
  server.on("/sta-forget", HTTP_POST, handleStaForget);
  server.onNotFound(handleNotFound);
  server.begin();

  // 启动 WebSocket 服务
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  // 启用心跳：每 5s 发一次 ping，等待 pong 超时 3s，超时后断开死连接
  webSocket.enableHeartbeat(5000, 3000, 2);

  Serial.println("ESP-01S Init!");
}

// ==================== 主循环 ====================
void loop() {
  server.handleClient();
  webSocket.loop();
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (serialInBuffer.length() > 0 && activeClient != -1) {
        // 只向当前活跃客户端发送，避免向死连接写入导致 TCP 阻塞
        webSocket.sendTXT(activeClient, serialInBuffer);
      }
      serialInBuffer = "";
    } else if (c == '\r') {
      continue;
    } else {
      serialInBuffer += c;
    }
  }
}
