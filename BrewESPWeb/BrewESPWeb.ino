#include "SECRETS.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <SPI.h>
#include "HT_SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
#include "max6675.h"         // Adafruit Thermocouple
#include <TM1638lite.h>      // 7Seg/8digit LED, LEDs, Buttons
#include <AutoPID.h>         // https://ryand.io/AutoPID/#autopidautopid-constructor

const char* ssid = MY_SSID;
const char* password = MY_PASSKEY;

#define PINLED 35   // Onboard White LED
#define PINSSR 6    // Use this one for heater relay control
#define PIN_POW A1  // The ADC for the POWer

#define PIN_SCK 39    //TC1/2
#define PIN_SDO 40    //TC1/2
#define PIN_TC1CS 38  //TC1
#define PIN_TC2CS 37  //TC2

// Setup Hardware
WebServer server(80);
String header;
//WiFiClient wifi_client;
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED);  // addr , freq , i2c group , resolution , rst
MAX6675 thermocouple1(PIN_SCK, PIN_TC1CS, PIN_SDO);
MAX6675 thermocouple2(PIN_SCK, PIN_TC2CS, PIN_SDO);

// These are parameter that are set by users with whatever UI
double mashsettemp = 150;      // Set point for mash temperature in F
double maxrimstemp = 160;      // Max temp at output of the RIMS heater
double maxpowerpercent = 100;  // it will be actually lower than 100% of maxpower of heater
// Values used to deregulate the power a little so it's not pulling 15A
//const double maxheaterset = 1300.;  //Watts
//const double heaterspec = 1650.;    //watts
bool enable_ops = false;  // A switch that disables the heater and stuff

double mashtemp = 0;
double rimstemp = 0;
double power_tot = 0;
double pid_out = 0;
const double KP = 10.;
const double KI = 0.1;  //.0003
const double KD = 0.1;  //.01;
const int OUTPUT_MIN = 0;
const int OUTPUT_MAX = 3276;  //maxheaterset / heaterspec * 4095.
AutoPID myPID(&rimstemp, &mashsettemp, &pid_out, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);


// Function to handle the root URL and show the current states
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<link rel=\"icon\" href=\"data:,\">";
  html += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; font-size: 20px}";
  html += ".button { background-color: #4CAF50; border: none; color: white; padding: 12px 20px; \
                     border-radius: 8px; text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}";
  html += ".button2 { background-color: #555555; }</style></head>";
  html += "<body><h1>ESPBrew</h1>";
  html += "<p>";
  if (enable_ops == false) {
    html += "<span style=\"color:red;\">DISABLED</span>&nbsp;";
    html += "<a href=\"/enable\"><button class=\"button\">ENABLE</button></a>";
  } else {
    html += "<span style=\"color:green;\">ENABLED</span>&nbsp;";
    html += "<a href=\"/disable\"><button class=\"button\", style=\"background-color:red\">DISABLE</button></a>";
  }
  html += "</p>";
  html += "<br>";
  html += "<h2>MASH Temp</h2>";
  html += "<a href=\"/mash_m5\"><button class=\"button\" style=\"background-color:blue;\">-5</button></a>&nbsp;";
  html += "<a href=\"/mash_m1\"><button class=\"button\" style=\"background-color:blue;\">-1</button></a>&nbsp;";
  html += String(mashsettemp)+"&nbsp;";
  html += "<a href=\"/mash_p1\"><button class=\"button\" style=\"background-color:red;\">+1</button></a>&nbsp;";
  html += "<a href=\"/mash_p5\"><button class=\"button\" style=\"background-color:red;\">+5</button></a>&nbsp;";
  html += "<br>";
  html += "<h2>RIMS Temp</h2>";
  html += "<p style=\"font_size:10;text-align:center\">Set Max temp allowed before shutting off heater</p>";
  html += "<a href=\"/rims_m5\"><button class=\"button\" style=\"background-color:blue;\">-5</button></a>&nbsp;";
  html += "<a href=\"/rims_m1\"><button class=\"button\" style=\"background-color:blue;\">-1</button></a>&nbsp;";
  html += String(maxrimstemp)+"&nbsp;";
  html += "<a href=\"/rims_p1\"><button class=\"button\" style=\"background-color:red;\">+1</button></a>&nbsp;";
  html += "<a href=\"/rims_p5\"><button class=\"button\" style=\"background-color:red;\">+5</button></a>&nbsp;";
  html += "<br>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}
