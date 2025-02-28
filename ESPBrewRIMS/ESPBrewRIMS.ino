#include <Wire.h>
#include "HT_SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
//#include "HT_DisplayUi.h"

/*
EPP32 HiLego Device (Amazon https://www.amazon.com/dp/B07DKD79Y9?ref=ppx_yo2ov_dt_b_fed_asin_title)
Inputs:
2 PT100 sensors, 1 for mash-tun and 1 for RIMS system
1 Power sensor on the AC to limit heater power (Amazon https://www.amazon.com/dp/B0C33L14LY?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1)
  This will produce 1V at 20A so resolution at 10 bit is 20mA
Encoder or push button to change temp or power settings?


Have a 4 pos switch that will select different modes for the RIMS system
https://www.amazon.com/dp/B09P9T86TT?ref=ppx_yo2ov_dt_b_fed_asin_title
0) OFF
1) Heating water, for mash in does not regulate MAXTEMP of RIMS system
2) Mashing - Dont want to denature enzymes so need to regulate the max temp in REMS as well
3) ??
*/

const float POWHEATER = 1650.; // Watts; the power of the heater
const float POWERLIMIT = 12*120; // Watts - Limit power to 12A

static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst
//DisplayUi ui( &display );

// These are parameter that are set by users in 

float HEATERPOWER = 100; // Percent of POWERLIMIT
float MASHSETTEMP = 150.; // Set point for mash temperature in F
float MAXRIMSTEMP = 160.; // Max temp at output of the RIMS heater
int MODE = 0; // What position is the selector in

float mashtemp = 0;
float rimstemp = 0;


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  VextON();
  delay(100);

  // Initialising the UI will init the display too.
  display.init();

  display.setFont(ArialMT_Plain_10);  
}

void update_display() {
  display.clear();
  display.drawString(5, 0, "Mode "+ String(MODE));
  switch (MODE) {
    case 0:
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(123,0, "OFF");
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      break;
    case 1:
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(123,0, "HEAT");
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(5, 15, "Power " + String(int(HEATERPOWER)) + "%");
      display.drawString(5, 30, "Mash " + String(mashtemp) + " / " + String(int(MASHSETTEMP)));
      break;
    case 2:
      display.setTextAlignment(TEXT_ALIGN_RIGHT);
      display.drawString(123, 0, "RIMS");
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.drawString(5, 15, "Power " + String(int(HEATERPOWER)) + "%");
      display.drawString(5, 30, "Mash " + String(int(mashtemp)) + " / " + String(int(MASHSETTEMP)));
      display.drawString(5, 45, "RIMS " + String(int(rimstemp)) + " / " + String(int(MAXRIMSTEMP)));
      break;
  }
  display.display();
}

void VextON(void) {
  // I dont know what these do, they were in minimal example
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  // I dont know what these do, they were in minimal example
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void loop() {
  update_display();
  MODE = (MODE+1)%3;
  delay(2000);
}
