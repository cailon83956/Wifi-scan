#include <ESP8266WiFi.h>

extern "C" {
#include "user_interface.h"
}

// Cấu trúc lọc gói tin cực nhẹ
void sniffer_callback(uint8_t *buf, uint16_t len) {
  // Chỉ kiểm tra byte 12 và 13 để tìm Handshake (EAPOL)
  // Việc kiểm tra ít giúp CPU không bị treo
  if (len > 34) {
    for (int i = 0; i < len - 1; i++) {
      if (buf[i] == 0x88 && buf[i+1] == 0x8E) {
        Serial.print("PKT:");
        for (int j = 0; j < len; j++) {
          if (buf[j] < 0x10) Serial.print("0");
          Serial.print(buf[j], HEX);
        }
        Serial.println();
        break;
      }
    }
  }
  // Lệnh quan trọng để không bị WDT Reset
  yield(); 
}

void setup() {
  // Hạ công suất phát Wi-Fi xuống mức thấp để tiết kiệm điện, tránh sụt áp
  system_phy_set_max_tpw(40); 
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[OK] SNIFFER DA KICH HOAT - KENH 5");

  WiFi.disconnect();
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_set_channel(5); // Cố định kênh 5 của mạng mục tiêu
  wifi_promiscuous_enable(1);
}

void loop() {
  // Luôn luôn nhường quyền cho hệ thống
  yield();
  delay(1);
}
