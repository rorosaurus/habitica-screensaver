#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include <SPI.h>
#include <TFT_eSPI.h>

#define TYPING_ANIMATION_DELAY 20
#define JSON_DOC_SIZE 10000
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300       /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

const char* requestAddress = "https://habitica.com/api/v3/tasks/user?type=todos";
String response;

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);

  // wakeup stuff
  ++bootCount;
  Serial.print("Boot number: " + String(bootCount) + "; ");
  print_wakeup_reason();

  // init TFT
  tft.init();
  tft.setRotation(1);
  tft.setTextWrap(true, true);
  tft.setTextPadding(2);
  tft.fillScreen(TFT_BLACK);

//  Serial.print("Screen width: ");
//  Serial.println(tft.width());
//  Serial.print("Screen height: ");
//  Serial.println(tft.height());

  writeToTFT("Remember this ToDo task?", true);

  // connect to wifi
  connectToWifi();
  
  // make request
  String taskText = makeAPIRequest();
  Serial.print("Randomly selected task: ");
  Serial.println(taskText);
  
  // disconnect wifi
  WiFi.disconnect();
//  Serial.println("Disconnecting Wifi");
  
  // print everything
  tft.fillScreen(TFT_BLACK);
  delay(500);
  
  writeToTFT(taskText, true);
  delay(1000);
  tft.fillScreen(TFT_BLACK);
  delay(100);
  writeToTFT(taskText, false);
  delay(500);
  tft.fillScreen(TFT_BLACK);
  delay(100);
  writeToTFT(taskText, false);
  delay(500);
  tft.fillScreen(TFT_BLACK);
  delay(100);
  writeToTFT(taskText, false);
  delay(10000);
  
  // go to sleep for a while
  tft.fillScreen(TFT_BLACK);
  tft.writecommand(0x10); // put tft to sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Going to sleep for " + String(TIME_TO_SLEEP) + " seconds");
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {
  
}

void writeToTFT(String taskText, bool animate) {
  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.setCursor(0, 0, 4);

  // Set the font colour to be white with no background
  tft.setTextColor(TFT_WHITE, TFT_WHITE);

  // We can now plot text on screen using the "print" class
  recursiveWordWrapPrint(taskText, animate);
}

void recursiveWordWrapPrint(String text, bool animate) {
  if (text == "") return;

  // does it fit?
  if (tft.textWidth(text) > tft.width()) {
    // how many of these characters can we fit on one line?
    int maxCharThisLine = text.length();
    String firstLine = text;
    while (tft.textWidth(firstLine) > tft.width()) {
      maxCharThisLine--;
      firstLine = text.substring(0, maxCharThisLine - 1);
    }
    
    // find the latest space within first maxCharThisLine letters
    int lastSpace = firstLine.lastIndexOf(" ");
//    Serial.println(lastSpace);
    
    if (lastSpace == -1) { // no space found, default to last char and add a hyphen
      if (!animate) tft.println((text.substring(0, maxCharThisLine - 2) + "-"));
      else {
        for (int i=0; i < maxCharThisLine - 2; i++){
          tft.print(text.charAt(i));
          delay(TYPING_ANIMATION_DELAY);
        }
        tft.print("-\n");
        delay(TYPING_ANIMATION_DELAY);
      }
      recursiveWordWrapPrint(text.substring(maxCharThisLine - 2), animate);
    }
    else { // a space was found, chop the string there
      if (!animate) tft.println(text.substring(0, lastSpace));
      else {
        for (int i=0; i < lastSpace; i++){
          tft.print(text.charAt(i));
          delay(TYPING_ANIMATION_DELAY);
        }
        tft.println();
      }
      recursiveWordWrapPrint(text.substring(lastSpace + 1), animate);
    }
  }
  else {
    if (!animate) tft.println(text);
    else {
      for (int i=0; i < text.length(); i++){
          tft.print(text.charAt(i));
          delay(TYPING_ANIMATION_DELAY);
        }
    }
  }
}

void connectToWifi() {
  WiFi.begin(ssid, password);
//  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
//    Serial.print(".");
  }
//  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

String makeAPIRequest() {
  HTTPClient http;
  String result = "";
  
  // Your IP address with path or Domain name with URL path 
  http.begin(requestAddress);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-client", xClient);
  http.addHeader("x-api-user", userId);
  http.addHeader("x-api-key", apiToken);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
//    Serial.print("HTTP Response code: ");
//    Serial.println(httpResponseCode);

    // The filter: it contains "true" for each value we want to keep
    DynamicJsonDocument filter(JSON_DOC_SIZE);
    filter["data"][0]["text"] = true;
    DynamicJsonDocument jsonResponse(JSON_DOC_SIZE);

//    Serial.print("JSON doc capacity: ");
//    Serial.println(jsonResponse.capacity());

    // Parse JSON object
    DeserializationError error = deserializeJson(jsonResponse, http.getStream(), DeserializationOption::Filter(filter));
    if (error) {
      result = "deserializeJson() failed";
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
    }
    else {
//      Serial.print("Shrinking JSON doc...");
//      jsonResponse.shrinkToFit();

//      Serial.print("JSON doc memory usage: ");
//      Serial.println(jsonResponse.memoryUsage());
      
      // Print the result
//      serializeJsonPretty(jsonResponse, Serial);

      // parse a random task to return
      JsonArray jsonArray = jsonResponse["data"].as<JsonArray>();
  
      int numOfTasks = jsonArray.size();
//      Serial.println(numOfTasks);
      int randomTaskIndex = random(numOfTasks);
//      Serial.println(randomTaskIndex);
    
      JsonVariant task = jsonArray[randomTaskIndex];
      result = task["text"].as<String>();
    }
  }
  else {
    result = "Error code: " + httpResponseCode;
  }
  // Free resources
  http.end();
  return result;
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
