#include <Wire.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "config.h"

//
// lightswitch.space Client
// version: 0.5alpha
//
// This Sketch is used to control a bunch of SK2812 LEDs (aka Neopixel)
// The Strips can be controlled from the lightswitch.space-Server.
//

#define _DEBUG false
#define _DEBUG_FADE false

unsigned long FadeTimer[AREAS] = {0}; 
unsigned long FadeTimerPart[AREAS] = {0};
unsigned long CurrentFadeTime[AREAS] = {0};
unsigned long previousFade[AREAS] = {0};
bool fade[AREAS] = {false};

unsigned long CircleTimer[AREAS] = {0}; 
unsigned long CircleTimerPart[AREAS] = {0};
unsigned long CurrentCircleTime[AREAS] = {0};
unsigned long previousCircle[AREAS] = {0};

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

// Setup Color Mode
int color_mode[AREAS] = {0};


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

        // Initialize Fadetimer / Circletimer
        FadeTimer[i]   = 10000000; // total fadetime in milliseconds for a colorchange
        CircleTimer[i] = 8000000; // total fadetime in milliseconds for a colorchange

        FadeTimerPart[i] = FadeTimer[i] / 255;
        CircleTimerPart[i] = CircleTimer[i] / 255;
    }

    // start serial
    Serial.begin(115200);
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
    /* --------------------------
     * check if WLAN is connected
     * -------------------------- */
    if (WiFi.status() != WL_CONNECTED)
    {
        wifiConnected = connectWiFi();
        
        if (wifiConnected) {
            udpConnected = connectUDP();
        }
    }
    
    /* -------------------------------
     * Broadcast & UPDRecieve Handling
     * ------------------------------- */
    if (wifiConnected) 
    {
        broadcastMyself();
        if (udpConnected) 
        {
            if(RecieveUpdPackage())
            {
                for(int i = 0; i < AREAS; i++)
                {
                    if(_DEBUG)
                    {
                        Serial.println("### Fadetime Reset ###");
                        Serial.print("ColorMode: ");
                        Serial.println(color_mode[i]);
                    }
                    
                    CurrentFadeTime[i] = 0;
                    fade[i] = true;

                }
            }
        }
    }
    
    /* -------------------------------------------
     * Color-Loop, dependent on mode for each Area
     * ------------------------------------------- */
    
    for(int i = 0; i < AREAS; i++)
    {
        if(_DEBUG)
        {
            Serial.print("### For Each Area ### Current: ");
            Serial.println(i);
        }
        
        unsigned long currentMicros = micros();
        double TimerPercentage = -1;
        double CirclePercentage = -1;
    
        // calculate Fade Timer
        if (currentMicros - previousFade[i] >= FadeTimerPart[i])
        {
            previousFade[i] = currentMicros;
            CurrentFadeTime[i] += FadeTimerPart[i];
            TimerPercentage = (double)CurrentFadeTime[i] / (double)FadeTimer[i] * 100.00;
        }
    
        // calculate Circle Timer
        if (currentMicros - previousCircle[i] >= CircleTimerPart[i])
        {
            previousCircle[i] = currentMicros;
            CurrentCircleTime[i] += CircleTimerPart[i];
            CirclePercentage = (double)CurrentCircleTime[i] / (double)CircleTimer[i] * 100.00;
        }
                
        if(CirclePercentage > 100)
        {
            CirclePercentage = 0;
            CurrentCircleTime[i] = 0;
        }

        if(_DEBUG)
        {
            Serial.print("Current Micros:         ");
            Serial.println(currentMicros);
            Serial.print("Color Mode:             ");
            Serial.println(color_mode[i]); 
            Serial.print("   -> TimerPercentage:  ");
            Serial.println(TimerPercentage);
            Serial.print("   -> CirclePercentage: ");
            Serial.println(CirclePercentage);
        }
        
        // Color-Mode 0: solid
        if(color_mode[i] == 0 && TimerPercentage > -1)
        {    
            if(_DEBUG)
            {  
                Serial.print("AREA: ");
                Serial.print(i);
                Serial.print(" | TimerPercentage: ");
                Serial.println(TimerPercentage);
            }
            
            if(fade[i] == true)
            {
                if(_DEBUG)
                {
                    Serial.println("### 1 ###");
                }
                
                from_red[i] = current_red[i];
                from_green[i] = current_green[i];
                from_blue[i] = current_blue[i];
                from_white[i] = current_white[i];

                fade[i] == false;
            }

            if(current_red[i] != target_red[i]
                || current_green[i] != target_green[i]
                || current_blue[i] != target_blue[i]
                || current_white[i] != target_white[i])
            {
                if(_DEBUG)
                {
                    Serial.println("### 2 ###");
                    Serial.print("From:    ");
                    Serial.println(from_red[i]);
                    Serial.print("Current: ");
                    Serial.println(current_red[i]);
                    Serial.print("Target:  ");
                    Serial.println(target_red[i]);
                }
                
                FadeLightTwo(i, TimerPercentage);
            }

            if(CurrentFadeTime[i] > FadeTimer[i])
            {
                if(_DEBUG)
                {
                    Serial.println("### 3 ###");
                }
                
                CurrentFadeTime[i] = 0;
    
                current_red[i] = target_red[i];
                current_green[i] = target_green[i];
                current_blue[i] = target_blue[i];
                current_white[i] = target_white[i];
            }
        }
        // Color-Mode 1: rainbow
        if(color_mode[i] == 1 && CirclePercentage > -1)
        {
            if(_DEBUG)
            {
                Serial.println("### RainbowFade ###");
            }
            RainbowFade(i, CirclePercentage);
        }
        // Color-Mode 2: rainbow cycle
        if(color_mode[i] == 2 && CirclePercentage > -1)
        { 
            if(_DEBUG)
            {
                Serial.println("### RainbowCycleFade ###");
            }
            RainbowCycleFade(i, CirclePercentage);
        }
    }
}


