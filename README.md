# ESP32BrewREMS

# Documentation
I dont know why, but it's been a real pain to find documentation for the ESP32 device. I'm using a HiLetgo ESP32 OLED WiFi Kit V3. This is apparently really a product by Heltec: https://heltec.org/project/wifi-kit32-v3/

USB-Serial Driver: https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers

This library seems to be really important https://github.com/HelTecAutomation/Heltec_ESP32


None of their links work, so I'm going to try to scrape together what I can find:  
## Specifications
* Master Chip  ESP32-S3FN8(Xtensa®32-bit lx7 dual core processor)  
* USB to Serial Chip  CP2102  
* WiFi  802.11 b/g/n, up to 150Mbps  
* Bluetooth LE  Bluetooth 5, Bluetooth mesh  
* Hardware Resource  7*ADC1+2*ADC2; 7*Touch; 3*UART; 2*I2C; 2*SPI, etc.  
* Memory  384KB ROM;  512KB SRAM;  16KB RTC SRAM;  8MB SiP Flash  
* 2*1.25 lithium battery interface;  2*18*2.54 Header Pin  
* Battery  3.7V lithium battery power supply and charging  

![image](https://github.com/user-attachments/assets/c54abfae-7d97-4f00-948b-bfbb6146a0cd)

### Encoder Input
I should have looked at the spec sheet. I think the circuit is close w/ the input pullup:
https://www.digikey.com/en/products/detail/bourns-inc/PEC11H-4220F-S0024/12349532
![image](https://github.com/user-attachments/assets/af4e54f5-55e1-4cca-9401-f7a91b8d3811)

ledc is a little weird but we're trying to use if for power control (https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html).
The range of settings is limited between freq and resolution.
Run this script to get the output https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/AnalogOut/ledcFrequency/ledcFrequency.ino
```
Bit resolution | Min Frequency [Hz] | Max Frequency [Hz]
             1 |              19532 |           20039138
             2 |               9766 |           10019569
             3 |               4883 |            5009784
             4 |               2442 |            2504892
             5 |               1221 |            1252446
             6 |                611 |             626223
             7 |                306 |             313111
             8 |                153 |             156555
             9 |                 77 |              78277
            10 |                 39 |              39138
            11 |                 20 |              19569
            12 |                 10 |               9784
            13 |                  5 |               4892
            14 |                  3 |               2446
```
