# Smart Watch Project

## 1. Project Introduction
The **Smart Watch Project** is an Arduino-based wearable prototype built with an **ESP32** microcontroller, a **GY-MAX30100** pulse oximeter sensor, and an **SSD1306 OLED** display (128x64). It displays real-time **heart rate (BPM)**, **oxygen saturation (SpO2)**, **date**, and **time** (synchronized via NTP for IST, UTC+5:30) on the OLED. The system stores date/time in **EEPROM** for offline use and updates time using `millis()` when disconnected from WiFi. A "Smart Watch" splash screen appears for 2 seconds on boot. Designed for educational and hobbyist purposes, this project demonstrates I2C sensor integration, NTP time syncing, and persistent storage, serving as a foundation for health-monitoring wearables.

## 2. Components
### Hardware
- **ESP32 Dev Module**: Microcontroller with WiFi for processing and NTP syncing.
- **GY-MAX30100 Sensor**: Pulse oximeter for BPM and SpO2 (I2C address: 0x57).
- **SSD1306 OLED Display**: 128x64 I2C display for data visualization (address: 0x3C or 0x3D).
- **Wires and Breadboard**: For prototyping connections.
- **3.3V Power Supply**: Powers ESP32 and sensors (avoid 5V).

### Software
- **Arduino IDE**: Development environment for coding and uploading.
- **ESP32 Board Support**: Enables ESP32 compatibility in Arduino IDE.
- **Libraries**: Listed below with authors and links.

### Connection Table
| Component    | Pin | ESP32 Pin |
| ------------ | --- | --------- |
| GY-MAX30100  | VIN | 3.3V      |
|              | GND | GND       |
|              | SDA | GPIO 21   |
|              | SCL | GPIO 22   |
| SSD1306 OLED | VCC | 3.3V      |
|              | GND | GND       |
|              | SDA | GPIO 21   |
|              | SCL | GPIO 22   |

**Note**: Both devices share the I2C bus (SDA/SCL). GY-MAX30100 includes 4.7kΩ pull-ups; add 10kΩ pull-ups if I2C is unstable. Use 3.3V power to avoid damage.

