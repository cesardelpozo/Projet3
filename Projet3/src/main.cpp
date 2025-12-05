#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <PubSubClient.h>

// WiFi credentials
const char* WIFI_SSID = "IOT-6220";
const char* WIFI_PASS = "6220M@cSelection";

// Thinger.io MQTT credentials
const char* MQTT_SERVER = "maisonneuve.aws.thinger.io";
const int MQTT_PORT = 1883;
const char* THINGER_USER = "Cesar"; // Replace with your Thinger.io username
const char* THINGER_DEVICE = "ESP32-C6-DevKit-M1";   // device id (used as MQTT client id)
const char* THINGER_CREDENTIAL = "Projet3"; // device credential / password 
const char* TOPIC = "coordonnees";

// Headers
String sendHttpRequest(const char* url);
String parseAndPrintISS(const String& payload);
bool connectToMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

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

// Connect to MQTT broker (Thinger.io)
bool connectToMQTT() {
  // Only reconnect if WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot connect to MQTT");
    return false; // WiFi not connected
  }

  if(mqttClient.connected()) {
    return true; // Already connected
  }

  Serial.print("Connecting to MQTT broker: ");
  Serial.println(MQTT_SERVER);

  if (mqttClient.connect(THINGER_DEVICE, THINGER_USER, THINGER_CREDENTIAL)) {
    Serial.println("MQTT connected!");
    return true; // Successfully connected
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(mqttClient.state());
    return false; // Connection failed
  }
}

// MQTT callback function (called when message is received on subscribed topic)
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Not subscribing to any topics yet, but keeping this for future use
}

void setup() {
  // Start serial for debug output
  Serial.begin(115200);
  // Small delay to allow Serial monitor to attach
  delay(100);

  // Configure MQTT client once
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);  // Increase buffer size for larger messages

  connectToWiFi();
  connectToMQTT();
}

void loop() {
  // Reconnect MQTT if disconnected
  if (!mqttClient.connected()) {
    if(connectToMQTT()) {
      Serial.println("Reconnected to MQTT broker");
    } else {
      Serial.println("Failed to reconnect to MQTT broker");
      delay(200);
      return; // Skip rest of loop if MQTT not connected
    }
  }
  mqttClient.loop(); // Keep MQTT connection alive

  // Example: send an HTTP GET request every 5 second
  static unsigned long lastRequest = 0;
  const unsigned long interval = 5000; // 5s
  unsigned long now = millis();

  if (now - lastRequest >= interval) {
    lastRequest = now;
    
    String issJson = sendHttpRequest("http://api.open-notify.org/iss-now.json");

    // Publish to MQTT topic
    while (!mqttClient.connected()) {
      Serial.println("MQTT disconnected, attempting to reconnect...");
      connectToMQTT();
      delay(200);
    }
    mqttClient.loop(); // Keep MQTT connection alive
    if (mqttClient.connected() && issJson.length() > 0) {
      Serial.print("Attempting to publish ");
      Serial.print(issJson.length());
      Serial.println(" bytes to MQTT...");
      if (mqttClient.publish(TOPIC, issJson.c_str())) {
        Serial.print("Published ISS data to MQTT topic: ");
        Serial.println(TOPIC);
      } else {
        Serial.print("Failed to publish to MQTT. State: ");
        Serial.println(mqttClient.state());
      }
    } else {
      Serial.println("MQTT not connected, skipping publish");
    }
  }
}

