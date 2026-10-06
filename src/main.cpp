#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <BLE2902.h>

// ============================================================
// ESP32-C3 Dual Touch BLE HID
// iPhone 12 / iOS 17 测试版
//
// Touch 1 -> DOWN -> UP
// Touch 2 -> DOWN -> UP
// Touch 1 -> DOWN -> UP
// ...
// ============================================================

BLEHIDDevice* hid = nullptr;
BLECharacteristic* inputReport = nullptr;

bool deviceConnected = false;

// ------------------------------------------------------------
// 屏幕尺寸
// iPhone 12：1170 x 2532
// ------------------------------------------------------------
constexpr uint16_t SCREEN_WIDTH  = 1169;
constexpr uint16_t SCREEN_HEIGHT = 2531;

// ------------------------------------------------------------
// 两个触点坐标
// 你之前使用的坐标
// ------------------------------------------------------------
constexpr uint16_t TOUCH1_X = 194;
constexpr uint16_t TOUCH1_Y = 1280;

constexpr uint16_t TOUCH2_X = 995;
constexpr uint16_t TOUCH2_Y = 1298;

// ------------------------------------------------------------
// HID Report Descriptor
//
// 一个 Report 中包含：
// Contact 1
// Contact 2
// Contact Count
//
// 每个 Contact：
// 1 byte  Flags
// 1 byte  Contact ID
// 2 byte  X
// 2 byte  Y
//
// Flags:
// bit 0 = Tip Switch
// bit 1 = In Range
// ------------------------------------------------------------
static const uint8_t hidReportDescriptor[] = {

    0x05, 0x0D,                    // Usage Page (Digitizers)
    0x09, 0x04,                    // Usage (Touch Screen)
    0xA1, 0x01,                    // Collection (Application)

    0x85, 0x05,                    // Report ID (5)

    // ========================================================
    // Contact 1
    // ========================================================
    0x09, 0x22,                    // Usage (Finger)
    0xA1, 0x02,                    // Collection (Logical)

        // Tip Switch
        0x09, 0x42,
        // In Range
        0x09, 0x32,

        0x15, 0x00,                // Logical Min = 0
        0x25, 0x01,                // Logical Max = 1
        0x75, 0x01,                // Report Size = 1
        0x95, 0x02,                // Report Count = 2
        0x81, 0x02,                // Input (Data, Variable, Absolute)

        // Padding
        0x75, 0x01,
        0x95, 0x06,
        0x81, 0x03,                // Input (Constant)

        // Contact Identifier
        0x09, 0x51,
        0x15, 0x00,
        0x25, 0xFF,
        0x75, 0x08,
        0x95, 0x01,
        0x81, 0x02,

        // X
        0x05, 0x01,                // Generic Desktop
        0x09, 0x30,                // X
        0x16, 0x00, 0x00,
        0x26, 0x91, 0x04,          // 1169
        0x75, 0x10,
        0x95, 0x01,
        0x81, 0x02,

        // Y
        0x09, 0x31,                // Y
        0x16, 0x00, 0x00,
        0x26, 0xE3, 0x09,          // 2531
        0x75, 0x10,
        0x95, 0x01,
        0x81, 0x02,

    0xC0,                          // End Contact 1

    // ========================================================
    // Contact 2
    // ========================================================
    0x05, 0x0D,                    // Digitizers
    0x09, 0x22,                    // Finger
    0xA1, 0x02,                    // Collection (Logical)

        // Tip Switch
        0x09, 0x42,
        // In Range
        0x09, 0x32,

        0x15, 0x00,
        0x25, 0x01,
        0x75, 0x01,
        0x95, 0x02,
        0x81, 0x02,

        // Padding
        0x75, 0x01,
        0x95, 0x06,
        0x81, 0x03,

        // Contact Identifier
        0x09, 0x51,
        0x15, 0x00,
        0x25, 0xFF,
        0x75, 0x08,
        0x95, 0x01,
        0x81, 0x02,

        // X
        0x05, 0x01,
        0x09, 0x30,
        0x16, 0x00, 0x00,
        0x26, 0x91, 0x04,          // 1169
        0x75, 0x10,
        0x95, 0x01,
        0x81, 0x02,

        // Y
        0x09, 0x31,
        0x16, 0x00, 0x00,
        0x26, 0xE3, 0x09,          // 2531
        0x75, 0x10,
        0x95, 0x01,
        0x81, 0x02,

    0xC0,                          // End Contact 2

    // ========================================================
    // Contact Count
    // ========================================================
    0x05, 0x0D,
    0x09, 0x54,                    // Contact Count
    0x15, 0x00,
    0x25, 0x02,
    0x75, 0x08,
    0x95, 0x01,
    0x81, 0x02,

    0xC0                           // End Application
};

// ------------------------------------------------------------
// BLE Server callbacks
// ------------------------------------------------------------
class ServerCallbacks : public BLEServerCallbacks {

