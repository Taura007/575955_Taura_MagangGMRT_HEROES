#include <Keypad.h>
#include <Adafruit_NeoPixel.h>

// PIN 
#define LED_PIN 13
#define EN_PIN  10
#define IN1     A0
#define IN2     A1
#define ENC_A   2
#define ENC_B   3

#define NUM_LED 8
// KEYPAD 
// 1 = CW, 2 = CCW, 0 = Stop
// 6 = Speed Up, 4 = Speed Down
// A = E-Stop

char keys[4][4] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[4] = {12, 11, 9, 8};
byte colPins[4] = {7, 6, 5, 4};
Keypad keypad = Keypad(
  makeKeymap(keys), rowPins, colPins, 4, 4
);
Adafruit_NeoPixel led(
  NUM_LED, LED_PIN, NEO_GRB + NEO_KHZ800
);
// VARIABEL
int speedPWM = 0;
int direction = 0;       // 0 Stop, 1 CW, 2 CCW
bool eStop = false;

volatile unsigned long encoderCount = 0;
unsigned long oldCount = 0;
unsigned long lastCheck = 0;

bool encoderFault = false;

// ENCODER 
void encoderPulse() {
  encoderCount++;
}
// SETUP 
void setup() {
  pinMode(EN_PIN, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);

  led.begin();
  led.show();

  attachInterrupt(digitalPinToInterrupt(ENC_A), encoderPulse, CHANGE);
  stopMotor();
}
// LOOP 
void loop() {

  char key = keypad.getKey();

  // E-STOP
  if (key == 'A') {
    eStop = !eStop;

    if (eStop)
      stopMotor();
  }
  // LOCKED STATE
  if (eStop) {
    blinkRed();
    return;
  }
  // KEYPAD CONTROL
  if (key) {

    if (key == '1') {
      direction = 1;
      if (speedPWM == 0) speedPWM = 128;
    }
    else if (key == '2') {
      direction = 2;
      if (speedPWM == 0) speedPWM = 128;
    }
    else if (key == '0') {
      stopMotor();
    }
    else if (key == '6') {
      speedPWM = min(255, speedPWM + 32);
    }
    else if (key == '4') {
      speedPWM = max(0, speedPWM - 32);

      if (speedPWM == 0)
        direction = 0;
    }
  }
  motorControl();
  checkEncoder();
  showSpeed();
}
// MOTOR 
void motorControl() {

  analogWrite(EN_PIN, speedPWM);

  if (direction == 1) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
  else if (direction == 2) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  }

  else {
    stopMotor();
  }
}
void stopMotor() {

  speedPWM = 0;
  direction = 0;

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(EN_PIN, 0);

  encoderFault = false;
}
// ENCODER FAULT 
void checkEncoder() {

  if (speedPWM == 0) {
    oldCount = encoderCount;
    lastCheck = millis();
    encoderFault = false;
    return;
  }
  if (millis() - lastCheck >= 1000) {

    if (encoderCount == oldCount)
      encoderFault = true;
    else
      encoderFault = false;

    oldCount = encoderCount;
    lastCheck = millis();
  }
}
// NEOPIXEL 
void showSpeed() {

  led.clear();

  if (speedPWM == 0) {
    led.show();
    return;
  }
  int leds;

  // Normal = encoder
  if (!encoderFault) {
    leds = map(encoderCount % 50, 0, 50, 1, 8);
  }
  // Encoder error = PWM fallback
  else {
    leds = map(speedPWM, 0, 255, 0, 8);
  }
  uint32_t color;

  if (direction == 1)
    color = led.Color(0, 255, 0);      // CW = Green
  else
    color = led.Color(0, 0, 255);      // CCW = Blue

  for (int i = 0; i < leds; i++)
    led.setPixelColor(i, color);

  led.show();
}
// E-STOP 
void blinkRed() {

  static unsigned long timer = 0;
  static bool state = false;

  if (millis() - timer >= 300) {

    timer = millis();
    state = !state;

    led.clear();

    if (state) {
      for (int i = 0; i < NUM_LED; i++)
        led.setPixelColor(i, led.Color(255, 0, 0));
    }
    led.show();
  }
}
