/*
 * 01_oled_hello_test.ino
 * ---------------------------------------------------------------
 * Bring-up test: ESP32-C3 Super Mini + 0.96" SSD1306 OLED (I2C)
 *
 * Purpose : Verify that the board flashes correctly, the I2C wiring
 *           is right, and the display responds.
 * Behaviour: Shows "Hello!" and toggles the display on/off every 500 ms.
 *
 * Libraries (install via Arduino Library Manager):
 *   - Adafruit SSD1306
 *   - Adafruit GFX Library
 *
 * Board settings (Arduino IDE):
 *   - Board: "ESP32C3 Dev Module"
 *   - USB CDC On Boot: "Enabled"   (needed for Serial over USB)
 * ---------------------------------------------------------------
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Display configuration ----------
constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int8_t  OLED_RESET    = -1;     // No reset pin on 4-pin modules
constexpr uint8_t OLED_ADDR     = 0x3C;   // Try 0x3D if the display is not found

// ---------- I2C pins (ESP32-C3 Super Mini defaults) ----------
// Change these if you wired the display to different GPIOs.
constexpr uint8_t SDA_PIN = 8;
constexpr uint8_t SCL_PIN = 9;

// ---------- Timing ----------
constexpr uint16_t BLINK_INTERVAL_MS = 500;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void showHello() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 25);
  display.println("Hello!");
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 not found. Check wiring and I2C address (0x3C / 0x3D)."));
    for (;;) { delay(1000); }   // Halt: nothing else can work without the display
  }

  Serial.println(F("SSD1306 initialised."));
  showHello();
}

void loop() {
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  delay(BLINK_INTERVAL_MS);
  display.ssd1306_command(SSD1306_DISPLAYON);
  delay(BLINK_INTERVAL_MS);
}
