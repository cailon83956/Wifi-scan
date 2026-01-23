#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Hàm callback xử lý gói tin bắt được
void sniffer_callback(uint8_t *buf, uint16_t len) {
  if (len == 0) return;

  // Cấu trúc gói tin thô (raw packet)
  // buf[0] thường chứa thông tin về loại gói tin (Frame Control)
  
  Serial.print("Packet length: ");
  Serial.print(len);
  
  // In 6 byte địa chỉ MAC nguồn (thường nằm ở offset 10 trong gói tin 802.11)
  if (len > 16) {
    Serial.print(" | SRC MAC: ");
    for (int i = 10; i < 16; i++) {
      Serial.printf("%02X", buf[i]);
      if (i < 15) Serial.print(":");
    }
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 1. Chuyển sang chế độ Station nhưng không kết nối vào mạng nào
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // 2. Kích hoạt chế độ Promiscuous (Monitor Mode)
  wifi_promiscuous_enable(0); // Tắt trước khi cài đặt
  wifi_set_promiscuous_rx_cb(sniffer_callback); // Đặt hàm xử lý
  wifi_promiscuous_enable(1); // Bật lại

  // 3. Cố định kênh 5
  wifi_set_channel(5);
  
  Serial.println("--- Đang Monitor trên Kênh 5 ---");
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
}

void loop() {
  // Không cần viết gì ở loop vì hàm sniffer_callback sẽ tự chạy khi có gói tin
  delay(1); 
}
