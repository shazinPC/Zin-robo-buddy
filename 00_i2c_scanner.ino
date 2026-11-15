/*
 * 00_i2c_scanner.ino
 * ---------------------------------------------------------------
 * Scans the I2C bus and prints every device address found.
 * Use this first if the OLED is not detected (usually 0x3C or 0x3D).
 *
 * Open Serial Monitor at 115200 baud.
 * Board setting: USB CDC On Boot = "Enabled".
 * ---------------------------------------------------------------
 */

#include <Wire.h>

constexpr uint8_t SDA_PIN = 8;   // ESP32-C3 Super Mini default
constexpr uint8_t SCL_PIN = 9;   // ESP32-C3 Super Mini default

void setup() {
  Serial.begin(115200);
  delay(1500);                    // Give USB serial time to connect
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println(F("I2C scanner started."));
}

void loop() {
  uint8_t found = 0;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("Device found at 0x"));
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);
      found++;
    }
  }

  if (found == 0) {
    Serial.println(F("No I2C devices found. Check wiring, power and pull-ups."));
  }
  Serial.println(F("--- scan complete ---\n"));
  delay(3000);
}
