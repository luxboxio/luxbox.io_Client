#include <Wire.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "config.h"

//
// lightswitch.space Client
// version: 0.4alpha
//
// This Sketch is used to control a bunch of SK2812 LEDs (aka Neopixel)
// The Strips can be controlled from the lightswitch.space-Server.
//
// Some globals
const int fadedelay = 5; // in millisecond
const int fadestep = 1;

const unsigned long FadeTimer = 2000000; // total fadetime in milliseconds for a colorchange
unsigned long FadeTimerPart = 0;
unsigned long CurrentFadeTime = 0;
unsigned long previousFade = 1;
// A light-element has a unique ID, so it can be identfied within the network.
// The MAC-address is taken as unique ID, so no need to change this parameter
String ID = "";

// Setup Pixel colors
int from_red[AREAS] = {0};
int from_green[AREAS] = {0};
int from_blue[AREAS] = {0};
int from_white[AREAS] = {0};

int current_red[AREAS] = {0};
int current_green[AREAS] = {0};
int current_blue[AREAS] = {0};
int current_white[AREAS] = {0};

int target_red[AREAS] = {0};
int target_green[AREAS] = {0};
int target_blue[AREAS] = {0};
int target_white[AREAS] = {0};

boolean wifiConnected = false;
int currentWifi = 0;

// UDP variables
unsigned int sendPort = 11110;
unsigned int recievePort = 11111;
WiFiUDP UDP;
boolean udpConnected = false;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged"; // a string to send back

const long BroadcastTimer = 5000;  
unsigned long previousBroadcast = 0;  // will store last time Broadcast was sent

 
void setup()
{    
    // Initialize all Areas (aka Neopixel stripes)
    for(int i = 0; i < AREAS; i++)
    {
        AREA[i].begin();   

        for (int j = 0; j < AREA[i].numPixels(); j++)
        {
            AREA[i].setPixelColor(j, 0); // all pixels off
            AREA[i].show();
        }
    }

    // Initialize Fadetimer
    FadeTimerPart = FadeTimer / 255;
    
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
    ID = WiFi.macAddress(); // ToDo: Remove the colons from MAC address
}

void loop()
{
    // check if WLAN is connected
    if (WiFi.status() != WL_CONNECTED)
    {
        wifiConnected = connectWiFi();
        
        if (wifiConnected) {
            udpConnected = connectUDP();
        }
    }

    // check if the WiFi and UDP connections were successful
    if (wifiConnected) {

        broadcastMyself();
            
        if (udpConnected) {

            // if there’s data available, read a packet
            int packetSize = UDP.parsePacket();
            if (packetSize)
            {
                // ToDo: deaktivate! Serial.println because of performance.
                
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

                // Clear Buffer before reading
                for( int i = 0; i < UDP_TX_PACKET_MAX_SIZE;  ++i ){
                    packetBuffer[i] = (char)0;
                }

                // read the packet into packetBufffer
                UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
                Serial.println("Contents: ");                
                Serial.println(packetBuffer);

                bool ok = UpdateDataFromJson(packetBuffer);

                if(ok)
                {
                    // Reset Fadetime and start fade again
                    CurrentFadeTime = 0;
                }
                
                // send a reply, to the IP address and port that sent us the packet we received
                UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
                UDP.write(ReplyBuffer);
                UDP.endPacket();
                delay(10);
            }
            
        }
    }

    for(int i = 0; i < AREAS; i++)
    {
        // New color Info set
        // fade starts from current colors :-)
        if(CurrentFadeTime == 0)
        {
            from_red[i] = current_red[i];
            from_green[i] = current_green[i];
            from_blue[i] = current_blue[i];
            from_white[i] = current_white[i];
        }
                
        if(current_red[i] != target_red[i]
            || current_green[i] != target_green[i]
            || current_blue[i] != target_blue[i]
            || current_white[i] != target_white[i])
        {
            //FadeLight(i); //old pragmatic way
            FadeLightTwo(i);
        }

        if(CurrentFadeTime > FadeTimer)
        {
            CurrentFadeTime = 0;

            current_red[i] = target_red[i];
            current_green[i] = target_green[i];
            current_blue[i] = target_blue[i];
            current_white[i] = target_white[i];
        }
    }
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
    StaticJsonBuffer<3000> jsonBuffer;

    Serial.println("StaticJsonBuffer defined");
    
    // Step 2: Deserialize the JSON string
    JsonObject& root = jsonBuffer.parseObject(json);

    Serial.println("Deserialized object");
    
    if (!root.success())
    {
      Serial.println("parseObject() failed");
      return false;
    }

    if(root["light_id"] == ID)
    {     
        Serial.println("-> ID correct");

        if(root["areas"].size() > 0)
        {
            // set light for every area
            for(int i = 0; i < root["areas"].size() || i < AREAS; i++)
            {
                // get color values for the current area
                int AreaNumber = root["areas"][i]["number"];
                
                //root["areas"][i]["values"].prettyPrintTo(Serial);

                for(int j = 0; j < root["areas"][i]["values"].size(); j++)
                {
                    String color_name = root["areas"][i]["values"][j]["color"];
                    int color_value = root["areas"][i]["values"][j]["value"];

                    Serial.print("Color: ");
                    Serial.print(color_name);
                    Serial.print(" -> ");
                    Serial.println(color_value);

                    // ToDo: Rework to Switch!?
                    
                    if(color_name == "r")
                    { 
                        target_red[i] = color_value;
                    }
                    if(color_name == "g")
                    {
                        target_green[i] = color_value;
                    }
                    if(color_name == "b")
                    {
                        target_blue[i] = color_value;
                    }
                    if(color_name == "w")
                    {
                        target_white[i] = color_value;
                    }
                } 
            }
        }
    }
    
    return state;
}

