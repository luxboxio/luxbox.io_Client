
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


// Setup LightserverIP
// -------------------
// Same procedure as above

const char* lightserverIP[NETWORKS] = {
    "192.168.0.112",
    "xxx.xxx.xxx.xxx",
    "xxx.xxx.xxx.xxx"
    };

