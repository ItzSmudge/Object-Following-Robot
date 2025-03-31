#include <WiFi101.h>

char ssid[] = "SSID";
char pass[] = "PASSWD";
int serverPort = 80;
WiFiServer server(serverPort);

#define E1 5
#define M1 4
#define E2 6
#define M2 7
#define buzzer_pin A3
#define red_led A2
#define green_led A1
#define button_pin A0

volatile int receivedCommand = 0;

void setup() {
    Serial.begin(9600);
    pinMode(M1, OUTPUT);
    pinMode(M2, OUTPUT);
    // pinMode(E1, OUTPUT); // If you intend to control speed, enable this pin
    // pinMode(E2, OUTPUT); // If you intend to control speed, enable this pin
    pinMode(buzzer_pin, OUTPUT);
    pinMode(red_led, OUTPUT);
    pinMode(green_led, OUTPUT);
    pinMode(button_pin, INPUT_PULLUP);

    digitalWrite(M1, LOW);
    analogWrite(E1, 0);
    digitalWrite(M2, HIGH);
    analogWrite(E2, 0);

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
                char buffer[4]; // Buffer to hold integer (max size 4 bytes)
                int bytesRead = client.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
                buffer[bytesRead] = '\0'; // Ensure null termination

                // Convert buffer to integer
                receivedCommand = atoi(buffer); // Convert the received string to an integer
                Serial.print("Raw Data Received: ");
                Serial.println(receivedCommand);

                // Handle movement commands based on the received data
                if (receivedCommand == 1) {  // Forward
                    Serial.println("Moving Forward");
                    spinWheelsForward();
                } else if (receivedCommand == 2) {  // Backward
                    Serial.println("Moving Backward");
                    spinWheelsBackward();
                } else if (receivedCommand == 0) {  // Stop
                    Serial.println("Stopping");
                    stopWheels();
                } else {
                    Serial.println("Invalid data received");
                }

                client.println("ACK"); // Send acknowledgment
            }
        }
        client.stop();
        Serial.println("Client disconnected");
    }

    if (digitalRead(button_pin) == LOW) {
        Serial.println("Resetting...");
        receivedCommand = 0;
        noTone(buzzer_pin);
        digitalWrite(red_led, LOW);
        digitalWrite(green_led, LOW);
    }
}

void spinWheelsForward() {
    digitalWrite(M1, HIGH);
    analogWrite(E1, 255);  // Set motor speed if using PWM
    digitalWrite(M2, HIGH);
    analogWrite(E2, 255);  // Set motor speed if using PWM
    delay(100);  // Small delay to ensure forward movement
    digitalWrite(green_led, HIGH);
    digitalWrite(red_led, LOW);
    tone(buzzer_pin, 1000, 100);  // Buzzer tone for movement
}

void spinWheelsBackward() {
    digitalWrite(M1, LOW);
    analogWrite(E1, 255);  // Set motor speed if using PWM
    digitalWrite(M2, LOW);
    analogWrite(E2, 255);  // Set motor speed if using PWM
    delay(100);  // Small delay to ensure backward movement
    digitalWrite(red_led, HIGH);
    digitalWrite(green_led, LOW);
    tone(buzzer_pin, 500, 100);  // Buzzer tone for movement
}

void stopWheels() {
    digitalWrite(M1, LOW);
    analogWrite(E1, 0);  // Stop motor by setting PWM to 0
    digitalWrite(M2, LOW);
    analogWrite(E2, 0);  // Stop motor by setting PWM to 0
    digitalWrite(red_led, LOW);  // Turn off red LED
    digitalWrite(green_led, LOW);  // Turn off green LED
    noTone(buzzer_pin);  // Turn off the buzzer
}