// New time and color constant Fader version
void FadeLightTwo(int a)
{
    unsigned long currentMicros = micros();

    if (currentMicros - previousFade >= FadeTimerPart)
    {
        previousFade = currentMicros;
        CurrentFadeTime += FadeTimerPart;
        double percentage = (double)CurrentFadeTime / (double)FadeTimer;

        double value_red = (double)(target_red[a] - from_red[a]) * percentage + (double)from_red[a];
        double value_green = (double)(target_green[a] - from_green[a]) * percentage + (double)from_green[a];
        double value_blue = (double)(target_blue[a] - from_blue[a]) * percentage + (double)from_blue[a];
        double value_white = (double)(target_white[a] - from_white[a]) * percentage + (double)from_white[a];

        int check_red = (int) floor(value_red);
        if(check_red > 255)
            check_red = 255;
        if(check_red < 0)
            check_red = 0;

        int check_green = (int) floor(value_green);
        if(check_green > 255)
            check_green = 255;
        if(check_green < 0)
            check_green = 0;
            
        int check_blue = (int) floor(value_blue);
        if(check_blue > 255)
            check_blue = 255;
        if(check_blue < 0)
            check_blue = 0;

        int check_white = (int) floor(value_white);
        if(check_white > 255)
            check_white = 255;
        if(check_white < 0)
            check_white = 0;
            
        current_red[a]      = check_red;
        current_green[a]    = check_green;
        current_blue[a]     = check_blue;
        current_white[a]    = check_white;

        /*Serial.print(CurrentFadeTime);
        Serial.print(" | ");
        Serial.print(percentage, 2);
        Serial.print(" | ");
        Serial.println(current_red[a]);*/
    }
    
    for (int i = 0; i < AREA[a].numPixels(); i++) {
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        AREA[a].setPixelColor(i, current_red[a], current_green[a], current_blue[a], current_white[a]);                    
    }
    AREA[a].show();
}

/*void FadeLight(int area)
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

    //Serial.print("Fade ");
    //Serial.print(current_red[area]);
    //Serial.print("; ");
    //Serial.print(current_green[area]);
    //Serial.print("; ");
    //Serial.print(current_blue[area]);
    //Serial.print("; ");
    //Serial.println(current_white[area]);
    
    for (int i = 0; i < AREA[area].numPixels(); i++) {
        
        if(AREA_TYPE[area] == "rgb")
        {
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            AREA[area].setPixelColor(i, current_red[area], current_green[area], current_blue[area]);   
        }
        if(AREA_TYPE[area] == "rgbw")
        {
            // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
            AREA[area].setPixelColor(i, current_red[area], current_green[area], current_blue[area], current_white[area]); 
        }
               
    }
    AREA[area].show();

    //Serial.print("#");
    
}
*/

// connect to wifi – returns true if successful or false if not
boolean connectWiFi() {
    boolean state = true;
    int i = 0;
    WiFi.begin(ssid[currentWifi], password[currentWifi]);
    Serial.println("");
    Serial.print("Connecting to WiFi '");
    Serial.print(ssid[currentWifi]);
    Serial.println("'");

    // Wait for connection
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        if (i > 15) {
            
            currentWifi++;
            if(currentWifi >= NETWORKS)
            {
                currentWifi = 0;
            }
            
            state = false;
            break;
        }
        i++;
    }
    if (state) {
        Serial.println("");
        Serial.println(" --- lightswitch.space client ---");
        Serial.print("Connected to ");
        Serial.println(ssid[currentWifi]);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("         ID: ");
        Serial.println(WiFi.macAddress());
        Serial.print("      Areas: ");
        Serial.println(AREAS);
        Serial.println("");
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

    if (UDP.begin(recievePort) == 1) {
        Serial.println("Connection successful");
        state = true;
    }
    else {
        Serial.println("Connection failed");
    }

    return state;
}

// Broadcast own configuration to lightswitch Server
void broadcastMyself()
{
    unsigned long currentMicros = millis();
    
    if (currentMicros - previousBroadcast >= BroadcastTimer) {
        // save the last time you blinked the LED
        previousBroadcast = currentMicros;

        //Serial.println("Try broadcasting...");

        // prepare Data
        //
        // Step 1: Reserve memory space
        //
        StaticJsonBuffer<1000> jsonBuffer;
        
        //
        // Step 2: Build object tree in memory
        //
        JsonObject& root = jsonBuffer.createObject();
        root["light_id"] = ID;
        root["name"] = NAME;
        
        JsonArray& areas = root.createNestedArray("areas");

        for(int i = 0; i < AREAS; i++)
        {
            JsonObject& area = areas.createNestedObject();
            area["number"] = i;
            area["name"] = AREA_NAME[i];
            area["color_type"] = AREA_TYPE[i];

            JsonArray& values = area.createNestedArray("values");

            for(int j = 0; j < AREA_TYPE[i].length(); j++){
                JsonObject& colorvalue = values.createNestedObject();
                String tmp = String(AREA_TYPE[i].charAt(j));
                colorvalue["color"] = tmp;
            }
        }
        
        //
        // Step 3: Generate the JSON string
        //
        char charBuf[500];
        root.printTo(charBuf, 500);        

        //Serial.println("Try broadcasting...");
        IPAddress broadcastIp(255, 255, 255, 255);
        UDP.beginPacket(broadcastIp,sendPort);
        UDP.write(charBuf);
        UDP.endPacket();
        //Serial.println("Broadcasting sent");
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
