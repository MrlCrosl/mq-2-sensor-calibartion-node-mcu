#include <ESP8266WiFi.h>        // For ESP8266 (replace with <WiFi.h> for ESP32)
#include <WiFiClientSecure.h>   // For HTTPS requests

// Wi-Fi credentials
const char* ssid = "Your_SSID";         // Replace with your Wi-Fi SSID
const char* password = "Your_PASSWORD"; // Replace with your Wi-Fi Password

// OpenAI API
const char* host = "api.openai.com";
const int httpsPort = 443; // HTTPS port
const char* apiKey = "Your_OpenAI_API_Key"; // Replace with your OpenAI API Key

WiFiClientSecure client;

#define MQ_PIN A0              // Pin for the analog input connected to MQ-2
#define RL_VALUE 10            // Load resistance (R_L) set to 10 kOhms
#define CALIBRATION_TIME 30000 // Calibration time in milliseconds (30 seconds)
#define SAMPLE_INTERVAL 1000   // Interval between readings during calibration, ms

float R0 = 0;                  // Initial sensor resistance in clean air

// Function to calculate sensor resistance Rs
float calculateRs(int analogValue) {
    if (analogValue == 0) {
        Serial.println("Error: Analog value is 0. Skipping Rs calculation.");
        return NAN; // Prevent division by zero
    }
    float vout = (float)analogValue * 5.0 / 1023.0; // Convert analog value to voltage
    return ((5.0 - vout) * RL_VALUE) / vout; // Calculate Rs
}

// Function to send Rs values to ChatGPT API
float sendRsValuesToChatGPT(float rsValues[], int size) {
    if (!client.connect(host, httpsPort)) {
        Serial.println("Connection to OpenAI failed.");
        return NAN;
    }

    // Create JSON array of rsValues
    String jsonArray = "[";
    for (int i = 0; i < size; i++) {
        jsonArray += String(rsValues[i]);
        if (i < size - 1) jsonArray += ",";
    }
    jsonArray += "]";

    // Create JSON payload
    String jsonPayload = "{\"model\":\"gpt-3.5-turbo\",\"messages\":[{\"role\":\"user\",\"content\":\"Approximate the ideal value for the following RS values: " + jsonArray + "\"}]}";

    // Send HTTP POST request
    client.println("POST /v1/chat/completions HTTP/1.1");
    client.println("Host: " + String(host));
    client.println("Authorization: Bearer " + String(apiKey));
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(jsonPayload.length()));
    client.println();
    client.println(jsonPayload);

    // Read response
    String response = "";
    while (client.connected() || client.available()) {
        if (client.available()) {
            String line = client.readStringUntil('\n');
            response += line;
        }
    }
    client.stop();

    // Parse response to extract avgRs
    int idx = response.indexOf("\"content\":\"") + 11; // Locate "content" field in the response
    int endIdx = response.indexOf("\"", idx);
    String extractedValue = response.substring(idx, endIdx);

    return extractedValue.toFloat(); // Convert to float and return
}

// Function to calibrate the sensor in clean air
float calibrateSensor() {
    unsigned long startTime = millis();
    float rsValues[30]; // Array to hold Rs values
    int samples = 0;

    Serial.println("Calibrating the sensor for LPG... Please wait.");

    while (millis() - startTime < CALIBRATION_TIME && samples < 30) {
        int analogValue = analogRead(MQ_PIN);
        float rs = calculateRs(analogValue);
        rsValues[samples++] = rs; // Save Rs value
        delay(SAMPLE_INTERVAL);
    }

    // Send Rs values to ChatGPT API for ideal avgRs
    float idealAvgRs = sendRsValuesToChatGPT(rsValues, samples);
    Serial.print("Calibration complete. Ideal avgRs: ");
    Serial.println(idealAvgRs);

    return idealAvgRs / 9.83; // Rs/R0 in clean air is 9.83
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing...");

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Connected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Establish trust for HTTPS
    client.setInsecure(); // Do not validate SSL certificate (for testing purposes)

    // Calibrate sensor
    R0 = calibrateSensor();
}

void loop() {
    float rs = calculateRs(analogRead(MQ_PIN));
    float ppm = 603.99 * pow((rs / R0), -2.083); // LPG concentration in ppm

    Serial.print("LPG concentration: ");
    Serial.print(ppm);
    Serial.println(" ppm");

    delay(1000); // Wait before the next measurement
}
