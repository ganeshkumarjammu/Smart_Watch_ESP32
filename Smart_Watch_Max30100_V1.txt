#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// GY-MAX30100 sensor settings
PulseOximeter pox;
#define MAX30100_I2C_ADDRESS 0x57
#define OLED_I2C_ADDRESS 0x3C
#define TEMP_EN_REG 0x14
#define TEMP_INT_REG 0x16
#define TEMP_FRAC_REG 0x17

// Measurement variables
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;
float temperature;
float spO2;
bool presenceDetected = false;

// Timing for updates
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;

// Check I2C devices with retries
byte checkI2CDevices() {
  const byte maxRetries = 3;
  for (byte retry = 1; retry <= maxRetries; retry++) {
    Wire.beginTransmission(MAX30100_I2C_ADDRESS);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.println("GY-MAX30100 detected at 0x57 on Wire");
      break;
    }
    Serial.print("Attempt ");
    Serial.print(retry);
    Serial.print(": GY-MAX30100 not detected at 0x57, error: ");
    Serial.println(error);
    delay(100);
    if (retry == maxRetries) {
      Serial.println("ERROR: GY-MAX30100 failed after retries. Check wiring/pull-ups!");
      return 0;
    }
  }

  byte oledAddress = OLED_I2C_ADDRESS;
  Wire.beginTransmission(oledAddress);
  if (Wire.endTransmission() != 0) {
    oledAddress = 0x3D;
    Wire.beginTransmission(oledAddress);
    if (Wire.endTransmission() != 0) {
      Serial.println("ERROR: OLED not detected at 0x3C or 0x3D on Wire");
      return 0;
    }
    Serial.println("OLED detected at 0x3D on Wire");
  } else {
    Serial.println("OLED detected at 0x3C on Wire");
  }
  return oledAddress;
}

// Read temperature from GY-MAX30100 registers
void readTemperature() {
  Wire.beginTransmission(MAX30100_I2C_ADDRESS);
  Wire.write(TEMP_EN_REG);
  Wire.write(0x01);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: Failed to enable temperature");
    temperature = 0;
    return;
  }
  delay(30);
  Wire.beginTransmission(MAX30100_I2C_ADDRESS);
  Wire.write(TEMP_INT_REG);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: Failed to set temperature register");
    temperature = 0;
    return;
  }
  Wire.requestFrom(MAX30100_I2C_ADDRESS, 1);
  if (Wire.available()) {
    int8_t tempInt = Wire.read();
    Wire.beginTransmission(MAX30100_I2C_ADDRESS);
    Wire.write(TEMP_FRAC_REG);
    if (Wire.endTransmission() != 0) {
      Serial.println("ERROR: Failed to read fractional register");
      temperature = tempInt;
      return;
    }
    Wire.requestFrom(MAX30100_I2C_ADDRESS, 1);
    if (Wire.available()) {
      uint8_t tempFrac = Wire.read();
      temperature = tempInt + (tempFrac * 0.0625);
    } else {
      Serial.println("ERROR: No fractional data available");
      temperature = tempInt;
    }
  } else {
    Serial.println("ERROR: No integer data available");
    temperature = 0;
  }
}

// Read SpO2
void readSpO2() {
  pox.update();
  spO2 = pox.getSpO2();
  if (spO2 < 50 || spO2 > 100) spO2 = 0;
}

// Read presence (using BPM/SpO2 as proxy)
void readPresence() {
  pox.update();
  presenceDetected = (pox.getHeartRate() > 30 || pox.getSpO2() > 50);
}

// Read heart rate
void readHeartRate() {
  pox.update();
  float bpm = pox.getHeartRate();
  if (bpm > 30 && bpm < 200) {
    if (millis() - lastBeat > 300) {
      beatsPerMinute = bpm;
      Serial.print("Beat detected, Instant BPM=");
      Serial.println(beatsPerMinute);
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
      lastBeat = millis();
    }
  }
  if (millis() - lastBeat > 5000) {
    beatAvg = 0;
    rateSpot = 0;
    for (byte i = 0; i < RATE_SIZE; i++) {
      rates[i] = 0;
    }
    beatsPerMinute = 0;
  }
}

// Placeholder for raw signals
void readRawSignals() {
  // No IR/Red data available
}

// Print data to OLED and Serial
void printData() {
  Serial.print("Temp=");
  Serial.print(temperature);
  Serial.print(", SpO2=");
  Serial.print(spO2);
  Serial.print("%, Instant BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.print(", Presence=");
  Serial.println(presenceDetected ? "Yes" : "No");

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print(F("BPM:"));
  display.println(beatAvg > 0 ? String(beatAvg) : "N/A");
  display.setTextSize(1);
  display.print(F("Inst:"));
  display.print(beatsPerMinute > 0 ? String((int)beatsPerMinute) : "N/A");
  display.print(F(" SpO2:"));
  display.println(spO2 > 0 ? String((int)spO2) + "%" : "N/A");
  display.print(F("Temp:"));
  display.print(temperature);
  display.print(F("C Pres:"));
  display.println(presenceDetected ? F("Yes") : F("No"));
  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing GY-MAX30100...");

  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }

  Wire.begin();
  Wire.setClock(100000);

  byte oledAddress = checkI2CDevices();
  if (oledAddress == 0) {
    for (;;);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, oledAddress)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Initializing..."));
  display.display();
  delay(1000);

  if (!pox.begin()) {
    Serial.println("GY-MAX30100 not found. Check wiring!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("GY-MAX30100 not found"));
    display.display();
    for (;;);
  }
  Serial.println("GY-MAX30100 initialized");
  pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);
}

void loop() {
  readTemperature();
  readSpO2();
  readPresence();
  readHeartRate();
  readRawSignals();

  if (millis() - lastUpdate >= updateInterval) {
    printData();
    lastUpdate = millis();
  }

  delay(10);
}