// Code nháy đèn xanh theo nhịp tay cho ESP8266
// Chân LED_BUILTIN thường là chân D4 trên NodeMCU/Wemos D1 Mini

void setup() {
  // Khởi tạo cổng Serial với tốc độ 115200 (phải khớp với app trên điện thoại)
  Serial.begin(115200);
  
  // Thiết lập chân đèn LED là đầu ra
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Tắt đèn lúc khởi động (thường mức HIGH là tắt trên ESP8266)
  digitalWrite(LED_BUILTIN, HIGH);
  
  Serial.println("--- Da ket noi! Bam 1 de BAT, 0 de TAT ---");
}

void loop() {
  // Kiểm tra xem có lệnh nào gửi từ điện thoại xuống không
  if (Serial.available() > 0) {
    char command = Serial.read(); // Đọc ký tự gửi đến
    
    if (command == '1') {
      digitalWrite(LED_BUILTIN, LOW);  // Bật đèn xanh
      Serial.println("LED: ON");
    } 
    else if (command == '0') {
      digitalWrite(LED_BUILTIN, HIGH); // Tắt đèn xanh
      Serial.println("LED: OFF");
    }
  }
}
