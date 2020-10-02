#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"
#include <SPI.h>
#include <TFT_eSPI.h>

#define JSON_DOC_SIZE 10000
#define MINUTES_INBETWEEN_CHECKS 1

const char* requestAddress = "https://habitica.com/api/v3/tasks/user";
String response;

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  
  tft.init();
  tft.setRotation(1);
  tft.setTextWrap(true, true);
  tft.setTextPadding(2);
  tft.fillScreen(TFT_BLACK);

  Serial.print("Screen width: ");
  Serial.println(tft.width());
  Serial.print("Screen height: ");
  Serial.println(tft.height());
}

void loop() {
  // connect to wifi
  connectToWifi();
  
  // make request
  String taskText = makeAPIRequest();
  Serial.print("Randomly selected task: ");
  Serial.println(taskText);
  
  // print everything
  writeToTFT(taskText);
  delay(2000);

  // disconnect wifi
  WiFi.disconnect();
  Serial.println("Disconnecting Wifi");
  
  // wait for a while
  tft.writecommand(0x10); // put tft to sleep
  Serial.print(MINUTES_INBETWEEN_CHECKS);
  Serial.println(" minutes, then checking again...");
  delay(1000 * 60 * MINUTES_INBETWEEN_CHECKS); // wait MINUTES_INBETWEEN_CHECKS minutes before trying again
}

void writeToTFT(String taskText) {
  tft.init();
  tft.setRotation(1);
  tft.setTextWrap(true, true);
  tft.setTextPadding(2);
  tft.fillScreen(TFT_BLACK);
  
  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.setCursor(0, 0, 4);

  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // We can now plot text on screen using the "print" class
  recursiveWordWrapPrint(taskText);
}

void recursiveWordWrapPrint(String text) {
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
      tft.println((text.substring(0, maxCharThisLine - 2) + "-"));
      recursiveWordWrapPrint(text.substring(maxCharThisLine - 2));
    }
    else { // a space was found, chop the string there
      tft.println(text.substring(0, lastSpace));
      recursiveWordWrapPrint(text.substring(lastSpace + 1));
    }
  }
  else {
    tft.println(text);
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
