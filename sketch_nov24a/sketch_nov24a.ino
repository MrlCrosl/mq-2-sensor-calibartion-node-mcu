#define MQ_PIN A0              // Pin for the analog input connected to MQ-2
#define RL_VALUE 10            // Load resistance (R_l) in kOhms
#define CALIBRATION_TIME 300000 // Calibration time in milliseconds (5 minutes)
#define SAMPLE_INTERVAL 1000    // Interval between readings during calibration, ms
#define READ_SAMPLE_TIMES 5     // Number of readings for averaging during measurement
#define READ_SAMPLE_INTERVAL 50 // Interval between readings during measurement, ms

float R0 = 0;                   // Initial sensor resistance in clean air (will be calculated)

// Constants for approximation specific to LPG (Liquefied Petroleum Gas)
const float a = -2.086;         // Slope of the curve for LPG
const float b = 2.781;          // Intercept of the curve for LPG

// Function to calculate sensor resistance Rs
float calculateRs(int analogValue) {
  float vout = (float)analogValue * 5.0 / 1023.0;
  return ((5.0 - vout) * RL_VALUE) / vout;
}

// Function to calibrate the sensor in clean air
float calibrateSensor() {
  unsigned long startTime = millis();
  float rsSum = 0;
  int samples = 0;

  Serial.println("Calibrating the sensor for LPG... Please wait.");
  Serial.print("Total calibration time: ");
  Serial.print(CALIBRATION_TIME / 1000);
  Serial.println(" seconds.");

  while (millis() - startTime < CALIBRATION_TIME) {
    int analogValue = analogRead(MQ_PIN);
    float rs = calculateRs(analogValue);
    rsSum += rs;
    samples++;

    // Output Rs and remaining time every 5 seconds
    if ((millis() - startTime) % 5000 < SAMPLE_INTERVAL) {
      int remainingTime = (CALIBRATION_TIME - (millis() - startTime)) / 1000;
      Serial.print("Calibration in progress... Rs: ");
      Serial.print(rs);
      Serial.print(" kOhms, Time left: ");
      Serial.print(remainingTime);
      Serial.println(" seconds.");
    }

    delay(SAMPLE_INTERVAL);
  }

  float avgRs = rsSum / samples;
  float r0 = avgRs / 1.7;  // Rs/R0 in clean air is 1.7

  Serial.println("Calibration complete.");
  Serial.print("R0 value (specific to LPG): ");
  Serial.print(r0);
  Serial.println(" kOhms");

  return r0;
}

// Function to calculate LPG concentration (ppm) based on Rs/R0
float calculatePPM(float rs) {
  float rs_ro_ratio = rs / R0;
  return pow(10, (log10(rs_ro_ratio) - b) / a);
}

void setup() {
  Serial.begin(9600);
  Serial.println("Initialization for LPG detection...");

  // Calibrate the sensor to get the R0 value
  R0 = calibrateSensor();
}

void loop() {
  float rs = 0;

  // Take multiple readings for averaging
  for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
    int analogValue = analogRead(MQ_PIN);
    rs += calculateRs(analogValue);
    delay(READ_SAMPLE_INTERVAL);
  }
  rs /= READ_SAMPLE_TIMES;

  // Calculate LPG concentration
  float ppm = calculatePPM(rs);

  // Output LPG concentration, Rs, and R0 value
  Serial.print("LPG concentration: ");
  Serial.print(ppm);
  Serial.print(" ppm, Rs: ");
  Serial.print(rs);
  Serial.print(" kOhms, R0: ");
  Serial.print(R0);
  Serial.println(" kOhms");

  delay(1000);  // Wait before the next measurement
}
