#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // UTC+5:30 for IST

// EEPROM settings
#define EEPROM_SIZE 12 // Store year (4 bytes), month (1 byte), day (1 byte)
#define DATE_ADDRESS 0

// Time variables
int currentHour = 0;
int currentMinute = 0;
int currentDay = 1;
int currentMonth = 1;
int currentYear = 2023;
unsigned long lastMinuteIncrement = 0;
const unsigned long minuteInterval = 60000; // 60 seconds

// Structure to hold date for EEPROM
struct Date {
  int year;
  byte month;
  byte day;
};

// Function to write date to EEPROM
void writeDateToEEPROM(int year, byte month, byte day) {
  Date date = {year, month, day};
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
  display.setTextSize(2); // Larger text for time
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.printf("%02d:%02d", currentHour, currentMinute);

  display.setTextSize(1); // Smaller text for date
  display.setCursor(10, 40);
  display.printf("%04d-%02d-%02d", currentYear, currentMonth, currentDay);

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
        if (currentDay > 31 || (currentMonth == 2 && currentDay > 28) || 
            ((currentMonth == 4 || currentMonth == 6 || currentMonth == 9 || currentMonth == 11) && currentDay > 30)) {
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
    displayTimeDate();
    // Print to Serial for debugging
    Serial.printf("Time: %02d:%02d, Date: %d-%02d-%02d\n", currentHour, currentMinute, currentYear, currentMonth, currentDay);
  }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println("SSD1306 allocation failed");
    for (;;); // Halt if OLED fails
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  // Initialize NTPClient
  timeClient.begin();
  timeClient.update();

  // Get date and time from NTP
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime(&epochTime);
  int ntpYear = ptm->tm_year + 1900;
  byte ntpMonth = ptm->tm_mon + 1;
  byte ntpDay = ptm->tm_mday;
  currentHour = timeClient.getHours();
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
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Set last increment time
  lastMinuteIncrement = millis();
}

void loop() {
  updateLocalTime();
}