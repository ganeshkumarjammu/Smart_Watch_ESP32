#include <Wire.h>
#include "MAX30105.h"
MAX30105 particleSensor;
#define MAX30102_I2C_ADDRESS 0x57
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);
  if (!particleSensor.begin(Wire, 100000, MAX30102_I2C_ADDRESS)) {
    Serial.println("MAX30102 not found. Check wiring!");
    for (;;);
  }
  Serial.println("MAX30102 initialized");
  particleSensor.setup(100, 4, 4, 100, 411, 16384);
  particleSensor.setPulseAmplitudeRed(0x1F);
}
void loop() {
  long irValue = particleSensor.getIR();
  Serial.print("IR=");
  Serial.println(irValue);
  delay(100);
}
