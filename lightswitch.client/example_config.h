
// ### Important ###
// Rename this file to 'config.h' to work


// Setup Wifi-Connection
// ---------------------
// Add multiple Wifi Networks if you use the lights at different locations.
// E.g. at home / at the makerspace/ ...

#define NETWORKS 3

const char* ssid[NETWORKS] = {
    "network_one",
    "network_two",
    "another_ssid"
    };

const char* password[NETWORKS] = {
    "password_one",
    "password_two",
    "another_password"
    };

    
// Setup Areas (aka LED-Stripes)
// -----------------------------
// A light-element controlled by a microcontroller can have one or more Areas
// customize the Arrays to your needs.
// INFO: possible AREA_MODEs are "rgb" and "rgbw"
#define NAME "My Lightelement Name"
#define AREAS 2
const String AREA_TYPE[AREAS] = {"rgb", "rgb"};
const String AREA_NAME[AREAS] = {"inner Element", "outer Ring"};
Adafruit_NeoPixel AREA[AREAS] = {
    Adafruit_NeoPixel(60, 13, NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(63, 14, NEO_GRB + NEO_KHZ800)};
