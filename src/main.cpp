#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_HDC302x.h>
#include <Adafruit_Sensor.h>

// PWM pin for fan control via MOSFET
const int FAN_PWM_PIN = 12;  // D12 connected to MOSFET gate

// HDC3022 sensor object
Adafruit_HDC302x hdc3022 = Adafruit_HDC302x();

// Temperature/fan speed data points
struct FanCurvePoint {
  float temperature;  // Temperature in Celsius
  float fanSpeed;     // Fan speed from 0.0 to 1.0 (0% to 100%)
};

// Fan curve data - loaded from CSV at startup
#define MAX_CURVE_POINTS 20  // Maximum number of curve points
FanCurvePoint fanCurve[MAX_CURVE_POINTS];
int numCurvePoints = 0;  // Actual number of points loaded

// CSV data embedded in program memory (from fan_curve.csv)
const char csvData[] PROGMEM = 
"30,0.00\n"
"40,0.20\n"
"50,0.35\n"
"60,0.50\n"
"70,0.70\n"
"80,0.85\n"
"90,1.00\n";

// Parse CSV data and load into fanCurve array
void loadFanCurveFromCSV() {
  char buffer[256];
  strcpy_P(buffer, csvData);  // Copy CSV data from program memory
  
  char* line = strtok(buffer, "\n");
  numCurvePoints = 0;
  
  // read each line of CSV
  while (line != NULL && numCurvePoints < MAX_CURVE_POINTS) {
    
    // read temperature and fan speed from CSV line
    float temp, speed;
    if (sscanf(line, "%f,%f", &temp, &speed) == 2) {
      fanCurve[numCurvePoints].temperature = temp;
      fanCurve[numCurvePoints].fanSpeed = speed;
      numCurvePoints++;
    }
    
    line = strtok(NULL, "\n");
  }
  
  Serial.print("Loaded ");
  Serial.print(numCurvePoints);
  Serial.println(" fan curve points from CSV");
}

// Get fan speed for a given temperature using linear interpolation
float getFanSpeed(float currentTemp) {
  if (numCurvePoints == 0) return 0.0;  // No curve loaded
  
  // If temperature is below minimum, return minimum fan speed
  if (currentTemp <= fanCurve[0].temperature) {
    return fanCurve[0].fanSpeed;
  }
  
  // If temperature is above maximum, return maximum fan speed
  if (currentTemp >= fanCurve[numCurvePoints - 1].temperature) {
    return fanCurve[numCurvePoints - 1].fanSpeed;
  }
  
  // Find the two points closest points 
  for (int i = 0; i < numCurvePoints - 1; i++) {
    float temp1 = fanCurve[i].temperature;
    float temp2 = fanCurve[i + 1].temperature;
    
    if (currentTemp >= temp1 && currentTemp <= temp2) {
      // midpoint between two points
      float speed1 = fanCurve[i].fanSpeed;
      float speed2 = fanCurve[i + 1].fanSpeed;
      
      float ratio = (currentTemp - temp1) / (temp2 - temp1);
      return speed1 + ratio * (speed2 - speed1);
    }
  }
  
  // Fallback 
  return 0.0;
}

// Set the fan speed using PWM (0.0 to 1.0 range)
void setFanSpeed(float speed) {
  // Clamp speed between 0.0 and 1.0
  if (speed < 0.0) speed = 0.0;
  if (speed > 1.0) speed = 1.0;
  
  // Convert speed (0.0-1.0) to PWM value (0-255)
  int pwmValue = (int)(speed * 255);
  
  // Write PWM signal to MOSFET gate
  analogWrite(FAN_PWM_PIN, pwmValue);
}

// Print the loaded fan curve to Serial
void printFanCurve() {
  Serial.println("Fan Cooling Curve:");
  Serial.println("Temp (C)\tFan Speed");
  Serial.println("------------------------");
  
  for (int i = 0; i < numCurvePoints; i++) {
    Serial.print(fanCurve[i].temperature, 1);
    Serial.print("\t\t");
    Serial.println(fanCurve[i].fanSpeed, 2);
  }
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }
  
  Serial.println("\n=== Fan Curve System ===");
  
  // Load fan curve from CSV data
  loadFanCurveFromCSV();
  
  // Configure PWM pin for fan control
  pinMode(FAN_PWM_PIN, OUTPUT);
  setFanSpeed(0.0);  

  Wire.begin();
  
  // Initialize  sensor
  if (!hdc3022.begin(0x44, &Wire)) {
    Serial.println("ERROR: Could not find HDC3022 sensor!");
    Serial.println("Check wiring: SDA and SCL connections");
    while (1) delay(10); 
  }
  
  Serial.println("HDC3022 sensor initialized successfully!");
  Serial.println();
  
  // Print the loaded fan curve
  printFanCurve();
  
  // Test 
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
  // Read temperature and humidity from  sensor
  double temp = 0.0;
  double RH = 0.0;
  
  hdc3022.readTemperatureHumidityOnDemand(temp, RH, TRIGGERMODE_LP0);
  
  float currentTemp = (float)temp;
  float currentHumidity = (float)RH;
  
  // Calculate the required fan speed based on current temperature
  float requiredFanSpeed = getFanSpeed(currentTemp);
  
  // Control the fan via PWM based on calculated speed
  setFanSpeed(requiredFanSpeed);
  
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
  
  // Update every 2 sxeconds
  delay(2000);
}
