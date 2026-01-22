#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Cấu trúc điều khiển gói tin
struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;
  unsigned SGI: 1;
  unsigned rx_end_callback_time: 32;
  unsigned snr: 8;
  unsigned: 0;
};

struct SnifferPacket {
  struct RxControl rx_ctrl;
  uint8_t data[512];
  uint16_t cnt;
  uint16_t len;
};

// Hàm callback xử lý dữ liệu khi bắt được gói tin
void sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *packet = (struct SnifferPacket *)buffer;
  
  // Kiểm tra gói tin EAPOL (Handshake - 0x888E)
  if (packet->data[32] == 0x88 && packet->data[33] == 0x8e) {
    Serial.println("\n[!] PHÁT HIỆN HANDSHAKE TRÊN KÊNH 5!");
    Serial.printf("RSSI: %d | Độ dài: %d\n", packet->rx_ctrl.rssi, length);
    
    for (int i = 0; i < length; i++) {
      Serial.printf("%02X", packet->data[i]);
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Chế độ Station
  wifi_set_opmode(STATION_MODE);
  
  // Tắt promiscuous để cấu hình
  wifi_promiscuous_enable(0);
  
  // Thiết lập hàm callback
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  
  // QUAN TRỌNG: Cố định kênh 5 tại đây
  wifi_set_channel(5); 
  
  // Bật chế độ sniffer
  wifi_promiscuous_enable(1);
  
  Serial.println("--- SNIFFER ĐANG CHẠY: CỐ ĐỊNH KÊNH 5 ---");
}

void loop() {
  // Không cần nhảy kênh, giữ trống loop hoặc dùng để duy trì hệ thống
  delay(1); 
}
