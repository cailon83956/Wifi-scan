#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"  // SDK cũ, cần cho promiscuous
}

#define CHANNEL_FIXED 11     // Fix cứng channel 11 như bạn yêu cầu
#define SERIAL_BAUD 115200

// Callback khi nhận gói tin promiscuous
static void ICACHE_FLASH_ATTR promisc_cb(uint8_t *buf, uint16_t len) {
  // buf[0..] là raw 802.11 frame + radiotap-like header (ESP thêm ~ bytes đầu)

  // Bỏ qua nếu gói quá ngắn
  if (len < 40) return;

  // Kiểm tra FCS (Frame Check Sequence) ở cuối gói có valid không (ESP thường thêm)
  // Nhưng để đơn giản, ta skip check FCS

  // Tìm offset đến 802.11 header (thường sau 12-24 bytes header ESP)
  // Thực tế: buf[0..3] = length + RSSI + ..., rồi MAC header
  // Để chính xác hơn, ta tìm EAPOL signature: Type 0x88 0x8e (LLC + EAPOL)

  // Duyệt tìm byte 0x88 0x8e (EAPOL thường ở offset ~24 + LLC 8 bytes)
  for (int i = 10; i < len - 2; i++) {  // bắt đầu từ offset khả thi
    if (buf[i] == 0x88 && buf[i+1] == 0x8e) {
      // Tìm thấy EAPOL! In ra info cơ bản
      Serial.println();
      Serial.print("=== EAPOL DETECTED! Len=");
      Serial.print(len);
      Serial.print(" RSSI=");
      Serial.print((int8_t)buf[1]);  // RSSI thường ở byte 1 (signed)
      Serial.print(" Channel=");
      Serial.println(CHANNEL_FIXED);

      // In hex dump 60 bytes đầu để check handshake (Message 1/2/3/4)
      Serial.print("Hex dump: ");
      for (int j = 0; j < 60 && j < len; j++) {
        if (buf[j] < 16) Serial.print("0");
        Serial.print(buf[j], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Optional: lưu vào flash/SD nếu bạn muốn (cần thêm code)
      // Ở đây chỉ in serial để bạn xem bằng Serial Monitor
      return;  // Có thể break nếu chỉ cần 1 EAPOL
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(100);
  Serial.println("\nESP8266 Promiscuous Sniffer - Fixed Channel 11");
  Serial.println("Chờ EAPOL (WPA handshake) trên channel 11...");

  WiFi.mode(WIFI_STA);          // Station mode
  WiFi.disconnect();            // Ngắt kết nối cũ
  delay(100);

  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);   // Tắt trước để set lại
  delay(10);

  wifi_set_channel(CHANNEL_FIXED);          // Fix cứng channel 11
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Đăng ký callback
  wifi_promiscuous_enable(1);               // Bật promiscuous

  Serial.println("Sniffing started on channel 11...");
}

void loop() {
  // Không cần làm gì ở loop, callback sẽ tự gọi khi có gói
  // Có thể thêm channel hop nếu sau muốn, nhưng bạn yêu cầu fix 11
  delay(10);
}
