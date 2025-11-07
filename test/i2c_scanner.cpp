// I2C Scanner - use this to verify your HDC3022 is connected
// Upload this to check if the sensor is detected at address 0x44

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(9600);
  while (!Serial);
  
  Serial.println("\n=== I2C Scanner ===");
  Serial.println("Scanning for I2C devices...");
  
  Wire.begin();
  
  byte count = 0;
  
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("I2C device found at address 0x");
      if (i < 16) Serial.print("0");
      Serial.print(i, HEX);
      Serial.println(" !");
      
      if (i == 0x44) {
        Serial.println("  ^ This is your HDC3022 sensor!");
      }
      
      count++;
    }
  }
  
  if (count == 0) {
    Serial.println("\nNO I2C devices found!");
    Serial.println("\nCheck your wiring:");
    Serial.println("  - VIN/VDD -> 3.3V or 5V");
    Serial.println("  - GND -> GND");
    Serial.println("  - SDA -> A4");
    Serial.println("  - SCL -> A5");
  } else {
    Serial.print("\nFound ");
    Serial.print(count);
    Serial.println(" device(s)");
  }
  
  Serial.println("\nScan complete!");
}

void loop() {
  // Nothing to do here
}
