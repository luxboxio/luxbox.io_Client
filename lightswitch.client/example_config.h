
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

// Number of Areas
const int AREAS = 2;
// Data-Pins from MC to LED-Stripe
const int AREA_PIN[AREAS] = {13, 14};
// Number of LEDs for each Stripe
const int AREA_COUNT[AREAS] = {60, 63};
// colormode for each Stripe
const String AREA_MODE[AREAS] = {"rgb", "rgb"};
