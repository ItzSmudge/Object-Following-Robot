#include <SPI.h>
#include <WiFi101.h>

//Use this code to obtain the uip address of the wifi shield
// Replace with your network credentials
char ssid[] = "SSID";
char pass[] = "PASSWD";

// Server port
int serverPort = 80;

WiFiServer server(serverPort);  // Create a server object

void setup() {
  Serial.begin(9600);  // Start serial communication
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Once connected, print the IP address in the proper format
  IPAddress ip = WiFi.localIP();
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.print(ip[0]); 
  Serial.print(".");
  Serial.print(ip[1]); 
  Serial.print(".");
  Serial.print(ip[2]); 
  Serial.print(".");
  Serial.println(ip[3]);

  // Print the server port
  Serial.print("Server is running on port: ");
  Serial.println(serverPort);

  // Start the server
  server.begin();
}

void loop() {
  // Check if a client is connected
  WiFiClient client = server.available();

  if (client) {
    Serial.println("Client connected");

    // Wait for data from the client
    while (client.connected()) {
      if (client.available()) {
        String data = client.readStringUntil('\n');  // Read incoming data
        Serial.println("Data received: " + data);

        // Optionally send a response back
        client.println("Message received.");
      }
    }
    client.stop();  // Close the connection
    Serial.println("Client disconnected");
  }
}
