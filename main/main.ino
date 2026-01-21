#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}

// Cấu hình mạng mục tiêu để lọc sạch Serial
const uint8_t target_mac[] = {0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8}; 

void sniffer_callback(uint8_t *buf, uint16_t len) {
  // byte 12 là Frame Control
  uint8_t frame_type = (buf[12] & 0x0C) >> 2;
  uint8_t frame_subtype = (buf[12] & 0xF0) >> 4;

  bool is_eapol = false;
  
  // 1. Kiểm tra gói EAPOL (Handshake)
  if (len > 34) { // Gói Handshake thường dài
    for (int i = 0; i < len - 1; i++) {
      if (buf[i] == 0x88 && buf[i+1] == 0x8E) {
        is_eapol = true;
        break;
      }
    }
  }

  // 2. Kiểm tra gói Beacon từ đúng MAC Hoang Phat
  bool is_target_beacon = false;
  if (frame_type == 0 && frame_subtype == 8) {
    if (memcmp(&buf[22], target_mac, 6) == 0) { // Byte 22-27 thường là BSSID
      is_target_beacon = true;
    }
  }

  // Chỉ in khi là Handshake hoặc Beacon của nhà mình
  if (is_eapol || is_target_beacon) {
    Serial.print("PKT:");
    for (int i = 0; i < len; i++) {
      if (buf[i] < 0x10) Serial.print("0");
      Serial.print(buf[i], HEX);
    }
    Serial.println();
  }
  
  // Tránh lỗi Watchdog Timer
  yield(); 
}

void setup() {
  // Giảm công suất Wi-Fi để tránh sụt áp gây treo chip
  system_phy_set_max_tpw(50); 
  
  Serial.begin(115200);
  delay(2000); // Đợi Serial ổn định
  Serial.println("\n[OK] KHOI DONG SNIFFER - KENH 5");

  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  
  // Cố định kênh 5
  wifi_set_channel(5); 
  wifi_promiscuous_enable(1);
}

void loop() {
  // Nuôi Watchdog để không bị Reset máy
  yield();
  delay(1);
}
