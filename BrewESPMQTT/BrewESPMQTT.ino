#include <WiFi.h>
#include <Wire.h>
#include "HT_SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
#include "max6675.h"     // Adafruit Thermocouple
#include <TM1638lite.h>  // 7Seg/8digit LED, LEDs, Buttons
#include <AutoPID.h>     // https://ryand.io/AutoPID/#autopidautopid-constructor
 
const char* ssid = MY_SSID;
const char* password = MY_PASSKEY;

#define PINLED 35      // Onboard White LED
#define PINSSR 6      // Use this one for heater relay control
#define PIN_TOTPOW A0  // The ADC for the TOTal POWer
#define PIN_HTRPOW A1  // ADC for the HeaTeR POWer
#define PIN_SCK 39    //TC1/2
#define PIN_SDO 40    //TC1/2
#define PIN_TC1CS 38  //TC1
#define PIN_TC2CS 37  //TC2

// Setup Hardware
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst
MAX6675 thermocouple1(PIN_SCK, PIN_TC1CS, PIN_SDO);
MAX6675 thermocouple2(PIN_SCK, PIN_TC2CS, PIN_SDO);

// These are parameter that are set by users with whatever UI
double mashsettemp = 150;      // Set point for mash temperature in F
double maxrimstemp = 160;      // Max temp at output of the RIMS heater
double maxpowerpercent = 100;  // it will be actually lower than 100% of maxpower of heater
// Values used to deregulate the power a little so it's not pulling 15A
const double maxheaterset = 1300;  //Watts
const double heaterspec = 1650;    //watts
bool enable_ops = 0;  // A switch that disables the heater and stuff

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


// Loop Parameters
unsigned long loop_time_curr;
unsigned long loop_time_prev;
const unsigned long update_time = 500;
const unsigned long selection_timeout = 5000;  // How many seconds before encoder times out
unsigned long select_time_last = 0;
int selected_parmeter = 0;  // [ POWER, MASH_TEMP, RIMS_TEMP ]
bool selection_active = 0;
int wifi_status = 0;
char cstringToParse[20];
char * parseChars;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
  wifi_status = 1;
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.localIP()[3]);
  wifi_status = int(WiFi.localIP()[3]);
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
  wifi_status = 0;
}

void setup(){
  Serial.begin(115200);
  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_16);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5, 5, "Setup...");
  display.display();

  // delete old config
  WiFi.disconnect(true);
  delay(1000);
  WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);


  /* Remove WiFi event
  Serial.print("WiFi Event ID: ");
  Serial.println(eventID);
  WiFi.removeEvent(eventID);*/

  WiFi.begin(ssid, password);
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.println("Wait for WiFi... ");
}

void loop(){
  delay(1000);
}

void updateDisplay(void){
  display.clear();
  // WiFi Status
  if (wifi_status == true){
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.drawString(123)
  }
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