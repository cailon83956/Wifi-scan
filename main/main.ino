#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Cấu trúc gói tin từ SDK
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

// Hàm xử lý khi bắt được BẤT KỲ gói tin nào
void sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct SnifferPacket *packet = (struct SnifferPacket *)buffer;
  
  // Lấy loại gói tin (Frame Control)
  uint8_t frameType = packet->data[0];
  uint8_t frameSubType = (frameType & 0xF0) >> 4;
  
  // In thông tin tóm tắt để không làm treo chip
  Serial.printf("[Ch:5][RSSI:%4d][Len:%4d] Type: 0x%02X | Sub: 0x%X ", 
                packet->rx_ctrl.rssi, length, frameType, frameSubType);

  // Nhận diện nhanh một số loại gói quan trọng
  if (frameType == 0x80) Serial.print("-> BEACON");
  if (packet->data[32] == 0x88 && packet->data[33] == 0x8e) Serial.print(" -> !!! HANDSHAKE !!!");
  
  Serial.println();

  /* // NẾU MUỐN XEM FULL HEX (Cẩn thận: Dễ gây treo nếu có quá nhiều wifi xung quanh)
  for (int i = 0; i < length; i++) {
    Serial.printf("%02X ", packet->data[i]);
  }
  Serial.println();
  */
}

void setup() {
  // Tăng tốc độ Serial lên mức tối đa để tránh nghẽn dữ liệu
  Serial.begin(230400); 
  delay(500);

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  
  // Cố định kênh 5
  wifi_set_channel(5); 
  
  wifi_promiscuous_enable(1);
  
  Serial.println("\n--- ĐANG QUÉT TOÀN BỘ GÓI TIN TRÊN KÊNH 5 ---");
}

void loop() {
  // Để trống để ưu tiên tài nguyên cho Sniffer
  delay(1);
}
