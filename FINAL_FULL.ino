#include <WiFi101.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

// WiFi Credentials
char ssid[] = "Conn";
char pass[] = "12345678";
WiFiServer server(80);

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
#define buzzer_pin A3
#define red_led A2
#define green_led A1
#define button_pin A0

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
volatile long pulseCount1 = 0;
volatile long pulseCount2 = 0;
volatile int direction1 = 1;
volatile int direction2 = 1;
int textY = 0;
volatile int receivedCommand = 0;

// Encoder Interrupts
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
    digitalWrite(M2, HIGH);
    analogWrite(E2, 255);
    digitalWrite(green_led, HIGH);
    digitalWrite(red_led, LOW);
    tone(buzzer_pin, 1000, 100);
}

void spinWheelsBackward() {
    digitalWrite(M1, LOW);
    analogWrite(E1, 255);
    digitalWrite(M2, LOW);
    analogWrite(E2, 255);
    digitalWrite(red_led, HIGH);
    digitalWrite(green_led, LOW);
    tone(buzzer_pin, 500, 100);
}

void stopWheels() {
    digitalWrite(M1, LOW);
    analogWrite(E1, 0);
    digitalWrite(M2, LOW);
    analogWrite(E2, 0);
    digitalWrite(red_led, LOW);
    digitalWrite(green_led, LOW);
    noTone(buzzer_pin);
}

void printPulsesDirection() {
    if (textY >= 30) {
        tft.fillScreen(ILI9341_BLACK);
        textY = 0;
        tft.setCursor(0, 0);
    }
    tft.setCursor(0, textY * 10);
    tft.print("M1:"); tft.print(pulseCount1);
    tft.print(" D:"); tft.print(direction1);
    tft.print(" | M2:"); tft.print(pulseCount2);
    tft.print(" D:"); tft.println(direction2);
    textY++;
    delay(100);
}

void setup() {
    Serial.begin(115200);
    tft.begin();
    tft.setRotation(2);
    tft.fillScreen(ILI9341_BLACK);
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

    while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.print("Connected to WiFi. IP: ");
    Serial.println(WiFi.localIP());
    server.begin();
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        Serial.println("Client connected");
        while (client.connected()) {
            if (client.available()) {
                char buffer[4];
                int bytesRead = client.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
                buffer[bytesRead] = '\0';
                receivedCommand = atoi(buffer);
                Serial.print("Received: ");
                Serial.println(receivedCommand);
                if (receivedCommand == 1) spinWheelsForward();
                else if (receivedCommand == 2) spinWheelsBackward();
                else if (receivedCommand == 0) stopWheels();
                else Serial.println("Invalid Command");
                client.println("ACK");
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }
    if (digitalRead(button_pin) == LOW) {
        Serial.println("Resetting...");
        receivedCommand = 0;
        pulseCount1 = pulseCount2 = 0;
        direction1 = direction2 = 1;
        noTone(buzzer_pin);
        digitalWrite(red_led, LOW);
        digitalWrite(green_led, LOW);
    }
    printPulsesDirection();
    delay(1000);
}
