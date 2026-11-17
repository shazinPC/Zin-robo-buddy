/*
 * 04_morphing_face.ino
 * ---------------------------------------------------------------
 * Smoothly animated face for Zin Robo Buddy.
 *
 * Every expression is just a set of numbers (a "pose"): eye width,
 * height, corner roundness, lid slope, cheek lift and vertical offset.
 * Each frame the eyes ease a little way toward the target pose, so
 * the face morphs from one expression to the next instead of
 * switching. Random blinking, glancing and expression changes make
 * it feel alive.
 *
 * Hardware : ESP32-C3 Super Mini + 0.96" SSD1306 OLED (I2C, 128x64)
 * Libraries: Adafruit SSD1306, Adafruit GFX Library
 * Board    : "ESP32C3 Dev Module", USB CDC On Boot = Enabled
 *
 * Serial Monitor (115200 baud): type 1-8 to force an expression,
 * or 0 to go back to automatic mode.
 *
 * Style inspired by the expressive eyes of robots such as Anki Cozmo.
 * All code in this file is original.
 * ---------------------------------------------------------------
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ---------- Display ----------
constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr int8_t  OLED_RESET    = -1;
constexpr uint8_t OLED_ADDR     = 0x3C;   // Try 0x3D if not found
constexpr uint8_t SDA_PIN       = 8;
constexpr uint8_t SCL_PIN       = 9;

// ---------- Behaviour ----------
constexpr uint16_t FRAME_MS          = 33;     // ~30 frames per second
constexpr float    EASE              = 0.30f;  // 0..1, higher = snappier morphs
constexpr uint16_t EXPR_MIN_MS       = 3500;   // time between expression changes
constexpr uint16_t EXPR_MAX_MS       = 7000;
constexpr uint16_t BLINK_MIN_MS      = 2000;
constexpr uint16_t BLINK_MAX_MS      = 5000;
constexpr uint16_t BLINK_DURATION_MS = 200;
constexpr uint16_t LOOK_MIN_MS       = 1200;
constexpr uint16_t LOOK_MAX_MS       = 3500;
constexpr float    LOOK_RANGE_X      = 8.0f;   // glance distance (pixels)
constexpr float    LOOK_RANGE_Y      = 4.0f;
constexpr int      EYE_GAP           = 28;     // space between the eyes
constexpr int      EYE_CENTER_Y      = 30;

// ================= Poses =================
//
// slope : lid tilt in pixels. Positive = inner corner lower (angry),
//         negative = outer corner lower (sad).
// cheek : how far the bottom edge is lifted into a curve (happy).
struct Pose {
  float w;        // eye width
  float hL, hR;   // left / right eye height
  float r;        // corner radius
  float slopeL, slopeR;
  float cheek;
  float dy;       // vertical shift of both eyes
};

enum Emotion : uint8_t {
  NORMAL, HAPPY, ANGRY, SAD, SURPRISED, SLEEPY, SKEPTICAL, EXCITED, EMOTION_COUNT
};

//                       w   hL  hR   r  slopeL slopeR cheek dy
const Pose POSES[EMOTION_COUNT] = {
  /* NORMAL    */ {30,  36, 36,  9,   0,   0,   0,   0},
  /* HAPPY     */ {30,  30, 30, 11,   0,   0,  13,   2},
  /* ANGRY     */ {30,  28, 28,  5,  13,  13,   0,   2},
  /* SAD       */ {28,  30, 30,  8, -11, -11,   0,   4},
  /* SURPRISED */ {32,  46, 46, 15,   0,   0,   0,  -2},
  /* SLEEPY    */ {30,  14, 14,  6,   4,   4,   0,   6},
  /* SKEPTICAL */ {30,  36, 22,  9,   0,   9,   0,   0},
  /* EXCITED   */ {34,  40, 40, 13,   0,   0,  10,  -2},
};

const char* const EMOTION_NAMES[EMOTION_COUNT] = {
  "normal", "happy", "angry", "sad", "surprised", "sleepy", "skeptical", "excited"
};

// ---------- Animation state (all floats so they can ease smoothly) ----------
Pose  cur = POSES[NORMAL];
Emotion target = NORMAL;
bool  autoMode = true;

float lookX = 0, lookY = 0, lookTX = 0, lookTY = 0;

uint32_t lastFrame = 0, nextExprAt = 0, nextBlinkAt = 0, nextLookAt = 0;
int32_t  blinkStart = -1;           // -1 = not blinking

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================= Helpers =================

inline float approach(float value, float goal) {
  return value + (goal - value) * EASE;
}