//////////////////////
// Helper functions //
//////////////////////


bool RecieveUpdPackage()
{
    bool ok = false;
    
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
        for( int i = 0; i < UDP_TX_PACKET_MAX_SIZE;  ++i )
        {
            packetBuffer[i] = (char)0;
        }

        // read the packet into packetBufffer
        UDP.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        Serial.println("Contents: ");                
        Serial.println(packetBuffer);

        ok = UpdateDataFromJson(packetBuffer);
        
        // send a reply, to the IP address and port that sent us the packet we received
        UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
        UDP.write(ReplyBuffer);
        UDP.endPacket();
        delay(10);
    }   

     return ok;
}

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

                color_mode[i] = root["areas"][i]["color_mode"];

                // Color-Mode 0: solid
                if(color_mode[i] == 0)
                {                
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
                // Color-Mode 1: rainbow
                if(color_mode[i] == 1)
                { 
                   // ToDo ?! 
                }
                // Color-Mode 2: rainbow cycle
                if(color_mode[i] == 1)
                { 
                    // ToDo ?!
                }
            }
        }
    }
    
    return state;
}

// Rainbow Effekt Loop
void RainbowFade(int a, double percentage) {
    uint16_t j;

    double value_rainbow = (double)255 * percentage / (double)100;    
    j = (int) floor(value_rainbow);

    if(j > 255)
        j = 255;
    if(j < 0)
        j = 0;

    if(_DEBUG_FADE)
    {
        //Serial.print("ValueRainbow:   ");
        //Serial.println(value_rainbow);
        Serial.print("           j:   ");
        Serial.println(j);
        Serial.print("   Wheel (0):   ");
        Serial.println(Wheel((j) & 255));
    }
    
    for (int i = 0; i < AREA[a].numPixels(); i++) 
    {
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        AREA[a].setPixelColor(i, Wheel(j & 255));                    
    }
    
    AREA[a].show();
}

// Rainbow-Cycle Effekt Loop
void RainbowCycleFade(int a, double percentage) {
    uint16_t j;

    double value_rainbow = (double)255 * percentage / (double)100;    
    j = (int) floor(value_rainbow);

    if(j > 255)
        j = 255;
    if(j < 0)
        j = 0;

    if(_DEBUG_FADE)
    {
        //Serial.print("ValueRainbow:   ");
        //Serial.println(value_rainbow);
        Serial.print("           j:   ");
        Serial.println(j);
        Serial.print("   Wheel (0):   ");
        Serial.println(Wheel((j) & 255));
    }
    
    for (int i = 0; i < AREA[a].numPixels(); i++) 
    {
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        AREA[a].setPixelColor(i, Wheel(((i * 256 / AREA[a].numPixels()) +j) & 255));                
    }
    
    AREA[a].show();
}

/*void rainbowCycle() {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
  }
}
*/

// New time and color constant Fader version
void FadeLightTwo(int a, double percentage)
{
    percentage = percentage / 100;
    
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
    
    for (int i = 0; i < AREA[a].numPixels(); i++) {
        // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
        AREA[a].setPixelColor(i, current_red[a], current_green[a], current_blue[a], current_white[a]);                    
    }
    AREA[a].show();
}

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
        StaticJsonBuffer<2000> jsonBuffer;
        
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


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return (byte)(255 - WheelPos * 3)<<16 | (byte)(0)<<8 | (byte)(WheelPos * 3)<<0 | (byte)(0)<<24;
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return (byte)(0)<<16 | (byte)(WheelPos * 3)<<8 | (byte)(255 - WheelPos * 3)<<0 | (byte)(0)<<24;
  }
  WheelPos -= 170;
  return (byte)(WheelPos * 3)<<16 | (byte)(255 - WheelPos * 3)<<8 | (byte)(0)<<0 | (byte)(0)<<24;
}
