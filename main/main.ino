#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"  // Cần cho promiscuous
}

#define FIXED_CHANNEL     5          // Channel cố định 5
#define SERIAL_BAUD       115200
#define MAX_DUMP_BYTES    250        // Dump càng nhiều bytes càng tốt (tăng cơ hội thấy full EAPOL)

static uint32_t eapol_count = 0;     // Đếm số EAPOL bắt được

// Callback promiscuous - gọi mỗi khi có gói tin
static void ICACHE_FLASH_ATTR promisc_cb(uint8_t *buf, uint16_t len) {
  if (len < 50) return;  // Bỏ gói quá ngắn

  // RSSI signed (thường byte 1 hoặc tùy SDK)
  int8_t rssi = (int8_t)buf[1];

  // Tìm signature EAPOL: 0x88 0x8E (EtherType)
  for (uint16_t i = 10; i < len - 4; i++) {  // Bắt đầu sau header rx_ctrl (~12 bytes)
    if (buf[i] == 0x88 && buf[i + 1] == 0x8E) {
      eapol_count++;
      
      uint16_t eapol_start = i;
      uint16_t eapol_len_approx = len - eapol_start;

      Serial.printf("\n[%u] === EAPOL #%u DETECTED! Len total=%u | EAPOL approx=%u | RSSI=%d dBm | Ch=%d\n",
                    millis() / 1000, eapol_count, len, eapol_len_approx, rssi, FIXED_CHANNEL);

      // In hex dump dài nhất có thể
      Serial.print("Full hex dump (first ");
      Serial.print(MAX_DUMP_BYTES);
      Serial.print(" bytes): ");

      uint16_t dump_end = min((uint16_t)MAX_DUMP_BYTES, len);
      for (uint16_t j = 0; j < dump_end; j++) {
        if (buf[j] < 16) Serial.print("0");
        Serial.print(buf[j], HEX);
        Serial.print(" ");
        if ((j + 1) % 16 == 0) Serial.print("  ");  // Ngắt dòng cho dễ đọc
      }
      Serial.println();

      // Optional: Check xem có vẻ full Message 2/3/4 không (thường >95 bytes)
      if (eapol_len_approx > 95) {
        Serial.println("!!! POTENTIAL FULL HANDSHAKE MESSAGE !!! (len >95 → có thể crack được)");
      }

      // Dừng sớm nếu chỉ cần 1 handshake (comment nếu muốn capture nhiều)
      // return;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);
  Serial.println("\n=====================================");
  Serial.println("ESP8266 MAX STRENGTH Sniffer - Channel 5 ONLY");
  Serial.println("Tối ưu capture EAPOL WPA handshake");
  Serial.println("=====================================\n");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  wifi_set_opmode(STATION_MODE);

  // Tắt promiscuous trước để reset
  wifi_promiscuous_enable(0);
  delay(50);

  // Fix cứng channel 5
  wifi_set_channel(FIXED_CHANNEL);

  // Đăng ký callback
  wifi_set_promiscuous_rx_cb(promisc_cb);

  // Bật promiscuous
  wifi_promiscuous_enable(1);

  Serial.printf("Sniffing STARTED - Locked on Channel %d\n", FIXED_CHANNEL);
  Serial.println("Chờ client reconnect để tạo 4-way handshake...");
  Serial.println("Làm: Quên mạng → nhập pass lại (lặp 5-10 lần)");
}

void loop() {
  // Không cần code gì nhiều, callback tự chạy
  // In thống kê mỗi 10 giây (optional)
  static uint32_t last = 0;
  if (millis() - last > 10000) {
    last = millis();
    Serial.printf("[INFO] Đã bắt được %u EAPOL frames\n", eapol_count);
  }
  delay(5);
}
