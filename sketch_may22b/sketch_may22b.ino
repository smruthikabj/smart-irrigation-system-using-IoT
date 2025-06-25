#include <WiFi.h>           // WiFi library for ESP32
#include <HTTPClient.h>     // HTTP client library to make POST requests
#include <DHT.h>            // DHT sensor library

// Define DHT sensor pin and type
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const int soilMoisturePin = 34; // Analog pin for soil moisture sensor
const int phPin = 35;           // Analog pin for pH sensor
const int pumpPin = 26;         // Digital output pin for controlling water pump relay

// WiFi credentials
const char* ssid = "Galaxy A15 5G DAE4";
const char* password = "chintu123";

// Server URL where ESP32 will send sensor data (replace with your Flask server IP)
const String serverURL = "http://192.168.152.179:5000/data";

// Variables to track pump override and current pump state
bool pumpOverride = false;  // When true, manual pump control is active
bool pumpState = false;     // true = ON, false = OFF

void setup() {
  Serial.begin(115200);             // Initialize serial communication for debugging
  pinMode(pumpPin, OUTPUT);         // Set pump pin as output
  digitalWrite(pumpPin, LOW);       // Make sure pump is off initially
  dht.begin();                     // Initialize DHT sensor

  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("📡 ESP32 IP: ");
  Serial.println(WiFi.localIP());   // Print ESP32 IP address
}

void loop() {
  // Read humidity and temperature from DHT sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Read raw soil moisture analog value and convert to percentage (0-100%)
  int soilRaw = analogRead(soilMoisturePin);
  float soilMoisturePercent = 100.0 - (soilRaw / 4095.0) * 100.0;  
  soilMoisturePercent = constrain(soilMoisturePercent, 0.0, 100.0);  // Clamp value to valid range

  // Read raw pH sensor value and convert to pH scale (~0-14)
  float phRaw = analogRead(phPin);
  Serial.println(phRaw);  // Print raw pH analog value for debugging
  float phValue = 0.74 * (phRaw * (14.0 / 4095.0));  // Calibrated pH conversion factor

  // Auto pump control logic: turn pump ON if soil moisture below 30%
  bool dry = soilMoisturePercent < 30.0;  // True if soil is dry

  // If no manual override, control pump automatically based on soil moisture
  if (!pumpOverride) {
    pumpState = dry;
    digitalWrite(pumpPin, pumpState ? HIGH : LOW);  // Turn pump ON or OFF
  }

  // Send sensor data to Flask server via HTTP POST
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);                         // Specify destination URL
    http.addHeader("Content-Type", "application/json");  // Specify content-type header

    // Construct JSON payload with sensor readings and pump status
    String postData = "{\"temp\":" + String(temperature, 1) +
                      ",\"humidity\":" + String(humidity, 1) +
                      ",\"soil\":" + String(soilMoisturePercent, 1) +
                      ",\"ph\":" + String(phValue, 2) +
                      ",\"pump\":\"" + (pumpState ? "ON" : "OFF") + "\"}";

    int httpCode = http.POST(postData);  // Send POST request
    Serial.print("HTTP POST code: ");
    Serial.println(httpCode);

    if (httpCode == 200) {  // If request successful
      String response = http.getString();  // Get response from server
      Serial.println("Server response: " + response);

      // Check if server requests manual pump override
      if (response.indexOf("manual") != -1) {
        pumpOverride = true; // Activate manual override mode

        // Parse pump state from server response (ON/OFF)
        pumpState = response.indexOf("ON") != -1;
        digitalWrite(pumpPin, pumpState ? HIGH : LOW);
        Serial.println("🚨 Manual Pump Override Activated");
      } else {
        // If no manual override requested, revert to auto control
        pumpOverride = false;
      }
    } else {
      Serial.println("❌ POST Failed");  // Log failed POST request
    }

    http.end();  // Close connection
  } else {
    Serial.println("⚠ No WiFi");  // Warn if WiFi disconnected
  }

  // Print sensor readings and pump status to serial monitor for debugging
  Serial.println("------------ SENSOR DATA ------------");
  Serial.print("🌡 Temp: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("💧 Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("🌱 Soil Moisture: "); Serial.print(soilMoisturePercent, 1); Serial.println(" %");
  Serial.print("🧪 pH: "); Serial.println(phValue, 2);
  Serial.print("💡 Pump: "); Serial.println(pumpState ? "ON" : "OFF");
  Serial.println("-------------------------------------");

  delay(3000);  // Wait 3 seconds before next reading
}
