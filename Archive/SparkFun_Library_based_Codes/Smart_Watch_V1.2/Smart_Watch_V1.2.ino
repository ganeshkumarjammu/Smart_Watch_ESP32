#include <Wire.h>
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "heartRate.h"
#include "MAX30105.h"


// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
TwoWire Wire2 = TwoWire(1); // Second I2C bus
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, OLED_RESET);

// MAX30102 sensor settings
MAX30105 particleSensor;
#define MAX30102_I2C_ADDRESS 0x57
#define OLED_I2C_ADDRESS 0x3C // Try 0x3D if 0x3C fails
#define OLED_SDA 33
#define OLED_SCL 32

// Heart rate calculation variables
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

// Measurement variables
float temperature;
bool presenceDetected = false;

// Timing for updates
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 1000;

// Function to check I2C devices
byte checkI2CDevices() {


  byte oledAddress = OLED_I2C_ADDRESS;
  Wire1.beginTransmission(oledAddress);
  if (Wire1.endTransmission() != 0) {
    oledAddress = 0x3D;
    Wire1.beginTransmission(oledAddress);
    if (Wire1.endTransmission() != 0) {
      Serial.println("ERROR: OLED not detected at 0x3C or 0x3D on Wire1");
      return 0;
    }
    Serial.println("OLED detected at 0x3D on Wire1");
  } else {
    Serial.println("OLED detected at 0x3C on Wire1");
  }

  Wire.beginTransmission(MAX30102_I2C_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println("ERROR: MAX30102 not detected at 0x57 on Wire");
    return 0;
  }
  Serial.println("MAX30102 detected at 0x57 on Wire");

  return oledAddress;
}

// Function to read temperature
void readTemperature() {
  temperature = particleSensor.readTemperature();
}

// Function to read presence
void readPresence(long irValue) {
  presenceDetected = (irValue > 50000);
}

// Function to read heart rate
void readHeartRate(long irValue) {
  if (checkForBeat(irValue)) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60000.0 / delta;
    Serial.print("Beat detected, Instant BPM=");
    Serial.println(beatsPerMinute);
    if (beatsPerMinute < 200 && beatsPerMinute > 30) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }
  // Reset if no beat for 5 seconds
  if (millis() - lastBeat > 5000) {
    beatAvg = 0;
    rateSpot = 0;
    for (byte i = 0; i < RATE_SIZE; i++) {
      rates[i] = 0;
    }
    beatsPerMinute = 0;
  }
}

// Function to print data to OLED and Serial
void printData(long irValue) {
  // Serial output
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", Temp=");
  Serial.print(temperature);
  Serial.print(", Instant BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.print(", Presence=");
  Serial.println(presenceDetected ? "Yes" : "No");

  // OLED display
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.print(F("BPM: "));
  display.println(beatAvg > 0 ? String(beatAvg) : "N/A");
  display.setTextSize(1);
  display.println();
  display.print(F("Inst BPM: "));
  display.println(beatsPerMinute > 0 ? String((int)beatsPerMinute) : "N/A");
  display.print(F("Temp: "));
  display.print(temperature);
  display.println(F(" C"));
  display.print(F("Presence: "));
  display.println(presenceDetected ? F("Yes") : F("No"));

  // Plot IR signal
  static int plotY[SCREEN_WIDTH];
  static int plotIndex = 0;
  if (presenceDetected) {
    int plotValue = map(irValue, 50000, 100000, 0, SCREEN_HEIGHT / 3);
    plotY[plotIndex] = constrain(plotValue, 0, SCREEN_HEIGHT / 3 - 1);
    for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
      int x0 = i;
      int x1 = i + 1;
      int y0 = SCREEN_HEIGHT - plotY[i];
      int y1 = SCREEN_HEIGHT - plotY[(i + 1) % SCREEN_WIDTH];
      display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }
    plotIndex = (plotIndex + 1) % (SCREEN_WIDTH - 1);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize rates array
  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }

  // Initialize I2C buses
  Wire.begin(); // I2C0 for MAX30102 (GPIO 21 SDA, 22 SCL)
  Wire.setClock(100000);
  Wire1.begin(OLED_SDA, OLED_SCL); // I2C1 for OLED (GPIO 33 SDA, 32 SCL)
  Wire1.setClock(100000);

  // Check I2C devices
  byte oledAddress = checkI2CDevices();
  if (oledAddress == 0) {
    for (;;);
  }

  // Initialize OLED
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

  // Initialize MAX30102
  if (!particleSensor.begin(Wire, 100000, MAX30102_I2C_ADDRESS)) {
    Serial.println("MAX30102 not found. Check wiring!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("MAX30102 not found"));
    display.display();
    for (;;);
  }

  // Configure sensor
  particleSensor.setup(100, 4, 4, 100, 411, 16384);
  particleSensor.setPulseAmplitudeRed(0x1F);
  particleSensor.setPulseAmplitudeGreen(0); // Green LED off
  particleSensor.enableDIETEMPRDY();
}

void loop() {
  // Read sensor data
  long irValue = particleSensor.getIR();

  // Read measurements
  readTemperature();
  readPresence(irValue);
  readHeartRate(irValue);

  // Update display and serial every second
  if (millis() - lastUpdate >= updateInterval) {
    printData(irValue);
    lastUpdate = millis();
  }

  delay(10);
}
