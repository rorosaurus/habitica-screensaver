#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const char* requestAddress = "https://habitica.com/api/v3/tasks/user";
String response;

#define JSON_DOC_SIZE 10000
DynamicJsonDocument jsonResponse(JSON_DOC_SIZE);

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
      
  httpGETRequest(requestAddress);

//  serializeJsonPretty(jsonResponse, Serial);
  JsonArray jsonArray = jsonResponse["data"].as<JsonArray>();
  
  int numOfTasks = jsonArray.size();
  Serial.println(numOfTasks);
  
  int randomTaskIndex = random(numOfTasks);
  Serial.println(randomTaskIndex);

  JsonVariant task = jsonArray[randomTaskIndex];
  Serial.println(task["text"].as<String>());
}

void loop() {

}

void httpGETRequest(const char* requestAddress) {
  HTTPClient http;
  
  // Your IP address with path or Domain name with URL path 
  http.begin(requestAddress);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-client", xClient);
  http.addHeader("x-api-user", userId);
  http.addHeader("x-api-key", apiToken);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // The filter: it contains "true" for each value we want to keep
    DynamicJsonDocument filter(JSON_DOC_SIZE);
    filter["data"][0]["text"] = true;

    Serial.print("JSON doc capacity: ");
    Serial.println(jsonResponse.capacity());

    // Parse JSON object
    DeserializationError error = deserializeJson(jsonResponse, http.getStream(), DeserializationOption::Filter(filter));
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    else {
//      Serial.print("Shrinking JSON doc...");
//      jsonResponse.shrinkToFit();

      Serial.print("JSON doc memory usage: ");
      Serial.println(jsonResponse.memoryUsage());
      
      // Print the result
//      serializeJsonPretty(jsonResponse, Serial);
    }
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
}
