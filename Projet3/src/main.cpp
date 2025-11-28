#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// WiFi credentials
const char* WIFI_SSID = "IOT-6220";
const char* WIFI_PASS = "6220M@cSelection";

//Headers
void sendHttpRequest(const char* url);

WiFiClient espClient;

// Attempt to connect to WiFi network; if it fails, wait 1 second and try again.
void connectToWiFi() {
  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(WIFI_SSID);

  // Begin WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Loop until connected, retrying every 1 second
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
  }
}

void setup() {
  // Start serial for debug output
  Serial.begin(115200);
  // Small delay to allow Serial monitor to attach
  delay(100);

  connectToWiFi();
}

void loop() {
  // Example: send an HTTP GET request every 10 seconds
  static unsigned long lastRequest = 0;
  const unsigned long interval = 10000; // 10s
  unsigned long now = millis();

  if (now - lastRequest >= interval) {
    lastRequest = now;
    // Replace the URL below with your API endpoint
    sendHttpRequest("http://google.com");
  }
}

// Sends a simple HTTP GET request to the provided URL and prints the response
void sendHttpRequest(const char* url) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, attempting to reconnect...");
    connectToWiFi();
  }

  HTTPClient http;
  http.begin(espClient, url);
  int httpCode = http.GET();
  Serial.print("HTTP GET to ");
  Serial.print(url);
  Serial.print(" returned ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println("Response payload:");
    Serial.println(payload);
  } else {
    Serial.print("HTTP request failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}