void handleEnable() {
  enable_ops = true;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleDisable() {
  enable_ops = false;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleMashP5() {
  mashsettemp = mashsettemp+5.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleMashP1() {
  mashsettemp = mashsettemp+1.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleMashM5() {
  mashsettemp = mashsettemp-5.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleMashM1() {
  mashsettemp = mashsettemp-1.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
// RIMS Temps
void handleRIMSP5() {
  maxrimstemp = maxrimstemp+5.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleRIMSP1() {
  maxrimstemp = maxrimstemp+1.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleRIMSM5() {
  maxrimstemp = maxrimstemp-5.0;
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}
void handleRIMSM1() {
  maxrimstemp = maxrimstemp-1.0;
  //handleRoot();
  server.sendHeader("Location", "/",true);
  server.send(302, "text/plain", "");
}


// Loop Parameters
unsigned long loop_time_curr;
unsigned long loop_time_prev;
unsigned long web_time_prev = 0;
const long web_timeout_time = 2000;
unsigned long mqtt_loop_time_prev;
const unsigned long update_time = 1000;
const unsigned long mqtt_update_time = 5000;
int wifi_status = 0;
// char cstringToParse[20];
// char * parseChars;

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Connected to AP successfully!");
  wifi_status = 1;
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.localIP()[3]);
  wifi_status = int(WiFi.localIP()[3]);
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid, password);
  wifi_status = 0;
}

void setup() {
  Serial.begin(115200);
  VextON();
  delay(100);
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


  server.on("/", handleRoot);
  server.on("/enable", handleEnable);
  server.on("/disable", handleDisable);
  server.on("/mash_p5", handleMashP5);
  server.on("/mash_p1", handleMashP1);
  server.on("/mash_m1", handleMashM1);
  server.on("/mash_m5", handleMashM5);
  server.on("/rims_p5", handleRIMSP5);
  server.on("/rims_p1", handleRIMSP1);
  server.on("/rims_m5", handleRIMSM5);
  server.on("/rims_m1", handleRIMSM1);

  WiFi.begin(ssid, password);
  delay(1000);
  read_temps();
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Setup Done");
}

void loop() {
  loop_time_curr = millis();
  if (loop_time_curr - loop_time_prev > update_time) {
    read_temps();
    read_power();
    update_power();
    update_display();
    loop_time_prev = loop_time_curr;
  }
  // Handle incoming client requests
  server.handleClient();
}

void update_display(void) {
  display.clear();
  display.setFont(ArialMT_Plain_10);
  // WiFi Status
  IPAddress ip = WiFi.localIP();
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 0, "WiFi:" + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]));
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  //display.drawString(0,0,  String(int(power_tot)) + " / " + String(int(pid_out*100/4096)));
  display.drawString(0, 15, "Mash " + String(int(mashtemp)) + "/" + String(int(mashsettemp)));
  display.drawString(0, 30, "RIMS " + String(int(rimstemp)) + "/" + String(int(maxrimstemp)));
  if (enable_ops == true) {
    display.drawString(10, 45, "ENABLED");
  } else {
    display.drawString(10, 45, "DISABLED");
  }
  display.display();
}



void update_power() {
  if (rimstemp > maxrimstemp or enable_ops == false) {
    ledcWrite(PINLED, 0);
    ledcWrite(PINSSR, 0);
  } else {
    myPID.run();
    ledcWrite(PINLED, int(pid_out));
    ledcWrite(PINSSR, int(pid_out));
  }
}

void read_power() {
  power_tot = 20. * 120. * (float(analogRead(PIN_POW)) / 4095.) * 3.33;
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