void easePose(Pose& p, const Pose& goal) {
  p.w      = approach(p.w, goal.w);
  p.hL     = approach(p.hL, goal.hL);
  p.hR     = approach(p.hR, goal.hR);
  p.r      = approach(p.r, goal.r);
  p.slopeL = approach(p.slopeL, goal.slopeL);
  p.slopeR = approach(p.slopeR, goal.slopeR);
  p.cheek  = approach(p.cheek, goal.cheek);
  p.dy     = approach(p.dy, goal.dy);
}

// Draws one eye. isLeft decides which side counts as the "inner" corner.
void drawEye(int x, int cy, int w, int h, int radius, int slope, int cheek, bool isLeft) {
  if (h < 2) h = 2;
  int top = cy - h / 2;
  display.fillRoundRect(x, top, w, h, min(radius, h / 2), SSD1306_WHITE);

  // Slanted upper lid: erase a triangle from the top edge
  if (slope != 0) {
    int inner = isLeft ? x + w : x;
    int outer = isLeft ? x : x + w;
    if (slope > 0) {
      display.fillTriangle(outer, top - 1, inner, top - 1, inner, top + slope, SSD1306_BLACK);
    } else {
      display.fillTriangle(outer, top - 1, inner, top - 1, outer, top - slope, SSD1306_BLACK);
    }
  }

  // Lifted lower lid: carve a circle out of the bottom edge
  if (cheek > 0) {
    int R = w;
    display.fillCircle(x + w / 2, top + h + R - cheek, R, SSD1306_BLACK);
  }
}

void drawFace(float blinkScale) {
  int w  = (int)lroundf(cur.w);
  int cy = EYE_CENTER_Y + (int)lroundf(cur.dy + lookY);
  int leftX  = SCREEN_WIDTH / 2 - EYE_GAP / 2 - w + (int)lroundf(lookX);
  int rightX = SCREEN_WIDTH / 2 + EYE_GAP / 2     + (int)lroundf(lookX);

  display.clearDisplay();
  drawEye(leftX,  cy, w, (int)lroundf(cur.hL * blinkScale), (int)lroundf(cur.r),
          (int)lroundf(cur.slopeL), (int)lroundf(cur.cheek), true);
  drawEye(rightX, cy, w, (int)lroundf(cur.hR * blinkScale), (int)lroundf(cur.r),
          (int)lroundf(cur.slopeR), (int)lroundf(cur.cheek), false);
  display.display();
}

// ================= Scheduling =================

void pickNewEmotion() {
  Emotion next;
  do {
    // Favour NORMAL so the face rests between emotions
    next = (random(0, 100) < 30) ? NORMAL : (Emotion)random(0, EMOTION_COUNT);
  } while (next == target);
  target = next;
  Serial.println(EMOTION_NAMES[target]);
}

void scheduleAll(uint32_t now) {
  nextExprAt  = now + random(EXPR_MIN_MS, EXPR_MAX_MS);
  nextBlinkAt = now + random(BLINK_MIN_MS, BLINK_MAX_MS);
  nextLookAt  = now + random(LOOK_MIN_MS, LOOK_MAX_MS);
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '0') {
      autoMode = true;
      Serial.println(F("auto mode"));
    } else if (c >= '1' && c <= '8') {
      autoMode = false;
      target = (Emotion)(c - '1');
      Serial.println(EMOTION_NAMES[target]);
    }
  }
}

// ================= Arduino entry points =================

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 not found. Check wiring and I2C address."));
    for (;;) { delay(1000); }
  }

  randomSeed(esp_random());
  scheduleAll(millis());
  drawFace(1.0f);
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;

  handleSerial();

  // New expression
  if (autoMode && now >= nextExprAt) {
    pickNewEmotion();
    nextExprAt = now + random(EXPR_MIN_MS, EXPR_MAX_MS);
  }

  // Start a blink
  if (blinkStart < 0 && now >= nextBlinkAt) {
    blinkStart = (int32_t)now;
  }

  // Blink animation: eyes shrink to a slit and reopen (smooth sine curve)
  float blinkScale = 1.0f;
  if (blinkStart >= 0) {
    float t = (float)(now - (uint32_t)blinkStart) / BLINK_DURATION_MS;
    if (t >= 1.0f) {
      blinkStart = -1;
      nextBlinkAt = now + random(BLINK_MIN_MS, BLINK_MAX_MS);
    } else {
      blinkScale = 1.0f - 0.92f * sinf(t * 3.14159f);
    }
  }

  // New glance direction
  if (now >= nextLookAt) {
    lookTX = (float)random(-100, 101) / 100.0f * LOOK_RANGE_X;
    lookTY = (float)random(-100, 101) / 100.0f * LOOK_RANGE_Y;
    nextLookAt = now + random(LOOK_MIN_MS, LOOK_MAX_MS);
  }

  // Ease everything toward its target
  easePose(cur, POSES[target]);
  lookX = approach(lookX, lookTX);
  lookY = approach(lookY, lookTY);

  drawFace(blinkScale);
}
