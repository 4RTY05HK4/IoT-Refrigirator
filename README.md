# IoT-Refrigirator
My project for an IoT Refrigirator. It is based on a ESP-32 with DS18B20 temperature sensors and magnetic contactrons.

## List of components I used
- ESP-WROOM-32 DevKit
- CMD918 magnetic contactrons
- 4.7kOhm resistors
- 10uF capacitor
- LCD 2x16 with I2C converter
- RTC module DS3231 with I2C
- DS18B20 temperature sensors
- YMD12065G buzzer


## Important libraries
For this code to work you'll need a few additional libraries not normally included in ArduinoIDE:
- RTClib
- LiquidCrystal_I2C
- ESP32Ping
