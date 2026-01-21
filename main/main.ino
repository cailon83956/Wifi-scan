#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Buffer lưu gói tin (chỉ lưu khi cần, tránh copy liên tục)
volatile uint8_t packetBuffer[512];
volatile uint16_t packetLen = 0;
volatile bool newPacket = false;

void promiscuous_cb(uint8_t *buf, uint16_t len) {
  // Tránh làm việc nặng trong callback để không treo
  if (len < 50 || len > sizeof(packetBuffer) || newPacket) return;

  uint8_t fc = buf[0];
  if ((fc & 0xFC) != 0x88) return;  // Không phải QoS Data

  if ((buf[1] & 0x40) == 0) return;  // Không protected

  uint8_t *payload = buf + 24;
  if ((fc >> 4) & 0x08) payload += 2;  // QoS Control

  // Kiểm tra SNAP + EAPOL (888E)
  if (payload[0] == 0xAA && payload[1] == 0xAA && payload[2] == 0x03 &&
      payload[3] == 0x00 && payload[4] == 0x00 && payload[5] == 0x00 &&
      payload[6] == 0x88 && payload[7] == 0x8E) {

    // Copy gói tin vào buffer (volatile để an toàn)
    memcpy((void*)packetBuffer, buf, len);
    packetLen = len;
    newPacket = true;

    // Chỉ in thông báo ngắn gọn trong callback
    Serial.print(".");  // Dấu chấm để biết đang bắt được gói
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP8266 Sniffer - Bat EAPOL/PMKID tren channel 5 (da sua treo)");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  wifi_promiscuous_enable(1);
  wifi_set_promiscuous_rx_cb(promiscuous_cb);

  // Cố định channel 5
  wifi_set_channel(5);
  Serial.println("Da set channel 5 - Bat dau nghe...");

  // Yield để tránh watchdog
  yield();
}

void loop() {
  if (newPacket) {
    // Xử lý gói tin ở đây (ngoài callback) để an toàn
    Serial.printf("\n[ EAPOL DETECTED ] Len: %d | From MAC: ", packetLen);

    // Source MAC offset 10
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", packetBuffer[10 + i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();

    // Kiểm tra PMKID (nếu là message 1)
    uint8_t *eapol = packetBuffer + 24 + ((packetBuffer[0] >> 4) & 0x08 ? 26 : 24);  // offset sau header
    eapol += 8;  // sau SNAP

    if (eapol[0] == 0x02 && eapol[1] == 0x03 && eapol[13] == 0x00) {
      Serial.println("PMKID detected! (Message 1)");
      Serial.print("PMKID: ");
      for (int i = 0; i < 16; i++) {
        Serial.printf("%02X", eapol[77 + i]);
      }
      Serial.println();
    }

    // Optional: in hex dump ngắn gọn (chỉ 64 byte đầu)
    Serial.print("Hex (first 64 bytes): ");
    for (int i = 0; i < 64 && i < packetLen; i++) {
      Serial.printf("%02X ", packetBuffer[i]);
    }
    Serial.println("\n");

    newPacket = false;
  }

  // Yield thường xuyên để tránh watchdog reset
  yield();
  delay(10);  // Nhẹ nhàng, không chiếm CPU
}
