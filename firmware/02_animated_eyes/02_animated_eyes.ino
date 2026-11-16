/*
 * 02_animated_eyes.ino
 * ---------------------------------------------------------------
 * Animated face for Zin Robo Buddy: two rounded eyes and a smile
 * that blink and glance left/right at random intervals.
 *
 * Hardware : ESP32-C3 Super Mini + 0.96" SSD1306 OLED (I2C, 128x64)
 * Libraries: Adafruit SSD1306, Adafruit GFX Library
 * Board    : "ESP32C3 Dev Module", USB CDC On Boot = Enabled
 *
 * Tweak the constants in the "Face design" and "Behaviour" sections
 * to change the look and personality.
 * ---------------------------------------------------------------
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Display ----------
constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int8_t  OLED_RESET    = -1;
constexpr uint8_t OLED_ADDR     = 0x3C;   // Try 0x3D if not found
constexpr uint8_t SDA_PIN       = 8;
constexpr uint8_t SCL_PIN       = 9;

// ---------- Face design ----------
constexpr int EYE_W        = 30;          // eye width
constexpr int EYE_H        = 38;          // eye height when fully open
constexpr int EYE_RADIUS   = 9;           // corner roundness
constexpr int LEFT_EYE_X   = 20;
constexpr int RIGHT_EYE_X  = 78;
constexpr int EYE_CENTER_Y = 24;
constexpr int MOUTH_X      = 64;          // mouth centre
constexpr int MOUTH_Y      = 52;
constexpr int MOUTH_HALF_W = 9;
constexpr int MOUTH_DEPTH  = 4;

// ---------- Behaviour ----------
constexpr uint16_t FRAME_MS        = 33;  // ~30 frames per second
constexpr uint16_t BLINK_MIN_MS    = 2000;
constexpr uint16_t BLINK_MAX_MS    = 5000;
constexpr uint16_t LOOK_MIN_MS     = 1500;
constexpr uint16_t LOOK_MAX_MS     = 4000;
constexpr int      LOOK_OFFSET     = 7;   // how far the eyes glance (pixels)
constexpr uint8_t  DOUBLE_BLINK_PCT = 20; // chance of a quick second blink

// Eye height at each step of a blink (open -> closed -> open)
const uint8_t BLINK_FRAMES[] = {38, 28, 14, 4, 14, 28, 38};
constexpr uint8_t BLINK_STEPS = sizeof(BLINK_FRAMES) / sizeof(BLINK_FRAMES[0]);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- Animation state ----------
uint32_t lastFrame   = 0;
uint32_t nextBlinkAt = 0;
uint32_t nextLookAt  = 0;
int8_t   blinkStep   = -1;                // -1 = not blinking
bool     doubleBlinkPending = false;
int      lookX       = 0;                 // current horizontal offset
int      lookTarget  = 0;                 // where the eyes are heading

void scheduleBlink(uint32_t now) {
  nextBlinkAt = now + random(BLINK_MIN_MS, BLINK_MAX_MS);
}

void scheduleLook(uint32_t now) {
  nextLookAt = now + random(LOOK_MIN_MS, LOOK_MAX_MS);
  const int choices[] = {-LOOK_OFFSET, 0, 0, LOOK_OFFSET};   // favour looking ahead
  lookTarget = choices[random(0, 4)];
}

void drawEye(int x, int eyeH) {
  int top    = EYE_CENTER_Y - eyeH / 2;
  int radius = min(EYE_RADIUS, eyeH / 2);
  display.fillRoundRect(x, top, EYE_W, eyeH, radius, SSD1306_WHITE);
}

void drawSmile(int offsetX) {
  int cx = MOUTH_X + offsetX;
  int prevX = 0, prevY = 0;
  for (int i = -MOUTH_HALF_W; i <= MOUTH_HALF_W; i++) {
    int y = MOUTH_Y + MOUTH_DEPTH - (i * i * MOUTH_DEPTH) / (MOUTH_HALF_W * MOUTH_HALF_W);
    int x = cx + i;
    if (i > -MOUTH_HALF_W) {
      display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
      display.drawLine(prevX, prevY + 1, x, y + 1, SSD1306_WHITE);  // 2 px thick
    }
    prevX = x;
    prevY = y;
  }
}

void drawFace() {
  int eyeH = (blinkStep >= 0) ? BLINK_FRAMES[blinkStep] : EYE_H;
  display.clearDisplay();
  drawEye(LEFT_EYE_X + lookX, eyeH);
  drawEye(RIGHT_EYE_X + lookX, eyeH);
  drawSmile(lookX / 2);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 not found. Check wiring and I2C address."));
    for (;;) { delay(1000); }
  }

  randomSeed(esp_random());                // hardware random on ESP32
  uint32_t now = millis();
  scheduleBlink(now);
  scheduleLook(now);
  drawFace();
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  // Start a blink when it's time
  if (blinkStep < 0 && now >= nextBlinkAt) {
    blinkStep = 0;
    doubleBlinkPending = (random(0, 100) < DOUBLE_BLINK_PCT);
  }

  // Advance the blink
  if (blinkStep >= 0) {
    blinkStep++;
    if (blinkStep >= BLINK_STEPS) {
      if (doubleBlinkPending) {
        doubleBlinkPending = false;
        blinkStep = 0;                     // go straight into a second blink
      } else {
        blinkStep = -1;
        scheduleBlink(now);
      }
    }
  }

  // Pick a new glance direction when it's time
  if (now >= nextLookAt) {
    scheduleLook(now);
  }

  // Ease the eyes toward the target, one pixel per frame
  if (lookX < lookTarget) lookX++;
  else if (lookX > lookTarget) lookX--;

  drawFace();
}
