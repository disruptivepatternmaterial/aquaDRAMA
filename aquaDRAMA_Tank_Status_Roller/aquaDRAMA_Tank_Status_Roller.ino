//NodeMCU 12-E

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>
#include "OLEDDisplayUi.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SSD1306Wire.h>
#include "WiFi_Info.h"

// Include custom images

#include "images.h"

#define LED D4

// Initialize the OLED display using Wire library
SSD1306Wire  display(0x3c, D1, D2);
// SH1106Wire display(0x3c, D3, D5);

OLEDDisplayUi ui  ( &display );

WiFiClientSecure client;

int val_chiller ; // 0
long val_millis ; // 94026031
float val_pH ; // 8.208
float val_specificgravity ; // 1.024
float val_temp ; // 78.125
String chiller_text;


// User defined variables for Exosite reporting period and averaging samples
#define REPORT_TIMEOUT 300000        //milliseconds period for reporting to Exosite.com
#define SENSOR_READ_TIMEOUT  300000   //milliseconds period for reading sensors in loop

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(64, 0, "aquaDRAMA Condition");
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // draw an xbm image.
  // Please note that everything that should be transitioned
  // needs to be drawn relative to x and y
  display->drawXbm(x + 0, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  String toshow = "Temp:" + String(val_temp, 2) + "F";
  display->drawString(0 + x, 18 + y, toshow);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  String toshow = "pH:" + String(val_pH) + "";
  display->drawString(0 + x, 18 + y, toshow);

}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  String toshow = "SG:" + String(val_specificgravity, 3);
  display->drawString(0 + x, 18 + y, toshow);

}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_24);
  String toshow = "Chiller is " + String(chiller_text);
  display->drawString(0 + x, 18 + y, toshow);
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };

// how many frames are there?
int frameCount = 5;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void setup() {
  Serial.begin(115200);
  Serial.println("Boot");
  display.init();
  display.flipScreenVertically();
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    //bootScreen();
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();

  Serial.println("Setup is Done");
}

int startcount = 1;
void loop() {
  static unsigned long pollDataTime = 0;
  static unsigned long loopDisplayTime = 0;
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {

    if (millis() - pollDataTime > REPORT_TIMEOUT || startcount == 1 ) {
      HTTPClient http;  //Object of class HTTPClient
      //int beginCode = http.begin("http://scooterlabs.com/echo");
      //CF DD 71 A1 BF 01 76 98 A9 21 84 7A 5D 41 38 A0 44 F4 29 3E A5 5C 0B D6 86 4C 76 BA 0B 05 0F 39
      //, "C3 90 0E 8B CB 2D 7A 32 1B 55 5C 00 FA 7B 39 5E 53 BC D2 8F"
      // need to get these signatures from the source site
      //https://api.thinger.io/v1/users/ntableman/buckets/aquaDramaData/data?items=1&max_ts=0&sort=desc&authorization=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJhcXVhRHJhbWFEaXNwbGF5IiwidXNyIjoibnRhYmxlbWFuIn0.DMMboKYppQFO3x8KOXa5PebfnontqyyTc49jsGAIdsI", "C3 90 0E 8B CB 2D 7A 32 1B 55 5C 00 FA 7B 39 5E 53 BC D2 8F"
      http.begin("https://api.thinger.io/v1/users/ntableman/buckets/aquaDramaData/data?items=1&max_ts=0&sort=desc&authorization=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJhcXVhRHJhbWFEaXNwbGF5IiwidXNyIjoibnRhYmxlbWFuIn0.DMMboKYppQFO3x8KOXa5PebfnontqyyTc49jsGAIdsI", "B8 DE 87 81 84 7D F0 83 71 95 6E E6 E2 97 50 54 C6 78 AF A3");
      http.addHeader("Content-Type", "application/json");
      //http.addHeader("Authorization", "Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJhcXVhRHJhbWFEaXNwbGF5IiwidXNyIjoibnRhYmxlbWFuIn0.DMMboKYppQFO3x8KOXa5PebfnontqyyTc49jsGAIdsI", true, true);
      //http.setUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_14) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/12.0 Safari/605.1.15");
      delay(250);
      size_t httpCode = http.GET();
      delay(250);
      Serial.println("---------------------------------");
      Serial.println(httpCode);
      Serial.println("---------------------------------");
      Serial.println("done getting");

      //Check the returning code
      if (httpCode > 0) {
        // Get the request response payload
        String payload = http.getString();
        Serial.println(payload);
        const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(10) + 200;
        DynamicJsonBuffer jsonBuffer(capacity);

        JsonArray& root = jsonBuffer.parseArray(payload);

        long root_0_ts = root[0]["ts"];

        JsonObject& root_0_val = root[0]["val"];
        val_chiller = root_0_val["chiller"];
        val_millis = root_0_val["millis"];
        val_pH = root_0_val["pH"];
        const char* root_0_val_phCalibCode = root_0_val["phCalibCode"];
        const char* root_0_val_phProbeResponse = root_0_val["phProbeResponse"];
        const char* root_0_val_sgCalibCode = root_0_val["sgCalibCode"];
        const char* root_0_val_sgProbeResponse = root_0_val["sgProbeResponse"];
        val_specificgravity = root_0_val["specificgravity"];
        val_temp = root_0_val["temp"];
        int root_0_val_thingerfixedsg = root_0_val["thingerfixedsg"];

        if ( val_chiller == 0) {
          chiller_text = "off";
        } else {
          chiller_text = "on";
        }
        Serial.println(val_temp);
      }
      http.end();   //Close connection
      pollDataTime = millis(); //reset report period timer
      startcount = 2;
    }
    delay(remainingTimeBudget);
  }
}