// Sends a simple HTTP GET request to the provided URL and prints the response
String sendHttpRequest(const char* url) {
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
  Serial.print(httpCode);

  while(httpCode == -1) {
    Serial.println(" (timeout, retrying...)");
    http.end();  // Clean up the previous connection
    delay(100);  // Wait before retrying
    http.begin(espClient, url);  // Re-establish connection
    httpCode = http.GET();
    Serial.print("HTTP GET to ");
    Serial.print(url);
    Serial.print(" returned ");
    Serial.print(httpCode);
  }

  Serial.println();

  if (httpCode >= 200 && httpCode <= 304) {
    String payload = http.getString();
    http.end();
    // Parse ISS JSON and print nicely formatted output
    return parseAndPrintISS(payload);
  } else {
    Serial.print("HTTP request failed, error: ");
    Serial.println(http.errorToString(httpCode));
    http.end();
    return "{}"; // Return empty JSON on failure
  }
}

// Parse a simple ISS JSON payload, print the important fields and return a compact JSON string
String parseAndPrintISS(const String& payload) {
  String p = payload;
  // Make a lowercase copy for case-insensitive key search
  String lower = p;
  lower.toLowerCase();

  String latitude = "(not found)";
  String longitude = "(not found)";
  String message = "(not found)";
  long timestamp = 0;

  // message: "message":"value"
  int idx = lower.indexOf("\"message\"");
  if (idx != -1) {
    int colon = lower.indexOf(':', idx);
    int q1 = p.indexOf('"', colon + 1);
    if (q1 != -1) {
      int q2 = p.indexOf('"', q1 + 1);
      if (q2 != -1) message = p.substring(q1 + 1, q2);
    }
  }

  // timestamp: "timestamp": 123456789
  idx = lower.indexOf("\"timestamp\"");
  if (idx != -1) {
    int colon = lower.indexOf(':', idx);
    if (colon != -1) {
      int endPos = lower.indexOf(',', colon + 1);
      if (endPos == -1) endPos = lower.indexOf('}', colon + 1);
      if (endPos != -1) {
        String ts = p.substring(colon + 1, endPos);
        ts.trim();
        timestamp = ts.toInt();
      }
    }
  }

  // latitude and longitude: either top-level or under iss_position
  idx = lower.indexOf("\"latitude\"");
  if (idx != -1) {
    int colon = lower.indexOf(':', idx);
    if (colon != -1) {
      int q1 = p.indexOf('"', colon + 1);
      if (q1 != -1) {
        int q2 = p.indexOf('"', q1 + 1);
        if (q2 != -1) latitude = p.substring(q1 + 1, q2);
      } else {
        int endPos = lower.indexOf(',', colon + 1);
        if (endPos == -1) endPos = lower.indexOf('}', colon + 1);
        if (endPos != -1) {
          latitude = p.substring(colon + 1, endPos);
          latitude.trim();
        }
      }
    }
  }

  // longitude
  idx = lower.indexOf("\"longitude\"");
  if (idx != -1) {
    int colon = lower.indexOf(':', idx);
    if (colon != -1) {
      int q1 = p.indexOf('"', colon + 1);
      if (q1 != -1) {
        int q2 = p.indexOf('"', q1 + 1);
        if (q2 != -1) longitude = p.substring(q1 + 1, q2);
      } else {
        int endPos = lower.indexOf(',', colon + 1);
        if (endPos == -1) endPos = lower.indexOf('}', colon + 1);
        if (endPos != -1) {
          longitude = p.substring(colon + 1, endPos);
          longitude.trim();
        }
      }
    }
  }

  // Print formatted summary
  Serial.println("--- ISS Position ---");
  Serial.print("Message   : "); Serial.println(message);
  Serial.print("Timestamp : ");
  if (timestamp != 0) Serial.println(timestamp); else Serial.println("(not found)");
  Serial.print("Latitude  : "); Serial.println(latitude);
  Serial.print("Longitude : "); Serial.println(longitude);
  Serial.println("---------------------");

  // Build a compact JSON to publish via MQTT
  String out = "{";
  out += "\"message\":\"" + message + "\",";
  out += "\"timestamp\":" + String(timestamp) + ",";
  out += "\"latitude\":\"" + latitude + "\",";
  out += "\"longitude\":\"" + longitude + "\"";
  out += "}";

  return out;
}
