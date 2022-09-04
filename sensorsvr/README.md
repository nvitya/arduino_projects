# SENSORSVR Project

This project is made by Sloeber IDE, the Arduino IDE based on Eclipse.

Sloeber IDE configuration:
  - At the Preferences/Arduino/Third Party Index URL-s add this:
    ```https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json```
  - In the Preferences/Arduino/Platforms and Boards select the ESP32 v1.0.6
    (The v2.0+ have an upload problem, which can be fixed editing the platform.txt. The v1.0.6 compiles faster)

Import this project into the Sloeber IDE.

HW Requirements:
  - ESP32 Development Kit
