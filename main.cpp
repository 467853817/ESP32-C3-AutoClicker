#include <Arduino.h>
#include <BleCombo.h>

// ESP32-C3 BLE Auto Clicker
// Connect the ESP32-C3 to your iPhone as a Bluetooth HID mouse.
// After connection, it sends a left click at the interval below.

BleComboKeyboard keyboard("ESP32-C3-AutoClicker", "Espressif", 100);
BleComboMouse mouse(&keyboard);

// Change this value to adjust the click interval.
// 1000 = 1 click per second
// 500  = 2 clicks per second
// 100  = 10 clicks per second
const unsigned long CLICK_INTERVAL_MS = 1000;

unsigned long lastClick = 0;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("ESP32-C3 AutoClicker starting...");

    keyboard.begin();
    mouse.begin();

    Serial.println("BLE HID started.");
    Serial.println("Device name: ESP32-C3-AutoClicker");
    Serial.println("Waiting for Bluetooth connection...");
}

void loop() {
    // Only click after a Bluetooth HID connection is established.
    if (keyboard.isConnected()) {
        unsigned long now = millis();

        if (now - lastClick >= CLICK_INTERVAL_MS) {
            mouse.click();
            lastClick = now;
            Serial.println("CLICK");
        }
    } else {
        // Reset the timer while disconnected.
        lastClick = millis();
    }

    delay(5);
}
