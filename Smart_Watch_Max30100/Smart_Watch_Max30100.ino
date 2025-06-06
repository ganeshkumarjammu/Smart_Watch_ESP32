#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"



// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Wi-Fi credentials
const char* ssid = "G";
const char* password = "thinkbig";

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000);  // UTC+5:30 for IST

// EEPROM settings
#define EEPROM_SIZE 12  // Store year (4 bytes), month (1 byte), day (1 byte)
#define DATE_ADDRESS 0

// Time variables
int currentHour = 0;
int currentMinute = 0;
int currentDay = 1;
int currentMonth = 1;
int currentYear = 2023;
unsigned long lastMinuteIncrement = 0;
const unsigned long minuteInterval = 60000;  // 60 seconds
bool timeFlag = false;

// Structure to hold date for EEPROM
struct Date {
  int year;
  byte month;
  byte day;
};

// ==============Max related Variables

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

// NTP related Functions

// Function to write date to EEPROM
void writeDateToEEPROM(int year, byte month, byte day) {
  Date date = { year, month, day };
  EEPROM.put(DATE_ADDRESS, date);
  EEPROM.commit();
}

// Function to read date from EEPROM
Date readDateFromEEPROM() {
  Date date;
  EEPROM.get(DATE_ADDRESS, date);
  return date;
}

// Function to check if date has changed
bool isDateDifferent(int newYear, byte newMonth, byte newDay) {
  Date storedDate = readDateFromEEPROM();
  return (storedDate.year != newYear || storedDate.month != newMonth || storedDate.day != newDay);
}

// Function to display time and date on OLED
void displayTimeDate() {
  display.clearDisplay();
  display.setTextSize(2);  // Larger text for time
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 10);
  display.printf("%02d:%02d", currentHour, currentMinute);
  display.setTextSize(1);  // Smaller text for date
  display.setCursor(35, 30);
  display.printf("%04d-%02d-%02d", currentYear, currentMonth, currentDay);
  display.setCursor(35,50);
  display.print(F("Pressed:"));
  display.println(presenceDetected ? F("Yes") : F("No"));
  display.display();
}

// Function to update time locally
void updateLocalTime() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastMinuteIncrement >= minuteInterval) {
    currentMinute++;
    lastMinuteIncrement = currentMillis;

    if (currentMinute >= 60) {
      currentMinute = 0;
      currentHour++;

      if (currentHour >= 24) {
        currentHour = 0;
        currentDay++;

        // Simple day/month/year rollover
        if (currentDay > 31 || (currentMonth == 2 && currentDay > 28) || ((currentMonth == 4 || currentMonth == 6 || currentMonth == 9 || currentMonth == 11) && currentDay > 30)) {
          currentDay = 1;
          currentMonth++;
          if (currentMonth > 12) {
            currentMonth = 1;
            currentYear++;
          }
          // Update EEPROM with new date
          writeDateToEEPROM(currentYear, currentMonth, currentDay);
        }
      }
    }
    // Update OLED display
    if (!presenceDetected)  displayTimeDate();
    // Print to Serial for debugging
    Serial.printf("Time: %02d:%02d, Date: %d-%02d-%02d\n", currentHour, currentMinute, currentYear, currentMonth, currentDay);
  }
}

//===========> Max Related Functions
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
  // Serial.print("Temp=");
  // Serial.print(temperature);
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
  display.println(beatsPerMinute > 0 ? String((int)beatsPerMinute) : "N/A");
  display.println();
  display.setTextSize(2);
  display.print(F("SpO2:"));
  display.println(spO2 > 0 ? String((int)spO2) + "%" : "N/A");
  // display.print(F("Temp:"));
  // display.print(temperature);
  display.setTextSize(1);
  display.print(F("Pressed:"));
  display.println(presenceDetected ? F("Yes") : F("No"));
  display.display();
}



void setup() {
  Serial.begin(115200);
  Serial.println("Initializing ...");
  EEPROM.begin(EEPROM_SIZE);

  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }

  Wire.begin();
  Wire.setClock(100000);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3C for 128x64
    Serial.println("SSD1306 Connection failed");
    for (;;)
      ;  // Halt if OLED fails
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.println(F("SmartWatch"));
  display.setTextSize(1);
  display.println();
  display.println(F("Connecting to WiFi .."));
  display.display();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
  delay(1000);

  if (!pox.begin()) {
    Serial.println("GY-MAX30100 not found. Check wiring!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("GY-MAX30100 not found"));
    display.display();
    for (;;)
      ;
  }

  Serial.println("GY-MAX30100 initialized");
  pox.setIRLedCurrent(MAX30100_LED_CURR_50MA);

  // Initialize NTPClient
  timeClient.begin();
  timeClient.update();

  // Get date and time from NTP
  time_t epochTime = timeClient.getEpochTime();
  struct tm* ptm = gmtime(&epochTime);
  int ntpYear   = ptm->tm_year + 1900;
  byte ntpMonth = ptm->tm_mon + 1;
  byte ntpDay   = ptm->tm_mday;
  currentHour   = timeClient.getHours();
  currentMinute = timeClient.getMinutes();
  currentYear = ntpYear;
  currentMonth = ntpMonth;
  currentDay = ntpDay;

  // Check and update EEPROM if date is different
  if (isDateDifferent(ntpYear, ntpMonth, ntpDay)) {
    writeDateToEEPROM(ntpYear, ntpMonth, ntpDay);
    Serial.println("Date updated in EEPROM");
  } else {
    Serial.println("Date unchanged, no EEPROM update");
  }

  // Display initial time and date
  displayTimeDate();
  Serial.printf("Initial Time: %02d:%02d, Date: %d-%02d-%02d\n", currentHour, currentMinute, currentYear, currentMonth, currentDay);

  // Disconnect Wi-Fi to save power
  // WiFi.disconnect(true);
  // WiFi.mode(WIFI_OFF);

  // Set last increment time
  lastMinuteIncrement = millis()-30000;
}

void loop() {
  // readTemperature();
  readSpO2();
  readPresence();
  readHeartRate();
  readRawSignals();
  updateLocalTime();
  if ((millis() - lastUpdate >= updateInterval)){
    if (presenceDetected)
    {
       printData();
      timeFlag = true;
    }
    else if(timeFlag){
      displayTimeDate();
      timeFlag = false;
    }
    lastUpdate = millis();
  }
  delay(10);
}