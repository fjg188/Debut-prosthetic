#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HDC302x.h>
#include <Adafruit_Sensor.h>

// Create HDC3022 sensor object
Adafruit_HDC302x hdc3022 = Adafruit_HDC302x();

// Structure to hold temperature/fan speed data points
struct FanCurvePoint {
  float temperature;  // Temperature in Celsius
  float fanSpeed;     // Fan speed from 0.0 to 1.0 (0% to 100%)
};

// Fan curve data stored in program memory
// Format: {temperature, fanSpeed}
const FanCurvePoint fanCurve[] PROGMEM = {
  {30.0, 0.00},
  {40.0, 0.20},
  {50.0, 0.35},
  {60.0, 0.50},
  {70.0, 0.70},
  {80.0, 0.85},
  {90.0, 1.00}
};

const int numCurvePoints = sizeof(fanCurve) / sizeof(FanCurvePoint);

// Function to get fan speed for a given temperature using linear interpolation
float getFanSpeed(float currentTemp) {
  // If temperature is below minimum, return minimum fan speed
  if (currentTemp <= pgm_read_float(&fanCurve[0].temperature)) {
    return pgm_read_float(&fanCurve[0].fanSpeed);
  }
  
  // If temperature is above maximum, return maximum fan speed
  if (currentTemp >= pgm_read_float(&fanCurve[numCurvePoints - 1].temperature)) {
    return pgm_read_float(&fanCurve[numCurvePoints - 1].fanSpeed);
  }
  
  // Find the two points to interpolate between
  for (int i = 0; i < numCurvePoints - 1; i++) {
    float temp1 = pgm_read_float(&fanCurve[i].temperature);
    float temp2 = pgm_read_float(&fanCurve[i + 1].temperature);
    
    if (currentTemp >= temp1 && currentTemp <= temp2) {
      // Linear interpolation
      float speed1 = pgm_read_float(&fanCurve[i].fanSpeed);
      float speed2 = pgm_read_float(&fanCurve[i + 1].fanSpeed);
      
      float ratio = (currentTemp - temp1) / (temp2 - temp1);
      return speed1 + ratio * (speed2 - speed1);
    }
  }
  
  // Fallback (should never reach here)
  return 0.0;
}

// Function to print the fan curve to Serial
void printFanCurve() {
  Serial.println("Fan Cooling Curve:");
  Serial.println("Temp (C)\tFan Speed");
  Serial.println("------------------------");
  
  for (int i = 0; i < numCurvePoints; i++) {
    float temp = pgm_read_float(&fanCurve[i].temperature);
    float speed = pgm_read_float(&fanCurve[i].fanSpeed);
    
    Serial.print(temp, 1);
    Serial.print("\t\t");
    Serial.println(speed, 2);
  }
  Serial.println();
}

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("\n=== Fan Curve System ===");
  
  // Initialize I2C communication
  Wire.begin();
  
  // Initialize HDC3022 sensor
  if (!hdc3022.begin(0x44, &Wire)) {
    Serial.println("ERROR: Could not find HDC3022 sensor!");
    Serial.println("Check wiring: SDA and SCL connections");
    while (1) delay(10); // Halt if sensor not found
  }
  
  Serial.println("HDC3022 sensor initialized successfully!");
  Serial.println();
  
  // Print the loaded fan curve
  printFanCurve();
  
  // Test the interpolation function with various temperatures
  Serial.println("Testing fan speed interpolation:");
  Serial.println("Temp (C)\tFan Speed");
  Serial.println("------------------------");
  
  float testTemps[] = {25, 35, 45, 55, 65, 75, 85, 95};
  for (int i = 0; i < 8; i++) {
    float temp = testTemps[i];
    float speed = getFanSpeed(temp);
    
    Serial.print(temp, 1);
    Serial.print("\t\t");
    Serial.println(speed, 2);
  }
  
  Serial.println("\n=== Starting Temperature Monitoring ===\n");
}

void loop() {
  // Read temperature and humidity from HDC3022 sensor
  double temp = 0.0;
  double RH = 0.0;
  
  hdc3022.readTemperatureHumidityOnDemand(temp, RH, TRIGGERMODE_LP0);
  
  float currentTemp = (float)temp;
  float currentHumidity = (float)RH;
  
  // Calculate the required fan speed based on current temperature
  float requiredFanSpeed = getFanSpeed(currentTemp);
  
  // Print sensor readings and fan speed
  Serial.print("Temperature: ");
  Serial.print(currentTemp, 2);
  Serial.print(" °C\t");
  
  Serial.print("Humidity: ");
  Serial.print(currentHumidity, 1);
  Serial.print(" %\t");
  
  Serial.print("Fan Speed: ");
  Serial.print(requiredFanSpeed * 100, 0);
  Serial.println(" %");
  
  // Update every 2 seconds
  delay(2000);
}
