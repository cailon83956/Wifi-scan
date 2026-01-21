#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

// Buffer lưu gói tin (volatile để callback an toàn)
volatile uint8_t packetBuffer[512];
volatile uint16_t packetLen = 0;
volatile bool newPacket = false;

void sniffer_callback(uint8_t *buf, uint16_t len) {
  if (len < 50 || len > sizeof(packetBuffer) || newPacket) return;

  uint8_t fc = buf[0];
  if ((fc & 0xFC) != 0x88) return;  // Không phải QoS Data

  if ((buf[1] & 0x40) == 0) return;  // Không protected

  uint8_t *payload = buf + 24;
  if ((fc >> 4) & 0x08) payload += 2;

  if (payload[0] == 0xAA && payload[1] == 0xAA && payload[2] == 0x03 &&
      payload[3] == 0x00 && payload[4] == 0x00 && payload[5] == 0x00 &&
      payload[6] == 0x88 && payload[7] == 0x8E) {

    Serial.print("EAPOL DETECTED! Len: ");
    Serial.print(len);
    Serial.print(" | From MAC: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", buf[10 + i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();

    // Copy buffer (volatile nên dùng memcpy)
    memcpy((void*)packetBuffer, buf, len);
    packetLen = len;
    newPacket = true;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n[OK] ESP8266 Sniffer - Channel 5 - Bat EAPOL (da fix treo)");

  system_phy_set_max_tpw(40);  // Giữ nguyên hạ công suất

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(sniffer_callback);
  wifi_set_channel(5);
  wifi_promiscuous_enable(1);

  Serial.println("Da bat dau nghe tren channel 5...");
}

void loop() {
  if (newPacket) {
    // Xử lý gói tin ở đây (không volatile nữa)
    uint8_t *buf = (uint8_t*)packetBuffer;  // Cast an toàn

    // Tính offset eapol (sau header 802.11 + SNAP)
    uint8_t *eapol = buf + 24;
    if ((buf[0] >> 4) & 0x08) eapol += 2;  // QoS Control
    eapol += 8;  // Sau SNAP

    if (eapol[0] == 0x02 && eapol[1] == 0x03 && eapol[13] == 0x00) {
      Serial.println("PMKID detected! (Message 1)");
      Serial.print("PMKID: ");
      for (int i = 0; i < 16; i++) {
        Serial.printf("%02X", eapol[77 + i]);
      }
      Serial.println();
    }

    // In hex ngắn gọn (chỉ 64 byte đầu)
    Serial.print("Hex (first 64): ");
    for (int i = 0; i < 64 && i < packetLen; i++) {
      Serial.printf("%02X ", buf[i]);
    }
    Serial.println("\n");

    newPacket = false;
  }

  yield();
  delay(1);
}
