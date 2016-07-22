#include <Wire.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

//
// lightswitch.space Client
// version: 0.2alpha
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
//const char* ssid = "vspace.one";
//const char* password = "12345678";
const char* ssid = "Evergreen Terrace 742";
const char* password = "8840556831567070";
boolean wifiConnected = false;

// UDP variables
unsigned int localPort = 1234;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged"; // a string to send back

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
    wifiConnected = connectWiFi();
    if (wifiConnected) {
        udpConnected = connectUDP();
    }

    byte mac[6];
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

    // check if the WiFi and UDP connections were successful
    if (wifiConnected) {
        if (udpConnected) {
            // if there’s data available, read a packet
            int packetSize = UDP.parsePacket();
            if (packetSize)
            {
                Serial.println("");
                Serial.print("Received packet of size ");
                Serial.println(packetSize);
                Serial.print("From ");
                IPAddress remote = UDP.remoteIP();
                for (int i = 0; i < 4; i++)
                {
                    Serial.print(remote[i], DEC);
                    if (i < 3)
                    {
                        Serial.print(".");
                    }
                }
                Serial.print(", port ");
                Serial.println(UDP.remotePort());

                // read the packet into packetBufffer
                UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
                Serial.println("Contents: ");                
                Serial.println(packetBuffer);

                bool ok = UpdateDataFromJson(packetBuffer);
                
                // send a reply, to the IP address and port that sent us the packet we received
                UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
                UDP.write(ReplyBuffer);
                UDP.endPacket();

            }
            delay(10);
        }
    }

    // ToDo: Implement light control

}


//////////////////////
// Helper functions //
//////////////////////

// Deserialize JSON-String
bool UpdateDataFromJson(char* json)
{
    Serial.println("UpdateDataFromJson() running");
    
    bool state = true;
    // Step 1: Reserve memory space
    StaticJsonBuffer<500> jsonBuffer;

    Serial.println("StaticJsonBuffer defined");
    
    // Step 2: Deserialize the JSON string
    JsonObject& root = jsonBuffer.parseObject(json);

    Serial.println("Deserialized object");
    
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return false;
    }
    
    if(root["id"] == ID)
    {       
        int area = root["area"];
        String type = root["type"];
        
        if(type == "rgb")
        {
            int red = root["values"]["red"];
            int green = root["values"]["green"];
            int blue = root["values"]["blue"];
            
            Serial.print(" Area: ");
            Serial.println(area);
            Serial.print(" Type: ");
            Serial.println(type);
            Serial.print("  Red: ");
            Serial.println(red);
            Serial.print("Green: ");
            Serial.println(green);
            Serial.print(" Blue: ");
            Serial.println(blue);
        }
    }
    
    return state;
}

// connect to wifi – returns true if successful or false if not
boolean connectWiFi() {
    boolean state = true;
    int i = 0;
    WiFi.begin(ssid, password);
    Serial.println("");
    Serial.println("Connecting to WiFi");

    // Wait for connection
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (i > 10) {
            state = false;
            break;
        }
        i++;
    }
    if (state) {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.println("");
        Serial.println("Connection failed.");
    }
    return state;
}

// connect to UDP – returns true if successful or false if not
boolean connectUDP() {
    boolean state = false;

    Serial.println("");
    Serial.println("Connecting to UDP");

    if (UDP.begin(localPort) == 1) {
        Serial.println("Connection successful");
        state = true;
    }
    else {
        Serial.println("Connection failed");
    }

    return state;
}

String macToStr(const uint8_t* mac)
{
    String result;
    for (int i = 0; i < 6; ++i) {
        result += String(mac[i], 16);
    }
    return result;
}