    void onConnect(BLEServer* server) override {
        deviceConnected = true;
        Serial.println("BLE connected");
    }

    void onDisconnect(BLEServer* server) override {
        deviceConnected = false;
        Serial.println("BLE disconnected");

        delay(200);
        BLEDevice::startAdvertising();

        Serial.println("BLE advertising restarted");
    }
};

// ------------------------------------------------------------
// 发送一个完整 Touch Report
// ------------------------------------------------------------
void sendTouchReport(
    bool touch1Down,
    uint16_t x1,
    uint16_t y1,
    bool touch2Down,
    uint16_t x2,
    uint16_t y2
) {
    if (!deviceConnected || inputReport == nullptr) {
        return;
    }

    uint8_t report[13];

    // Report ID
    report[0] = 0x05;

    // --------------------------------------------------------
    // Contact 1
    // --------------------------------------------------------
    report[1] = touch1Down ? 0x03 : 0x00;
    report[2] = 0x01;

    report[3] = x1 & 0xFF;
    report[4] = (x1 >> 8) & 0xFF;

    report[5] = y1 & 0xFF;
    report[6] = (y1 >> 8) & 0xFF;

    // --------------------------------------------------------
    // Contact 2
    // --------------------------------------------------------
    report[7] = touch2Down ? 0x03 : 0x00;
    report[8] = 0x02;

    report[9]  = x2 & 0xFF;
    report[10] = (x2 >> 8) & 0xFF;

    report[11] = y2 & 0xFF;
    report[12] = (y2 >> 8) & 0xFF;

    // --------------------------------------------------------
    // Contact Count
    // --------------------------------------------------------
    uint8_t contactCount = 0;

    if (touch1Down) {
        contactCount++;
    }

    if (touch2Down) {
        contactCount++;
    }

    // 注意：
    // 当前 report 已经 13 bytes，
    // 最后一字节需要 Contact Count。
    //
    // 为避免修改前面的结构，
    // 我们这里重新组织成 14 bytes。
    uint8_t finalReport[14];

    memcpy(finalReport, report, 13);
    finalReport[13] = contactCount;

    inputReport->setValue(finalReport, sizeof(finalReport));
    inputReport->notify();
}

// ------------------------------------------------------------
// Touch 1
// ------------------------------------------------------------
void touch1Click() {

    if (!deviceConnected) {
        return;
    }

    Serial.println("Touch 1 DOWN");

    sendTouchReport(
        true,
        TOUCH1_X,
        TOUCH1_Y,
        false,
        TOUCH2_X,
        TOUCH2_Y
    );

    delay(80);

    Serial.println("Touch 1 UP");

    sendTouchReport(
        false,
        TOUCH1_X,
        TOUCH1_Y,
        false,
        TOUCH2_X,
        TOUCH2_Y
    );

    delay(120);
}

// ------------------------------------------------------------
// Touch 2
// ------------------------------------------------------------
void touch2Click() {

    if (!deviceConnected) {
        return;
    }

    Serial.println("Touch 2 DOWN");

    sendTouchReport(
        false,
        TOUCH1_X,
        TOUCH1_Y,
        true,
        TOUCH2_X,
        TOUCH2_Y
    );

    delay(80);

    Serial.println("Touch 2 UP");

    sendTouchReport(
        false,
        TOUCH1_X,
        TOUCH1_Y,
        false,
        TOUCH2_X,
        TOUCH2_Y
    );

    delay(120);
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {

    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("================================");
    Serial.println("ESP32-C3 Dual Touch HID");
    Serial.println("================================");

    // BLE 初始化
    BLEDevice::init("ESP32-C3-DualTouch");

    // BLE Server
    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());

    // HID Device
    hid = new BLEHIDDevice(server);

    // HID 信息
    hid->manufacturer()->setValue("Espressif");

    hid->pnp(
        0x02,
        0x303A,
        0x1001,
        0x0110
    );

    hid->hidInfo(
        0x00,
        0x01
    );

    // HID Report Descriptor
    hid->reportMap(
        (uint8_t*)hidReportDescriptor,
        sizeof(hidReportDescriptor)
    );

    // Input Report
    inputReport = hid->inputReport(0x05);

    // 开启 HID
    hid->startServices();

    // BLE Advertising
    BLEAdvertising* advertising = BLEDevice::getAdvertising();

    advertising->addServiceUUID(
        hid->hidService()->getUUID()
    );

    advertising->setScanResponse(true);
    advertising->start();

    Serial.println("BLE advertising started");
    Serial.println("Device name: ESP32-C3-DualTouch");
}

// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------
void loop() {

    if (!deviceConnected) {
        delay(500);
        return;
    }

    // Touch 1
    touch1Click();

    // Touch 2
    touch2Click();

    // 一轮结束
    delay(200);
}
