#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

// WiFi credentials
const char* WIFI_SSID = "IOT-6220";
const char* WIFI_PASS = "6220M@cSelection";

WiFiClient espClient;

// Attempt to connect to WiFi network; if it fails, wait 1 second and try again.
void connectToWiFi() {
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(WIFI_SSID);

  // Begin WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Loop until connected, retrying every 1 second
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("WiFi not connected, retrying in 1s...\n");
    delay(1000);
    // If not connected, call begin again to restart connection attempt
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // Start serial for debug output
  Serial.begin(115200);
  // Small delay to allow Serial monitor to attach
  delay(100);

  connectToWiFi();
}

void loop() {
  // put your main code here, to run repeatedly:
}
