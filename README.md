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

