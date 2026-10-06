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
const unsigned long CLICK_INTERVAL_MS = 300;

// 2. 手机屏幕分辨率（iPhone 12 默认已填好）
//    iPhone 12 / 12 Pro : 1170 x 2532
//    iPhone 12 mini     : 1080 x 2340
//    iPhone 12 Pro Max  : 1284 x 2778
//    其他机型请改成实际分辨率（设置 → 通用 → 关于本机 可查，或截图看像素）
const int SCREEN_WIDTH  = 1170;   // 屏幕宽度
const int SCREEN_HEIGHT = 2532;   // 屏幕高度

// 3. 要点击的位置列表（可任意增加或减少）
//    左上角是 (0, 0)，右下角是 (SCREEN_WIDTH, SCREEN_HEIGHT)
//    格式：{x坐标, y坐标},
struct Point {
  int x;
  int y;
};

Point clickPoints[] = {
  {194,  1280},    // 第1个点击位置 ← 改这里的数字
  {995, 1298},    // 第2个点击位置
  {800, 1700},    // 第3个点击位置
  // 想加更多点就继续往下写，例如：
  // {300, 2000},
  // {900, 1100},
};

// ============================================================
//                  以下代码一般不需要修改
// ============================================================

BleComboKeyboard keyboard("ESP32-C3-AutoClicker", "Espressif", 100);
BleComboMouse mouse(&keyboard);

const int POINT_COUNT = sizeof(clickPoints) / sizeof(clickPoints[0]);
int currentIndex = 0;
unsigned long lastClick = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32-C3 多点轮流连点器 启动中...");

  keyboard.begin();
  mouse.begin();

  // 设置屏幕分辨率（绝对坐标模式必须设置）
  mouse.setScreenSize(SCREEN_WIDTH, SCREEN_HEIGHT);

  // 可选：微调坐标偏移（如果整体偏左/偏上，可以改这里）
  // mouse.setCalibrationOffset(0, 0);

  Serial.println("BLE HID 已启动");
  Serial.println("设备名称: ESP32-C3-AutoClicker");
  Serial.print("共配置了 ");
  Serial.print(POINT_COUNT);
  Serial.println(" 个点击位置");
  Serial.print("点击间隔: ");
  Serial.print(CLICK_INTERVAL_MS);
  Serial.println(" ms");
  Serial.println("等待手机蓝牙连接...");
}

void doAbsoluteClick(int x, int y) {
  // 完整绝对坐标点击流程（推荐）
  // 1. 先移动到位置（不按下）
  mouse.sendAbsolutePixel(x, y, false, true);  // tip=false, inRange=true
  delay(20);

  // 2. 按下（tipSwitch = true）
  mouse.sendAbsolutePixel(x, y, true, true);
  delay(40);   // 按住时间，可调 30~80

  // 3. 松开
  mouse.sendAbsolutePixel(x, y, false, true);
  delay(15);

  // 4. 可选：离开范围（更干净）
  mouse.sendAbsolutePixel(x, y, false, false);
}

void loop() {
  if (keyboard.isConnected()) {
    unsigned long now = millis();

    if (now - lastClick >= CLICK_INTERVAL_MS) {
      // 取出当前要点击的坐标
      int x = clickPoints[currentIndex].x;
      int y = clickPoints[currentIndex].y;

      // 执行绝对坐标点击
      doAbsoluteClick(x, y);

      // 串口打印当前点击信息，方便调试
      Serial.print("点击 [");
      Serial.print(currentIndex + 1);
      Serial.print("/");
      Serial.print(POINT_COUNT);
      Serial.print("] 坐标:(");
      Serial.print(x);
      Serial.print(", ");
      Serial.print(y);
      Serial.println(")");

      // 切换到下一个点，循环
      currentIndex++;
      if (currentIndex >= POINT_COUNT) {
        currentIndex = 0;
      }

      lastClick = now;
    }
  } else {
    // 断开连接时重置计时
    lastClick = millis();
  }

  delay(5);
}
