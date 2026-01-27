#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define CHANNEL_FIXED 5
#define SERIAL_BAUD   115200

static void ICACHE_FLASH_ATTR promisc_cb(uint8_t *buf, uint16_t len) {
  if (len < 36) return;

  int8_t rssi = (int8_t)buf[1];

  for (int i = 24; i < len - 2; i++) {
    if (buf[i] == 0x88 && buf[i + 1] == 0x8E) {

      Serial.println();
      Serial.println("=== EAPOL DETECTED ===");
      Serial.print("Len: ");
      Serial.print(len);
      Serial.print(" | RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm | Channel: ");
      Serial.println(CHANNEL_FIXED);

      Serial.print("Hex dump (first 64 bytes): ");
      for (int j = 0; j < 64 && j < len; j++) {
        if (buf[j] < 0x10) Serial.print("0");
        Serial.print(buf[j], HEX);
        Serial.print(" ");
      }
      Serial.println("\n----------------------");

      return;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);

  Serial.println();
  Serial.println("ESP8266 Promiscuous Sniffer");
  Serial.println("Listening for EAPOL on channel 5...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  wifi_set_opmode(STATION_MODE);

  wifi_promiscuous_enable(0);
  delay(10);

  wifi_set_channel(CHANNEL_FIXED);   // ← channel 5
  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(1);

  Serial.println("Sniffing started.");
}

void loop() {
  delay(10);
}
