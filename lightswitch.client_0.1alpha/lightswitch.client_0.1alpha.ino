// Libs
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

// 
// lightswitch.space Client
// version: 0.1alpha
// 
// This Sketch is used to control a bunch of SK2812 LEDs (aka Neopixel)
// The Strips can be controlled from the lightswitch.space-Server.
// 
// A light-element has a unique ID, so it can be identfied within the network.
// The MAC-address is taken as unique ID, so no need to change this parameter
String ID = "";

// A light-element controlled by a microcontroller can have one or more Areas
const int AREAS = 2;              // ToDo: Change to your needs!

// Area 1
const int AREA_1_PIN = 14;        // ToDo: Change to your needs!
const int AREA_1_COUNT = 5;       // ToDo: Change to your needs!
const char* AREA_1_MODE = "RGB"; // ToDo: Change to your needs!
Adafruit_NeoPixel AREA_1 = Adafruit_NeoPixel(AREA_1_COUNT, AREA_1_PIN, NEO_GRB + NEO_KHZ800);

// Area 2
const int AREA_2_PIN = 13;        // ToDo: Change to your needs!
const int AREA_2_COUNT = 3;       // ToDo: Change to your needs!
const char* AREA_2_MODE = "RGB"; // ToDo: Change to your needs!
Adafruit_NeoPixel AREA_2 = Adafruit_NeoPixel(AREA_2_COUNT, AREA_2_PIN, NEO_GRB + NEO_KHZ800);

// Area n
// add more Areas when needed;

// Setup Wifi-Connection
const char* ssid = "vspace.one";
const char* password = "12345678";
byte mac[6]; 

void setup()
{  
  // start serial
  Serial.begin(9600);
  Serial.println("los gehts...");
  
  // Initialize LED-stripes
  AREA_1.begin();
  AREA_2.begin();

  // connect to WiFi
  WiFi.mode(WIFI_STA);
  connectWiFi();

  WiFi.macAddress(mac);
  ID += macToStr(mac);

  Serial.println(" --- lightswitch.space client ---");
  Serial.print(" IP-Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("         ID: ");
  Serial.println(ID);
  Serial.print("      Areas: ");
  Serial.println(AREAS);
  
}

void loop() 
{
  // check if WLAN is connected
  if (WiFi.status() != WL_CONNECTED)
  {
    connectWiFi();
  }

  // ToDo: Implement light control

}


//////////////////////
// Helper functions //
//////////////////////

void connectWiFi()
{
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  } 
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
  }
  return result;
}
