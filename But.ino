// Thư viện-----------
#include <Wire.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 mpu;
BleMouse bleMouse("But_3d", "HaNgocQuy", 100);

// Đặt chân chuột trái,phải
#define CHUOT_TRAI 4
#define CHUOT_PHAI 5

void setup() {
  // Mở bảng giám sát
  Serial.begin(115200);
  Wire.begin(21, 22); // Chân 21 là SDA, 22 là SCL
  
  // Bắt đầu chạy IMU (mpu6050) 
  mpu.initialize();
  // Bắt đầu điều khiển chuột
  bleMouse.begin();

  // Kết nối với IMU (mpu6050)
  if (mpu.testConnection()) {
    Serial.println("da ket noi mpu6050");
  } else {
    Serial.println("ket noi that bai mpu6050");
  }

  // Đặt chế độ nút nhấn
  pinMode(CHUOT_TRAI, INPUT_PULLUP);
  pinMode(CHUOT_PHAI, INPUT_PULLUP);
}
//Tạo các biến--------------------------------
int16_t ax, ay, az; // Gia tốc hướng (sẽ cần khi phát triển mô phỏng vị trí chuột)
int16_t gx, gy, gz; // Tốc độ xoay

// Khi không di chuyển, imu vẫn gửi tín hiệu: nhiễu, tín hiệu sai
// Tín hiệu sai, cụ thể là tín hiệu luôn tăng nhiều hơn so với giá trị gốc một giá trị không đổi.
// VD: lỗi +3000 và gốc a = 12000 => tín hiệu gửi ra 12000 + 3000 = 15000
long tongGx = 0, tongGz = 0; // Các biến khắc phục lỗi
int16_t Ogx = 0, Ogz = 0; // Các biến khắc phục lỗi

int16_t dx = 0, dz = 0; // di chuyển chuột
int16_t h = 800; // Chiều cao - độ nhạy ( cần dùng nếu mô phỏng 3 chiều )
int16_t i = 0, n = 1000; // Các biến khác
//============================================
void loop() {
  if (bleMouse.isConnected()) {
    // Lấy dữ liệu nguyên từ cảm biến-------------
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    // hiệu chỉnh đầu ra cảm biến-----------------
    if (i < n) {
      if (i == 0) {
      Serial.println("Đừng di chuyển mpu");
      }
      // Lấy tổng các giá trị gửi ra
      tongGx += gx;
      tongGz += gz;
      i ++;
      return;
    }
    if (i == n) {
      // Chia cho số lần đã lấy
      Ogx = tongGx / n;
      Ogz = tongGz / n;
      Serial.println("Đã hoàn tất");
      delay(1000);
      i ++;
    }
    // Ta lấy được Ogx/z là giá trị lỗi trung bình
    //============================================
    //Di chuyển chuột-----------------------------
    // Lấy tốc độ xoay (cảm biến - lỗi) chia cho độ nhạy
    dx = (gx - Ogx) / h; 
    dz = -(gz - Ogz) / h;
    // Di chuyển
    bleMouse.move(dx, dz); // Trên trục tọa độ Oxy, dx là trục x, dz là trục y
    //============================================
    //Nút nhấn chuột------------------------------
    // Chuột trái
    if (digitalRead(CHUOT_TRAI) == LOW) { // Nếu nhấn nút
      bleMouse.press(MOUSE_LEFT); // Nhấn(giữ) chuột
    } else { // Nếu không
      bleMouse.release(MOUSE_LEFT); // Thả chuột
    }
    // Chuột phải
    if (digitalRead(CHUOT_PHAI) == LOW) {
      bleMouse.press(MOUSE_RIGHT);
    } else {
      bleMouse.release(MOUSE_RIGHT);
    }
    //============================================
    //Giám sát------------------------------------
    Serial.print(" dx: "); Serial.print(dx);
    Serial.print(" dy: "); Serial.print(dz);
    Serial.print(" ogx: "); Serial.print(Ogx);
    Serial.print(" ogy: "); Serial.print(Ogz);
    Serial.print(" L: "); Serial.print(digitalRead(CHUOT_TRAI));
    Serial.print(" R: "); Serial.println(digitalRead(CHUOT_PHAI));
    //============================================
  }
  delay(5); // Delay để giảm bớt công việc cho cpu, tránh quá tải
}