## 3. Libraries - Authors - Links
Install via Arduino Library Manager:
- **MAX30100_PulseOximeter** (~1.2.0) or (~1.2.2).
  - **Author**: Oxullo Intersecans
  - **Link**: [https://github.com/oxullo/Arduino-MAX30100](https://github.com/oxullo/Arduino-MAX30100)
  - **Purpose**: Reads BPM and SpO2 from GY-MAX30100.
- **Adafruit_GFX** (~1.11.9)
  - **Author**: Adafruit
  - **Link**: [https://github.com/adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
  - **Purpose**: Graphics library for OLED rendering.
- **Adafruit_SSD1306** (~2.5.7)
  - **Author**: Adafruit
  - **Link**: [https://github.com/adafruit/Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
  - **Purpose**: SSD1306 OLED driver.
- **NTPClient** (~3.2.1)
  - **Author**: Fabrice Weinberg
  - **Link**: [https://github.com/arduino-libraries/NTPClient](https://github.com/arduino-libraries/NTPClient)
  - **Purpose**: Syncs time via NTP.
- **WiFi** (ESP32 core)
  - **Author**: Espressif Systems
  - **Link**: [https://github.com/espressif/arduino-esp32](https://github.com/espressif/arduino-esp32)
  - **Purpose**: WiFi connectivity for ESP32
- **ESP Async Webserver:**
  - **Author**: Espressif Systems
  - **Link**: https://github.com/ESP32Async/ESPAsyncWebServer
  - **Purpose**: WiFi connectivity for ESP32.
  - **References:** https://github.com/me-no-dev/ESPAsyncWebServer?tab=readme-ov-file
- **EEPROM** (Arduino core)
  - **Author**: Arduino
  - **Link**: [https://www.arduino.cc/en/Reference/EEPROM](https://www.arduino.cc/en/Reference/EEPROM)
  - **Purpose**: Stores date/time persistently.
**Other libraries for expansion :**
 - **ESPAsyncWebServer**: Search “ESP Async WebServer”, install by Hristo Gochkov (~1.2.3).
-  **AsyncTCP**: Search “AsyncTCP”, install by Hristo Gochkov (~1.1.1).

## 4. Installation
1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Add ESP32 board support:
   - Follow [Espressif’s guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
   - In Arduino IDE, go to File > Preferences, add `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` to Additional Boards Manager URLs.
   - Install “esp32” by Espressif via Boards Manager.
3. Install libraries via Arduino Library Manager (Sketch > Include Library > Manage Libraries).
4. Remove conflicting libraries:
   - Delete `C:\Users\<YourUsername>\Documents\Arduino\libraries\SPI-master`.
   - Delete `C:\Users\<YourUsername>\Documents\Arduino\libraries\Pulse_Oximeter` (if present).
5. Connect hardware as per the connection table.
6. Update `ssid` and `password` in the code for your WiFi network.
7. Upload the code to the ESP32 via Arduino IDE (select “ESP32 Dev Module” as the board).

## 5. Code
The main code (`ESP32_SmartWatch.ino`) initializes the GY-MAX30100 and SSD1306, syncs time via NTP, stores date/time in EEPROM, and displays date, time, instant BPM, average BPM, and SpO2 on the OLED with a 2-second "Smart Watch" splash screen.  Open Code Folder for more to look into to code.

Main Code :   [Click Here](./Smart_Watch_Max30100/Smart_Watch_Max30100.ino)
### Limitations
- **No Temperature Sensor**: The GY-MAX30100 lacks a temperature sensor (unlike MAX30102). Adding a DS18B20 or similar is required for temperature monitoring.
- **SpO2 and BPM Accuracy**: Readings are approximate due to sensor and library limitations; not suitable for medical use. Proper finger placement is critical.
- **WiFi Dependency**: Time syncing requires WiFi. Offline mode relies on EEPROM or `millis()`, which may drift over time.
- **No User Input**: Lacks buttons or tap gestures (e.g., no "Hello Buddy" or LED toggle from earlier versions).
- **Power Supply**: Uses 3.3V USB power; no battery management for portability.
- **I2C Stability**: Shared I2C bus may require additional pull-ups if communication is unreliable.

## 6. Troubleshooting
Below are test codes to diagnose common issues. Save each as a separate `.ino` file and upload to the ESP32.

### I2C Device Not Detected
- **Symptoms**: "GY-MAX30100 failed" or "SSD1306 failed" on Serial.
```x-arduino
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Device at 0x");
      Serial.println(i, HEX);
    }
  }
}
void loop() {}
```
- **Expected Output**: Devices at 0x57 (MAX30100), 0x3C/0x3D (OLED).
- **Fix**: Check wiring, ensure 4.7kΩ pull-ups, verify 3.3V power.

### No BPM/SpO2 Readings
- **Symptoms**: BPM/SpO2 show 0 or "N/A" on OLED/Serial.
```x-arduino
#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
PulseOximeter pox;
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!pox.begin()) {
    Serial.println("Sensor failed");
    while (true);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURRENT_50MA);
}
void loop() {
  pox.update();
  Serial.print("BPM: ");
  Serial.print(pox.getHeartRate());
  Serial.print(", SpO2: ");
  Serial.println(pox.getSpO2());
  delay(100);
}
```
- **Expected Output**: BPM ~60–100, SpO2 ~90–100% with finger on sensor.
- **Fix**: Ensure finger placement, check I2C wiring, reinstall library.

### Time/Date Not Syncing
- **Symptoms**: "N/A" for time/date or incorrect values.
```x-arduino
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  timeClient.begin();
}
void loop() {
  if (timeClient.update()) {
    Serial.println(timeClient.getFormattedTime());
  }
  delay(1000);
}
```
- **Expected Output**: Time in HH:MM:SS (e.g., 14:29:00 IST).
- **Fix**: Verify WiFi credentials, ensure internet access.

### OLED Not Displaying
- **Symptoms**: Blank OLED screen.
```x-arduino
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void setup() {
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("OLED Test"));
  display.display();
}
void loop() {}
```
- **Expected Output**: “OLED Test” on OLED.
- **Fix**: Check I2C address (0x3C/0x3D), verify wiring.

### EEPROM Issues
- **Symptoms**: Time/date not saved or loaded correctly.
```x-arduino
#include <EEPROM.h>
void setup() {
  Serial.begin(115200);
  EEPROM.begin(19);
  char timeStr[] = "12:34:56";
  for (byte i = 0; i < 8; i++) EEPROM.write(11 + i, timeStr[i]);
  EEPROM.commit();
  char eepromTime[9];
  for (byte i = 0; i < 8; i++) eepromTime[i] = EEPROM.read(11 + i);
  eepromTime[8] = '\0';
  Serial.println(eepromTime);
}
void loop() {}
```
- **Expected Output**: `12:34:56`.
- **Fix**: Ensure EEPROM size (19 bytes), check ESP32 flash integrity.

## 7. Notes
- **Power Supply**: Use 3.3V (not 5V) to avoid damaging GY-MAX30100 or SSD1306.
- **BPM/SpO2 Accuracy**: Requires proper finger placement. Readings are not medical-grade.
- **WiFi Configuration**: Update `ssid` and `password` in the code before uploading.
- **No Temperature**: GY-MAX30100 lacks a temperature sensor. For temperature, add a DS18B20 (request code if needed).
- **Previous Issues**: Avoided **SparkFun MAX30105** due to errors (e.g., no `getHeartRate()`). Can revert for IR data if required.
- **Timekeeping**: NTP sync requires WiFi; EEPROM and `millis()` handle offline mode but may drift.
- **Extensibility**: Add tap gestures, LED toggles, or battery power for enhanced functionality.
- **Expected Output** (at 14:29 IST, June 6, 2025):
  - **Serial**:
    ```
    Initializing Smart Watch...
    GY-MAX30100 detected at 0x57
    OLED detected at 0x3C
    WiFi connected: 192.168.1.100
    Saving date: 2025-06-06
    Saving time: 14:29:00
    Instant BPM=82.3, Avg BPM=82, SpO2=94%, Date=2025-06-06, Time=14:29:00
    ```
  - **OLED**:
    - Splash: “Smart Watch” (2s).
    - Main:
      ```
      Date:2025-06-06
      Time:14:29:00
      BPM:82
      Inst:82 SpO2:94%
      ```

## License
MIT License. See [LICENSE](LICENSE) for details.

---

### Additional Notes
- **Code Updates**: Fixed `¤tTimeStruct` typos in `updateTimeDate()` (replaced with `currentTimeStruct`) from previous versions to ensure compilation.
- **Library Choice**: Used **MAX30100_PulseOximeter** for simplicity and reliability over **SparkFun MAX30105**, which required custom BPM/SpO2 algorithms.
- **Limitations Addressed**: Highlighted the absence of temperature sensing and potential for I2C instability.
- **Connection Table**: Matches your hardware setup (GPIO 21/22 for I2C, 3.3V power).
 
 **Future Goal** :
  > - Try make this library better and also integrated max10300 library by Oxullo and sparkfun library and make a own library and easy library.
  > - Use IR  Capability to detect the presence for detecting the movement of hand or pressed or not.
  > - Use MAX30102 not GY but RCWL model and write the same code for it and also make it stable like this.
  > - Add Webpage and show the values on the webpage too.
  
  
### Next Steps
1. Install libraries as per the README.
2. Update `ssid` and `password` in `ESP32_SmartWatch.ino`.
3. Connect hardware per the connection table.
4. Upload the main code.
5. Test OLED display and Serial output (expect ~14:29 IST, June 6, 2025).
6. If issues arise:
   - Run troubleshooting codes.
   - Share Serial output and symptoms.
   - Specify if you need features like temperature (via DS18B20), tap gestures, or IR data (via MAX30105).
7. Save the README and test codes for reference.

