#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}

// Hàm xử lý gói tin bắt được
void sniffer_callback(uint8_t *buf, uint16_t len) {
  // Gói tin thô của ESP8266 bắt đầu từ byte thứ 12
  uint8_t frame_type = (buf[12] & 0x0C) >> 2;
  uint8_t frame_subtype = (buf[12] & 0xF0) >> 4;

  bool is_eapol = false;
  
  // Lọc gói tin Handshake (EAPOL)
  if (frame_type == 2 || frame_subtype == 8) {
    for (int i = 0; i < len - 1; i++) {
      if (buf[i] == 0x88 && buf[i+1] == 0x8E) {
        is_eapol = true;
        break;
      }
    }
  }

  // Lọc gói tin Beacon (Để lấy tên Wi-Fi)
  bool is_beacon = (frame_type == 0 && frame_subtype == 8);

  // Chỉ in ra Serial nếu là Handshake hoặc Beacon
  if (is_eapol || is_beacon) {
    Serial.print("PKT: ");
    for (int i = 0; i < len; i++) {
      if (buf[i] < 0x10) Serial.print("0");
      Serial.print(buf[i], HEX);
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[SYSTEM] KHOI DONG SNIFFER - KENH 5");

  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
  
  // CHỈNH KÊNH TẠI ĐÂY: Sửa thành 5 cho mạng Hoang Phat
  wifi_set_channel(5); 
}

void loop() {
  // Để trống để chip tập trung bắt gói tin ở kênh 5 liên tục
  delay(10);
}
