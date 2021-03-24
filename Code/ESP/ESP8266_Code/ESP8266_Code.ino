/*
 * This program was written by Felix Hillebrand 2021
 * This program is published under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International license
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FastLED.h>

#define MDNS_NAME "onairsign"

#define NUM_LED   8
#define DATA_PIN 16
#define LED_PIN   2

#define STARTUP_DELAY 2000l
#define WIFI_TIMEOUT 10000l

long pre_millis = 0;
long cur_millis = 0;
long intv = 1000;

short led = 0;
short server_override = 0;
int r, g, b = 0;
int luminosity = 128;

CRGB leds[NUM_LED];
CRGB serverColor = CRGB::Red;

const char* ssid = "<Your ssid here>";
const char* password = "<Your password here>";

ESP8266WebServer server(80);

void setup() {
    delay(STARTUP_DELAY);

    Serial.begin(115200);
    Serial.println("\n============"); 
    Serial.println("= Starting =");
    Serial.println("============");

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LED);
    FastLED.setBrightness(luminosity);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 1000);
    
    initWiFi();
    initmDNS();
    initWebServer();

    pinMode(LED_PIN, OUTPUT);
    Serial.println("\nStarting loop!");
    setColor(CRGB::AliceBlue);
    FastLED.show();
    delay(STARTUP_DELAY);
}

void initWiFi() {
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    long start = millis();
    long current = 0;
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(". ");
        delay(500);
        current = millis();
        if (led == 0) {
            led = 1;
            if (current - start >= WIFI_TIMEOUT) {
                setColor(CRGB::FireBrick);
            }
            else {
                setColor(CRGB::Amethyst);
            }
        }
        else {
            led = 0;
            setColor(CRGB::Black);
        }
    }
    Serial.println("");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void initmDNS() {
    if (!MDNS.begin(MDNS_NAME)) {
        Serial.println("Error setting up MDNS responder!");
        while (1) {
            delay(1000);
            if (led == 0) {
                led = 1;
                setColor(CRGB::Red);
            }
            else {
                led = 0; 
                setColor(CRGB::Black);
            }
            FastLED.show();
        }
    }
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS started");
}

void initWebServer() {
    Serial.println("Starting webserver");

    // Setup routing
    server.on("/", HTTP_GET, []() {
        server.send(200, F("text/html"),
            F("Welcome to the REST Web Server"));
    });
    server.on("/color", HTTP_POST, handleColor); 
    server.on("/color", HTTP_GET, handleColor);

    // Setup NotFound
    server.onNotFound(handleNotFound);

    // Start server
    server.begin();
}

void handleNotFound() {
    String msg = "FILE NOT FOUND\n\nURI: ";
    msg += server.uri();
    msg += "\nMethod: ";
    msg += (server.method() == HTTP_GET) ? "GET" : "POST";
    msg += "\nArguments: ";
    msg += server.args();
    msg += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        msg += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    
    msg += "Headers: ";
    msg += server.headers();
    msg += "\n";
    for (uint8_t i = 0; i < server.headers(); i++) {
        msg += " " + server.headerName(i) + ": " + server.header(i) + "\n";
    }
   
    server.send(404, "text/plain", msg);
}

void handleColor() {
    for (uint8_t i = 0; i < server.args(); i++) {
        if (server.argName(i).equalsIgnoreCase("r")) {
            r = server.arg(i).toInt();
        }
        else if(server.argName(i).equalsIgnoreCase("g")) {
            g = server.arg(i).toInt();
        }
        else if (server.argName(i).equalsIgnoreCase("b")) {
            b = server.arg(i).toInt();
        }
        else if (server.argName(i).equalsIgnoreCase("lum")) {
            luminosity = server.arg(i).toInt();
        }
        else if (server.argName(i).equalsIgnoreCase("s")) {
            server_override = server.arg(i).toInt();
        }
    }

    server.send(200, "none", "");
}

void loop() {
    MDNS.update();
    server.handleClient();

    if (server_override == 0) {
        rainbow_wave(25, 2);
    }
    else if(server_override == 1){
        setColor(CRGB(r, g, b));
        FastLED.setBrightness(luminosity);
    } else {
        setColor(CRGB::Black);
    }

    cur_millis = millis();
    if (cur_millis - pre_millis >= intv) {
        Serial.println("Alive");
        if (led == 1) {
            digitalWrite(LED_PIN, HIGH);
            led = 0;
        }
        else {
            digitalWrite(LED_PIN, LOW);
            led = 1;
        }
        pre_millis = cur_millis;
    }

    FastLED.show();
}

void rainbow_wave(uint8_t speed, uint8_t delta) {
    uint8_t hue = beat8(speed, 255);
    fill_rainbow(leds, NUM_LED, hue, delta);
}

void setColor(CRGB color) {
    for (int i = 0; i < NUM_LED; i++) {
        leds[i] = color;
    }
}
