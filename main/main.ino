#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// MAC Hoang Phat từ ảnh của bạn
uint8_t target_mac[6] = {0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8};

void sniffer_callback(uint8_t *buf, uint16_t len) {
  // Bỏ qua 12 byte tiêu đề điều khiển để lấy khung hình Wi-Fi thô
  uint8_t *frame = buf + 12;
  uint16_t frame_len = len - 12;

  // Lọc đúng MAC Hoang Phat (Source MAC hoặc BSSID)
  if (memcmp(frame + 10, target_mac, 6) == 0 || memcmp(frame + 16, target_mac, 6) == 0) {
    
    // Kiểm tra xem có phải Handshake (EAPOL) không
    bool isEAPOL = (frame[32] == 0x88 && frame[33] == 0x8E) || (frame[34] == 0x88 && frame[35] == 0x8E);

    if (isEAPOL) {
      Serial.println("\n[!!!] PHÁT HIỆN HANDSHAKE (EAPOL) [!!!]");
    }

    // IN RÕ TỪNG SỐ VÀ CHỮ (HEX DUMP) - KHÔNG CẮT NGẮN
    Serial.printf("RSSI:%d | Len:%d | DATA: ", (signed char)buf[0], frame_len);
    
    for (int i = 0; i < frame_len; i++) {
      Serial.printf("%02X ", frame[i]); // Hiện rõ từng cặp số Hex
    }
    
    Serial.println("\n------------------------------------------------");
  }
}

void setup() {
  // Tốc độ 115200 để Serial USB Terminal đọc mượt mà
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
  
  // Cố định Kênh 5 theo đúng ảnh Net Analyzer
  wifi_set_channel(5); 

  Serial.println("\n--- SNIFFER FULL DATA: HOANG PHAT ---");
  Serial.println("Dữ liệu đang được đổ vào Serial USB Terminal...");
}

void loop() {
  // Chạy ở xung nhịp cao nhất để không mất gói tin
  delay(1); 
}
