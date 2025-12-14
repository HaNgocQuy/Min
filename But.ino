#include <Wire.h>
#include <MPU6050.h>
#include <BleMouse.h>

MPU6050 mpu;
BleMouse bleMouse("Bút bluetooth");

// Lập biến-------------------------------------------------------
// Các biến đầu vào của IMU
int16_t ax, ay, az;
int16_t gx, gy, gz;

// Tính và loại bỏ độ lệch trung bình cảm biến đo được(hiệu chỉnh)
long Ogx = 0.0f, Ogz = 0.0f;

// Tính vị trí chuột
float x = 1.0f, y = 1.0f;    // Chuột đầu ra(thật)
float x2 = 0.0f, y2 = 0.0f;  // Chuột đầu vào(giả)

// Các Biến cài đặt
const float r = 80.0f;
const float v = 10.0f;

int16_t i = 0, n = 1000;
long tongGx = 0, tongGz = 0;
//================================================================

// Chuẩn bị
void setup() {
  // Chạy serial để quan sát
  Serial.begin(115200);

  // Đặt vai trò chân 21 là sda, chân 22 là scl(theo esp32)
  Wire.begin(21, 22);

  mpu.initialize();
  bleMouse.begin();

  // Đặt chức năng của các chân
  pinMode(4, INPUT_PULLUP);  // Chuột trái
  pinMode(5, INPUT_PULLUP);  // Chuột phải
  pinMode(2, OUTPUT);        // Đèn led

  // Kiểm tra kết nối IMU
  if (!mpu.testConnection()) {
    Serial.println("Kết nối MPU thất bại!");
  } else {
    Serial.println("Đã kết nối MPU!");
  }
}

void loop() {
  if (bleMouse.isConnected()) {
    // Đọc tốc độ góc từ cảm biến
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    if (i < n) {
      if (i == 0) {
        digitalWrite(2, HIGH);
        Serial.println("Đừng di chuyển mpu");
      }
      // Lấy tổng các giá trị gửi ra
      tongGx += gx;
      tongGz += gz;
      i++;
      return;
    }
    if (i == n) {
      // Chia cho số lần đã lấy
      Ogx = tongGx / n;
      Ogz = tongGz / n;
      x = 1;
      y = 1;
      x2 = 0;
      y2 = 0;
      Serial.println("Đã hoàn tất");
      digitalWrite(2, LOW);
      delay(1000);
      i++;
    }
    // Tính vị trí chuột giả
    // (float)gx - Ogx hay (float)gz - Ogz là đầu ra cảm biến sau khi hiệu chỉnh: lấy cảm biến thật trừ độ lệch
    float dx2 = -((float)gz - Ogz) / 800.0f;
    float dy2 = -((float)gx - Ogx) / 800.0f;
    if (fabsf(dx2) > 1 || fabsf(dy2) > 1) {
      x2 += dx2;
      y2 += dy2;
    }
    // Tìm tọa độ vectơ d(từ chuột thật đến chuột giả)
    float dx = x2 - x;
    float dy = y2 - y;

    // Tính chiều dài vectơ
    float dist = sqrtf(dx * dx + dy * dy);
    // Tính khoảng cần đi
    float can = dist - r;
    // Tính khoảng nên đi mỗi lần lặp(vì giới hạn tốc độ)
    float move = 0.0f;
    if (can > 0.0f) {
      move = fminf(can, v);
    } else {
      move = 0.0f;
    }

    float movex = 0.0f;
    float movey = 0.0f;

    // Để tránh lỗi: dist = 0 thì không chia được
    if (dist > 0.0001f) {
      // nếu có khoảng cách giữa chuột thật và đầu ra thì tạo đường đi(x, y) cho chuột
      movex = (dx / dist) * move;
      movey = (dy / dist) * move;
    } else {
      // Nếu không thì không di chuyển
      movex = 0.0f;
      movey = 0.0f;
    }
    // Tính vị trí chuột thật
    x += movex;
    y += movey;

    // Nếu dữ liệu lớn sẽ đặt lại tránh quá tải
    const float LIMIT = 3e4f;
    if (fabsf(x) > LIMIT || fabsf(y) > LIMIT || fabsf(x2) > LIMIT || fabsf(y2) > LIMIT) {
      x = 0.0f;
      y = 0.0f;
      if (fabsf(dx) < LIMIT || fabsf(dy) < LIMIT) {
        x2 = dx;
        y2 = dy;
      } else {
        x2 = 0.0f;
        y2 = 0.0f;
      }
    }

    // Di chuyển
    bleMouse.move(movex, movey);  // Di chuyển chuột theo vectơ(movex, movey)

    // Nhấn Chuột
    // Chuột trái
    if (digitalRead(4) == LOW) {     // Nếu nhấn nút
      bleMouse.press(MOUSE_LEFT);    // Nhấn(giữ) chuột
    } else {                         // Nếu không
      bleMouse.release(MOUSE_LEFT);  // Thả chuột
    }
    // Chuột phải (tương tự)
    if (digitalRead(5) == LOW) {
      bleMouse.press(MOUSE_RIGHT);
    } else {
      bleMouse.release(MOUSE_RIGHT);
    }

    // Giám sát
    Serial.print((long)roundf(x));
    Serial.print(",");
    Serial.print((long)roundf(y));
    Serial.print(";");
    Serial.print((long)roundf(x2));
    Serial.print(",");
    Serial.println((long)roundf(y2));
    delay(5);
  }
}
