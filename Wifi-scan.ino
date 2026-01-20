#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}

// Cấu hình kênh quét (Channel)
int channel = 1;

// Hàm xử lý khi bắt được gói tin thô (Raw Packet)
void sniffer_callback(uint8_t *buffer, uint16_t length) {
  // Gửi toàn bộ dữ liệu thô qua Serial để Python trên Termux hứng vào ổ 1TB
  for (int i = 0; i < length; i++) {
    Serial.printf("%02x", buffer[i]);
  }
  Serial.println(); // Ngắt dòng để script Python dễ lọc
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);
  
  Serial.println("\n[SYSTEM] Sniffer Mode Started. Channel: " + String(channel));
}

void loop() {
  // Tự động nhảy kênh sau mỗi 5 giây để quét toàn bộ thiết bị
  channel++;
  if (channel > 13) channel = 1;
  wifi_set_channel(channel);
  delay(5000); 
}
