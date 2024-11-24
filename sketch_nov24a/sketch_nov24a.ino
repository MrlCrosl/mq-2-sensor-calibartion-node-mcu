#define MQ_PIN A0              // Pin for the analog input connected to MQ-2
#define RL_VALUE 10             // Load resistance (R_L) set to 10 kOhms
#define CALIBRATION_TIME 30000 // Calibration time in milliseconds (30 seconds)
#define SAMPLE_INTERVAL 1000   // Interval between readings during calibration, ms
#define READ_SAMPLE_TIMES 5    // Number of readings for averaging during measurement
#define READ_SAMPLE_INTERVAL 50 // Interval between readings during measurement, ms

float R0 = 0;                   // Initial sensor resistance in clean air (will be calculated)

// Constants for approximation specific to LPG (Liquefied Petroleum Gas)
const float a = 603.99;         // Coefficient a for LPG
const float b = -2.083;         // Coefficient b for LPG

// Function to calculate sensor resistance Rs
float calculateRs(int analogValue) {
    if (analogValue == 0) {
        Serial.println("Error: Analog value is 0. Skipping Rs calculation.");
        return NAN; // Prevent division by zero
    }
    float vout = (float)analogValue * 5.0 / 1023.0; // Convert analog value to voltage
    if (vout <= 0 || vout >= 5.0) {
        Serial.println("Error: Vout out of range.");
        return NAN; // Prevent invalid calculations
    }
    return ((5.0 - vout) * RL_VALUE) / vout; // Calculate Rs
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

        // Output Rs and remaining time every second
        unsigned long elapsedTime = millis() - startTime;
        int remainingTime = (CALIBRATION_TIME - elapsedTime) / 1000;
        Serial.print("Elapsed time: ");
        Serial.print(elapsedTime / 1000);
        Serial.print(" seconds, Current Rs: ");
        Serial.print(rs);
        Serial.println(" kOhms");

        delay(SAMPLE_INTERVAL);
    }

    float avgRs = rsSum / samples;
    float r0 = avgRs / 9.83;  // Rs/R0 in clean air is 9.83
    Serial.println("Calibration complete.");
    Serial.print("R0 value: ");
    Serial.println(r0);
    return r0;
}

// Function to calculate LPG concentration (ppm) using exponential regression
float calculatePPM(float rs) {
    if (isnan(rs) || rs <= 0) {
        Serial.println("Error: Rs is invalid. Cannot calculate PPM.");
        return NAN;
    }
    float rs_ro_ratio = rs / R0;
    return a * pow(rs_ro_ratio, b); // Exponential regression formula
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initialization for LPG detection...");
    R0 = calibrateSensor();  // Calibrate the sensor (or set R0 manually)
  
}

void loop() {
    float rs = 0;
    for (int i = 0; i < READ_SAMPLE_TIMES; i++) {
        int analogValue = analogRead(MQ_PIN);
        rs += calculateRs(analogValue);
        delay(READ_SAMPLE_INTERVAL);
    }
    rs /= READ_SAMPLE_TIMES;

    float ppm = calculatePPM(rs);
    Serial.print("LPG concentration: ");
    Serial.print(ppm);
    Serial.print(" ppm, Rs: ");
    Serial.print(rs);
    Serial.print(" kOhms, R0: ");
    Serial.print(R0);
    Serial.println(" kOhms");

    delay(1000);  // Wait before the next measurement
}
