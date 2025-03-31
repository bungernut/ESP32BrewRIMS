#include <Wire.h>
#include "HT_SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
#include "AiEsp32RotaryEncoder.h"
#include "max6675.h"     // Adafruit Thermocouple
#include <TM1638lite.h>  // 7Seg/8digit LED, LEDs, Buttons
#include <AutoPID.h>     // https://ryand.io/AutoPID/#autopidautopid-constructor

#define PIN_ENABLE 41
#define PINLED 35      // Onboard White LED
#define PINSSR 6      // Use this one for heater relay control
#define PIN_TOTPOW A0  // The ADC for the TOTal POWer
#define PIN_HTRPOW A1  // ADC for the HeaTeR POWer

#define PIN_SCK 39    //TC
#define PIN_SDO 40    //TC
#define PIN_TC1CS 38  //TC
#define PIN_TC2CS 37  //TC2

#define PIN_LEDDIO GPIO3  // DIO
#define PIN_LEDCLK GPIO4  // CLK
#define PIN_LEDSTB GPIO5  // STB (strobe)


// Setup Constants
const float POWHEATER = 1650.;        // Watts; the power of the heater
const float POWERLIMIT = 12. * 120.;  // Watts - Limit power to 12A

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
**^^** Thinking about not doing this, how about just an enable switch.

PID? https://github.com/r-downing/AutoPID
May consider PID control, but for now lets look at simpler methods.
*/

// Setup Hardware
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst
MAX6675 thermocouple1(PIN_SCK, PIN_TC1CS, PIN_SDO);
MAX6675 thermocouple2(PIN_SCK, PIN_TC2CS, PIN_SDO);
TM1638lite ledDisplay(PIN_LEDSTB, PIN_LEDCLK, PIN_LEDDIO);  //STB,CLK,DIO

bool enable_ops = 0;  // A switch that disables the heater and stuff

// These are parameter that are set by users with whatever UI
double mashsettemp = 150;      // Set point for mash temperature in F
double maxrimstemp = 160;      // Max temp at output of the RIMS heater
double maxpowerpercent = 100;  // it will be actually lower than 100% of maxpower of heater
// Values used to deregulate the power a little so it's not pulling 15A
const double maxheaterset = 1300;  //Watts
const double heaterspec = 1650;    //watts


double mashtemp = 0;
double rimstemp = 0;
double power_tot = 0;
double pid_out = 0;
#define KP 3.14
#define KI .0003
#define KD 0
#define OUTPUT_MIN 0
#define OUTPUT_MAX maxheaterset / heaterspec * 4095.
AutoPID myPID(&mashtemp, &mashsettemp, &pid_out, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

// For displaying on digital LED
String temp_str;
char temp_cstr_array[3];

unsigned long loop_time_curr;
unsigned long loop_time_prev;
const unsigned long update_time = 500;
const unsigned long selection_timeout = 5000;  // How many seconds before encoder times out
unsigned long select_time_last = 0;
int selected_parmeter = 0;  // [ POWER, MASH_TEMP, RIMS_TEMP ]
bool selection_active = 0;



void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);
  // Setup Enable Switch
  pinMode(PIN_ENABLE, INPUT);
  pinMode(PINLED, OUTPUT);
  digitalWrite(PINLED, true);
  delay(1000);
  digitalWrite(PINLED, false);
  delay(1000);
  pinMode(PINSSR, OUTPUT);
  digitalWrite(PINLED, false);
  bool ledstat = ledcAttach(PINLED, 30, 12);  // PIN, freq (Hz), PWM Resolution (8bit=0-255) 12=4096
  bool ssrstat = ledcAttach(PINSSR, 30, 12);  // PIN, freq (Hz), PWM Resolution (8bit=0-255) 12=4096
  Serial.println("led stat:" + String(ledstat));
  Serial.println("led stat:" + String(ssrstat));

  ledDisplay.reset();
  ledDisplay.displayText("SETUP");

  read_temps();
  //update_display();
  Serial.println("Setup Done");
}



void loop() {
  loop_time_curr = millis();
  if (loop_time_curr - loop_time_prev >= update_time) {
    loop_time_prev = loop_time_curr;
    enable_ops = true;  // digitalRead(PIN_ENABLE);
    //Serial.println(String(enable_ops) + " " + String(loop_time_curr));
    if (enable_ops) {
      read_powers();
      read_temps();
      update_power();
    }
    update_display();
  }
  // myPID.reset();
}

void update_power() {
  // if (mashtemp > mashsettemp) {
  //   ledcWrite(PINLED,0);
  // }
  // else if (rimstemp > maxrimstemp) {
  //   ledcWrite(PINLED,0);
  // }
  // else {
  //   ledcWrite(PINLED,1000);
  // }
  myPID.run();
  if (enable_ops == true){
    ledcWrite(PINLED, int(pid_out));
    ledcWrite(PINSSR, int(pid_out));
    //Serial.println(double(pid_out));
  }
  else {
    ledcWrite(PINLED, 0);
    ledcWrite(PINSSR, 0);
  }
}

void update_display() {
  itoa(int(mashtemp), temp_cstr_array, 10);
  if (mashtemp < 100) {
    for (int i = 2; i > 0; i--) {
      temp_cstr_array[i] = temp_cstr_array[i - 1];
    }
    temp_cstr_array[0] = '0';
  }
  for (int i = 0; i < 3; i++) {
    ledDisplay.displayASCII(i, temp_cstr_array[i]);
  }
  //
  for (int i = 3; i < 5; i++) {
    ledDisplay.displayASCII(i, ' ');
  }
  //
  itoa(int(rimstemp), temp_cstr_array, 10);
  if (rimstemp < 100) {
    for (int i = 2; i > 0; i--) {
      temp_cstr_array[i] = temp_cstr_array[i - 1];
    }
    temp_cstr_array[0] = '0';
  }
  for (int i = 5; i < 8; i++) {
    ledDisplay.displayASCII(i, temp_cstr_array[i - 5]);
  }

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(10, 0, "Enabled");
  if (enable_ops) {
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(123, 0, "RIMS");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.drawString(10, 15, "Power " + String(int(maxpowerpercent)) + "%");
    display.drawString(10, 30, "Mash " + String(int(mashtemp)) + " / " + String(int(mashsettemp)));
    display.drawString(10, 45, "RIMS " + String(int(rimstemp)) + " / " + String(int(maxrimstemp)));
    if (selected_parmeter == 0) {
      if (selection_active) {
        display.drawString(0, 15, "@");
      } else {
        display.drawString(0, 15, ">");
      }
    } else if (selected_parmeter == 1) {
      if (selection_active) {
        display.drawString(0, 30, "@");
      } else {
        display.drawString(0, 30, ">");
      }
    } else if (selected_parmeter == 2) {
      if (selection_active) {
        display.drawString(0, 45, "@");
      } else {
        display.drawString(0, 45, ">");
      }
    }
  } else {
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(123, 0, "OFF");
    display.setTextAlignment(TEXT_ALIGN_LEFT);
  }
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(123, 30, String(int(power_tot)) + "W");
  display.display();
}

void read_powers() {
  power_tot = 2400. * (analogRead(PIN_TOTPOW) / 4095.) * 3.3;
}

void read_temps() {
  mashtemp = thermocouple1.readFahrenheit();
  rimstemp = thermocouple2.readFahrenheit();
}

void VextON(void) {
  // I dont know what these do, they were in minimal example
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  // I dont know what these do, they were in minimal example
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}
