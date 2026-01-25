#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"  // Cần cho promiscuous
}

#define FIXED_CHANNEL 5
#define SERIAL_BAUD 115200

uint32_t eapol_count = 0;

static void ICACHE_FLASH_ATTR promisc_cb(uint8_t *buf, uint16_t len) {
  if (len < 50) return;

  int8_t rssi = (int8_t)buf[1];  // RSSI signed

  // Tìm EAPOL: 0x88 0x8E
  for (uint16_t i = 10; i < len - 4; i++) {
    if (buf[i] == 0x88 && buf[i + 1] == 0x8E) {
      eapol_count++;
      uint16_t eapol_offset = i;
      uint16_t eapol_len = len - eapol_offset;

      Serial.println();
      Serial.printf("=== EAPOL #%u DETECTED! Total Len=%u | EAPOL approx=%u | RSSI=%d dBm | Ch=%d ===\n",
                    eapol_count, len, eapol_len, rssi, FIXED_CHANNEL);

      // Dump hex dài hơn (tối đa 250 bytes)
      Serial.print("Hex dump: ");
      uint16_t dump_len = min((uint16_t)250, len);
      for (uint16_t j = 0; j < dump_len; j++) {
        if (buf[j] < 16) Serial.print("0");
        Serial.print(buf[j], HEX);
        Serial.print(" ");
        if ((j + 1) % 16 == 0) Serial.print("  ");
      }
      Serial.println();

      if (eapol_len > 95) {
        Serial.println("!!! CO THE LA FULL MESSAGE (len >95) - Copy hex de convert Hashcat !!!");
      }

      return;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);
  Serial.println("\nESP8266 Sniffer EAPOL Handshake - Channel 5 FIXED");
  Serial.println("Cho client reconnect de tao handshake...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(200);

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  delay(100);

  wifi_set_channel(FIXED_CHANNEL);
  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(1);

  Serial.printf("Sniffing bat dau tren channel %d...\n", FIXED_CHANNEL);
}

void loop() {
  static uint32_t last_print = 0;
  if (millis() - last_print > 10000) {
    last_print = millis();
    Serial.printf("[INFO] Da bat %u EAPOL frames\n", eapol_count);
  }
  delay(10);
}
