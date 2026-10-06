#include <Arduino.h>
#include <BleCombo.h>

// ============================================================
//                  【用户配置区域】只需要改这里
// ============================================================

// 1. 点击间隔（单位：毫秒）
//    1000 = 每秒 1 次
//     500 = 每秒 2 次
//     300 = 大约每秒 3 次
//     200 = 每秒 5 次
//     100 = 每秒 10 次
const unsigned long CLICK_INTERVAL_MS = 500;   // 相对模式建议先用 500 以上，稳定后再调小

// 2. 手机屏幕分辨率（必须正确！）
//    iPhone 12 / 12 Pro : 1170 x 2532
//    iPhone 12 mini     : 1080 x 2340
//    iPhone 12 Pro Max  : 1284 x 2778
//    iPhone 13/14       : 1170 x 2532
//    iPhone 15          : 1179 x 2556
//    其他机型请改成实际分辨率
const int SCREEN_WIDTH  = 1170;
const int SCREEN_HEIGHT = 2532;

// 3. 要点击的位置列表（左上角是 0,0）
struct Point {
  int x;
  int y;
};

Point clickPoints[] = {
  {194,  1280},   // 分段（左边灰色）
  {995,  1298},   // 启动（右边绿色）
};

// ============================================================
//                  以下代码一般不需要修改
// ============================================================

BleComboKeyboard keyboard("ESP32-C3-AutoClicker", "Espressif", 100);
BleComboMouse mouse(&keyboard);

const int POINT_COUNT = sizeof(clickPoints) / sizeof(clickPoints[0]);
int currentIndex = 0;
unsigned long lastClick = 0;

// 相对移动单步最大值（HID 相对鼠标限制）
const int MAX_STEP = 127;

// 把光标强制归位到左上角 (0,0)
void homeToTopLeft() {
  // 多移动几次，确保不管当前位置在哪都能顶到左上角
  for (int i = 0; i < 20; i++) {
    mouse.move(-MAX_STEP, -MAX_STEP);
    delay(5);
  }
  // 再多往左和往上各推一次，更保险
  for (int i = 0; i < 10; i++) {
    mouse.move(-MAX_STEP, 0);
    delay(3);
  }
  for (int i = 0; i < 10; i++) {
    mouse.move(0, -MAX_STEP);
    delay(3);
  }
  delay(30);  // 等系统稳定
}

// 从当前位置相对移动到目标（已假设当前位置是 0,0）
void moveRelativeTo(int targetX, int targetY) {
  int remainX = targetX;
  int remainY = targetY;

  while (remainX != 0 || remainY != 0) {
    int stepX = constrain(remainX, -MAX_STEP, MAX_STEP);
    int stepY = constrain(remainY, -MAX_STEP, MAX_STEP);

    mouse.move(stepX, stepY);
    delay(4);

    remainX -= stepX;
    remainY -= stepY;
  }
  delay(20);
}

// 完整点击：归位 → 移动到目标 → 点击
void doRelativeClick(int x, int y) {
  // 1. 强制归位到左上角
  homeToTopLeft();

  // 2. 相对移动到目标坐标
  moveRelativeTo(x, y);

  // 3. 点击
  mouse.click();
  delay(30);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32-C3 多点轮流连点器 (相对鼠标模式) 启动中...");

  keyboard.begin();
  mouse.begin();

  Serial.println("BLE HID 已启动");
  Serial.println("设备名称: ESP32-C3-AutoClicker");
  Serial.print("共配置了 ");
  Serial.print(POINT_COUNT);
  Serial.println(" 个点击位置");
  Serial.print("点击间隔: ");
  Serial.print(CLICK_INTERVAL_MS);
  Serial.println(" ms");
  Serial.println("当前模式: 相对鼠标 + 归位 (兼容性最好)");
  Serial.println("等待手机蓝牙连接...");
}

void loop() {
  if (keyboard.isConnected()) {
    unsigned long now = millis();

    if (now - lastClick >= CLICK_INTERVAL_MS) {
      int x = clickPoints[currentIndex].x;
      int y = clickPoints[currentIndex].y;

      doRelativeClick(x, y);

      Serial.print("点击 [");
      Serial.print(currentIndex + 1);
      Serial.print("/");
      Serial.print(POINT_COUNT);
      Serial.print("] 坐标:(");
      Serial.print(x);
      Serial.print(", ");
      Serial.print(y);
      Serial.println(")");

      currentIndex++;
      if (currentIndex >= POINT_COUNT) {
        currentIndex = 0;
      }

      lastClick = now;
    }
  } else {
    lastClick = millis();
  }

  delay(5);
}
