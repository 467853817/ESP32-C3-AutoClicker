#include <Arduino.h>
#include <BleCombo.h>

// ============================================================
//                  【用户配置区域】只需要改这里
// ============================================================

// 1. 点击间隔（单位：毫秒）—— 先保持大一点方便观察
const unsigned long CLICK_INTERVAL_MS = 800;

// 2. 手机屏幕分辨率（必须正确！）
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

// 4. 移动缩放系数（重要！）
//    iOS 有指针加速，实际移动距离往往比发送的大
//    如果冲过头（跑到边缘/底部），把这个数调小，例如 0.6 ~ 0.8
//    如果走不到位，调大一点，例如 1.1 ~ 1.3
const float MOVE_SCALE = 0.7;

// ============================================================
//                  以下代码一般不需要修改
// ============================================================

BleComboKeyboard keyboard("ESP32-C3-AutoClicker", "Espressif", 100);
BleComboMouse mouse(&keyboard);

const int POINT_COUNT = sizeof(clickPoints) / sizeof(clickPoints[0]);
int currentIndex = 0;
unsigned long lastClick = 0;

// 相对移动单步最大值
const int MAX_STEP = 80;   // 改小一点，减少加速影响

// 把光标强制归位到左上角（温和版）
void homeToTopLeft() {
  // 分多次、小步长往左上推
  for (int i = 0; i < 25; i++) {
    mouse.move(-MAX_STEP, -MAX_STEP);
    delay(8);
  }
  // 再单独往左和往上各补几次
  for (int i = 0; i < 12; i++) {
    mouse.move(-MAX_STEP, 0);
    delay(6);
  }
  for (int i = 0; i < 12; i++) {
    mouse.move(0, -MAX_STEP);
    delay(6);
  }
  delay(50);  // 给系统一点时间稳定
}

// 从当前位置相对移动到目标（已假设当前位置接近 0,0）
void moveRelativeTo(int targetX, int targetY) {
  // 应用缩放系数，抵消 iOS 加速
  int scaledX = (int)(targetX * MOVE_SCALE);
  int scaledY = (int)(targetY * MOVE_SCALE);

  int remainX = scaledX;
  int remainY = scaledY;

  while (remainX != 0 || remainY != 0) {
    int stepX = constrain(remainX, -MAX_STEP, MAX_STEP);
    int stepY = constrain(remainY, -MAX_STEP, MAX_STEP);

    mouse.move(stepX, stepY);
    delay(6);

    remainX -= stepX;
    remainY -= stepY;
  }
  delay(30);
}

// 完整点击：归位 → 移动到目标 → 点击
void doRelativeClick(int x, int y) {
  homeToTopLeft();
  moveRelativeTo(x, y);
  mouse.click();
  delay(40);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32-C3 多点轮流连点器 (相对鼠标优化版) 启动中...");

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
  Serial.print("移动缩放: ");
  Serial.println(MOVE_SCALE);
  Serial.println("当前模式: 相对鼠标 + 温和归位");
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
