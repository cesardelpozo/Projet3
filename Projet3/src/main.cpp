#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// WiFi credentials
const char* WIFI_SSID = "Papi Cesar";
const char* WIFI_PASS = "Apple Time";

//Headers
void sendHttpRequest(const char* url);
const char* httpReasonPhrase(int code);
String parseAndPrintISS(const String& payload);

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
  // Example: send an HTTP GET request every 2 seconds
  static unsigned long lastRequest = 0;
  const unsigned long interval = 2000; // 2s
  unsigned long now = millis();

  if (now - lastRequest >= interval) {
    lastRequest = now;
    
    sendHttpRequest("http://api.open-notify.org/iss-now.json");
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
  Serial.print(httpCode);

  while(httpCode == -1) {
    Serial.println(" (timeout, retrying...)");
    httpCode = http.GET();
    Serial.print("HTTP GET to ");
    Serial.print(url);
    Serial.print(" returned ");
    Serial.print(httpCode);
  }

  // Print a human readable reason if available
  const char* reason = httpReasonPhrase(httpCode);
  if (reason != NULL) {
    Serial.print(" (");
    Serial.print(reason);
    Serial.print(")");
  }
  Serial.println();

  if (httpCode >= 200 && httpCode <= 304) {
    String payload = http.getString();
    // Parse ISS JSON and print nicely formatted output
    String issJson = parseAndPrintISS(payload);
  } else {
    Serial.print("HTTP request failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

// Return a brief reason phrase for common HTTP status codes
const char* httpReasonPhrase(int code) {
  switch (code) {
    case 100: return "Continue";
    case 101: return "Switching Protocols";
    case 102: return "Processing";
    case 103: return "Early Hints";
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 204: return "No Content";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 304: return "Not Modified";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 402: return "Payment Required";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 413: return "Payload Too Large";
    case 429: return "Too Many Requests";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway Timeout";
    default: return NULL;
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
  // latitude
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
