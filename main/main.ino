#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define CHANNEL_FIXED 5
#define SERIAL_BAUD   115200

// ---- 802.11 frame control ----
#define WIFI_TYPE_MGMT 0x00
#define WIFI_TYPE_CTRL 0x01
#define WIFI_TYPE_DATA 0x02

static void ICACHE_FLASH_ATTR promisc_cb(uint8_t *buf, uint16_t len) {
  if (len < 60) return;

  // RX control header (first 12 bytes on ESP8266)
  int8_t rssi = (int8_t)buf[0];
  uint16_t rx_ctrl_len = 12;

  uint8_t *frame = buf + rx_ctrl_len;
  uint16_t frame_len = len - rx_ctrl_len;

  // ---- Frame Control ----
  uint16_t fc = frame[0] | (frame[1] << 8);
  uint8_t type = (fc >> 2) & 0x03;

  // Chỉ nhận DATA frame
  if (type != WIFI_TYPE_DATA) return;

  // ---- Header length ----
  uint8_t header_len = 24;

  // QoS frame → +2 bytes
  if ((fc & 0x0080) == 0x0080) {
    header_len += 2;
  }

  if (frame_len < header_len + 8) return;

  uint8_t *llc = frame + header_len;

  // ---- LLC + SNAP check ----
  if (!(llc[0] == 0xAA && llc[1] == 0xAA && llc[2] == 0x03 &&
        llc[3] == 0x00 && llc[4] == 0x00 && llc[5] == 0x00)) {
    return;
  }

  // ---- EtherType ----
  uint16_t ethertype = (llc[6] << 8) | llc[7];
  if (ethertype != 0x888E) return;

  // ---- EAPOL payload ----
  uint8_t *eapol = llc + 8;
  uint8_t version = eapol[0];

  if (version != 1 && version != 2) return;

  // ---- PASS ALL CHECKS ----
  Serial.println();
  Serial.println("=== VALID EAPOL DETECTED ===");
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.print(" dBm | Channel: ");
  Serial.println(CHANNEL_FIXED);

  Serial.print("EAPOL Version: ");
  Serial.println(version);

  Serial.println("----------------------------");
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(200);

  Serial.println();
  Serial.println("ESP8266 Secure EAPOL Sniffer");
  Serial.println("Security-aware detection enabled");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  wifi_set_opmode(STATION_MODE);

  wifi_promiscuous_enable(0);
  delay(10);

  wifi_set_channel(CHANNEL_FIXED);
  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(1);

  Serial.println("Sniffing started.");
}

void loop() {
  delay(10);
}
