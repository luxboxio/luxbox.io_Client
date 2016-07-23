#include <Wire.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

//
// lightswitch.space Client
// version: 0.3alpha
//
// This Sketch is used to control a bunch of SK2812 LEDs (aka Neopixel)
// The Strips can be controlled from the lightswitch.space-Server.
//
// Some globals
const int fadedelay = 5; // in millisecond
const int fadestep = 1;

// A light-element has a unique ID, so it can be identfied within the network.
// The MAC-address is taken as unique ID, so no need to change this parameter
String ID = "";

// A light-element controlled by a microcontroller can have one or more Areas
const int AREAS = 2;

// customize the Arrays to your needs.
// INFO: possible AREA_MODEs are "rgb" and "rgbw"
const int AREA_PIN[AREAS] = {14, 13};
const int AREA_COUNT[AREAS] = {5, 3};
const String AREA_MODE[AREAS] = {"rgb", "rgb"};

Adafruit_NeoPixel AREA[AREAS];

// Setup Pixel colors
int current_red[AREAS] = {0};
int current_green[AREAS] = {0};
int current_blue[AREAS] = {0};
int current_white[AREAS] = {0};

int target_red[AREAS] = {0};
int target_green[AREAS] = {0};
int target_blue[AREAS] = {0};
int target_white[AREAS] = {0};

// Setup Wifi-Connection
const char* ssid = "vspace.one";
const char* password = "12345678";
//const char* ssid = "Evergreen Terrace 742";
//const char* password = "8840556831567070";
boolean wifiConnected = false;

// UDP variables
unsigned int localPort = 1234;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged"; // a string to send back

void setup()
{
    // Initialize all Areas (aka Neopixel stripes)
    for(int i = 0; i < AREAS; i++)
    {
        AREA[i] = Adafruit_NeoPixel(AREA_COUNT[i], AREA_PIN[i], NEO_GRB + NEO_KHZ800);
        AREA[i].begin();

        for (int j = 0; j < AREA_COUNT[i]; j++)
        {
            AREA[i].setPixelColor(j, 0); // all pixels off
            AREA[i].show();
        }
    }
    
    // start serial
    Serial.begin(9600);
    Serial.println("los gehts...");

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

    for(int i = 0; i < AREAS; i++)
    {
        if(current_red[i] != target_red[i]
            || current_green[i] != target_green[i]
            || current_blue[i] != target_blue[i]
            || current_white[i] != target_white[i])
        {
            FadeLight(i);
        }
        
    }
    
    delay(fadedelay);

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

        if(area >= 0 && area < AREAS)
        {
            String type = root["type"];
            
            target_red[area] = root["values"]["red"];
            target_green[area] = root["values"]["green"];
            target_blue[area] = root["values"]["blue"];
            
            if(type == "rgbw" && AREA_MODE[area] == type)
            {
                target_white[area] = root["values"]["white"];
            }
        }
    }
    
    return state;
}

void FadeLight(int area)
{
    // RED
    if(current_red[area] != target_red[area])
    {
        if(current_red[area] - target_red[area] > 0)
        {
            current_red[area] -= fadestep;
        }
        else
        {
            current_red[area] += fadestep;
        }

        if(current_red[area] > 255)
            current_red[area] = 255;
        if(current_red[area] < 0)
            current_red[area] = 0;
    }

    // GREEN
    if(current_green[area] != target_green[area])
    {
        if(current_green[area] - target_green[area] > 0)
        {
            current_green[area] -= fadestep;
        }
        else
        {
            current_green[area] += fadestep;
        }

        if(current_green[area] > 255)
            current_green[area] = 255;
        if(current_green[area] < 0)
            current_green[area] = 0;
    }

    // BLUE
    if(current_blue[area] != target_blue[area])
    {
        if(current_blue[area] - target_blue[area] > 0)
        {
            current_blue[area] -= fadestep;
        }
        else
        {
            current_blue[area] += fadestep;
        }

        if(current_blue[area] > 255)
            current_blue[area] = 255;
        if(current_blue[area] < 0)
            current_blue[area] = 0;
    }

    // WHITE
    if(current_white[area] != target_white[area])
    {
        if(current_white[area] - target_white[area] > 0)
        {
            current_white[area] -= fadestep;
        }
        else
        {
            current_white[area] += fadestep;
        }

        if(current_white[area] > 255)
            current_white[area] = 255;
        if(current_white[area] < 0)
            current_white[area] = 0;
    }

    Serial.print("Fade ");
    Serial.print(current_red[area]);
    Serial.print("; ");
    Serial.print(current_green[area]);
    Serial.print("; ");
    Serial.println(current_blue[area]);
    
    for (int i = 0; i < AREA_COUNT[area]; i++) {
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        AREA[area].setPixelColor(i, current_red[area], current_green[area], current_blue[area]); 
        AREA[area].show();
    }
    
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
