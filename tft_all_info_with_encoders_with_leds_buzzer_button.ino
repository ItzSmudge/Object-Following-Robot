#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// Motor and Encoder Pins
#define E1 5
#define M1 4
#define E2 6
#define M2 7

#define ENCODER1_A 2
#define ENCODER1_B 8
#define ENCODER2_A 3
#define ENCODER2_B 9

// TFT Display Pins
#define TFT_DC A5
#define TFT_CS 10

#define buzzer_pin A3  // buzzer sounds high pitch for left wheel movement, low pitch for right wheel movement
#define red_led A2      // red lights up for reverse movement
#define green_led A1      // green lights up for for forawrd movement

#define button_pin A0      // when button pressed, reset positions, directions, leds and buzzer


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Pulse count variables
volatile long pulseCount1 = 0;
volatile long pulseCount2 = 0;
volatile int direction1 = 1;
volatile int direction2 = 1;
int textY = 0;  // Y position for scrolling

// Interrupt Service Routine for Motor 1
void readEncoder1() {
    tone(buzzer_pin, 1000, 100);
    if ((digitalRead(ENCODER1_A) ^ digitalRead(ENCODER1_B)) == 1) {
        digitalWrite(green_led, HIGH);
        digitalWrite(red_led, LOW);
        pulseCount1++;
        direction1 = 1;
    } else {
        digitalWrite(red_led, HIGH);
        digitalWrite(green_led, LOW);
        pulseCount1--;
        direction1 = -1;
    }
}

// Interrupt Service Routine for Motor 2
void readEncoder2() {
    tone(buzzer_pin, 500, 100);
    if ((digitalRead(ENCODER2_A) ^ digitalRead(ENCODER2_B)) == 1) {
        digitalWrite(green_led, LOW);
        digitalWrite(red_led, HIGH);
        pulseCount2++;
        direction2 = 1;
    } else {
        digitalWrite(red_led, LOW);
        digitalWrite(green_led, HIGH);
        pulseCount2--;
        direction2 = -1;
    }
}

void spinWheelsForward() {
    digitalWrite(M1, HIGH);
    analogWrite(E1, 255);
    digitalWrite(M2, LOW);
    analogWrite(E2, 255);
    delay(100);
}

void spinWheelsBackward() {
    digitalWrite(M1, LOW);
    analogWrite(E1, 255);
    digitalWrite(M2, HIGH);
    analogWrite(E2, 255);
    delay(100);
}

void printPulsesDirection() {
    Serial.println(textY);
    Serial.print("Motor 1 - Pulses: ");
    Serial.print(pulseCount1);
    Serial.print(" Direction: ");
    Serial.println(direction1);

    Serial.print("Motor 2 - Pulses: ");
    Serial.print(pulseCount2);
    Serial.print(" Direction: ");
    Serial.println(direction2);

    // **Scroll Text on TFT**
    if (textY >= 30) {  // Reset screen when text reaches the bottom
        tft.fillScreen(ILI9341_BLACK);
        textY = 0;
        tft.setCursor(0, 0);
    }
    tft.setCursor(0, textY*10);
    tft.print("M1: "); tft.print(pulseCount1); 
    tft.print(" D: "); tft.print(direction1);
    tft.print(" | M2: "); tft.print(pulseCount2); 
    tft.print(" D: "); tft.println(direction2);
    
    textY++;  // Move cursor down for next line
    delay(100);
}

void setup() {
    Serial.begin(115200);
    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, textY);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);

    pinMode(ENCODER1_A, INPUT_PULLUP);
    pinMode(ENCODER1_B, INPUT_PULLUP);
    pinMode(ENCODER2_A, INPUT_PULLUP);
    pinMode(ENCODER2_B, INPUT_PULLUP);
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    pinMode(buzzer_pin, OUTPUT);
    pinMode(red_led, OUTPUT);
    pinMode(green_led, OUTPUT);
    pinMode(button_pin, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(ENCODER1_A), readEncoder1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER2_A), readEncoder2, CHANGE);

    Serial.println("Setup complete. Waiting for pulses...");
}

void loop() {

      printPulsesDirection();
      delay(1000);

      Serial.println(digitalRead(button_pin));
      if (digitalRead(button_pin) == LOW) {
        direction1 = 1 ;
        direction2 = 1 ;
        pulseCount1 = 0 ;
        pulseCount2 = 0 ;
        noTone(buzzer_pin);
        digitalWrite(red_led, LOW);
        digitalWrite(green_led, LOW);
      }

    
}
