#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "secrets.h"

const char* requestAddress = "https://habitica.com/api/v3/tasks/user";

String response;

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
      
  response = httpGETRequest(requestAddress);
  Serial.println(response);
}

void loop() {

}

String httpGETRequest(const char* requestAddress) {
  HTTPClient http;
  
  // Your IP address with path or Domain name with URL path 
  http.begin(requestAddress);

  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-client", xClient);
  http.addHeader("x-api-user", userId);
  http.addHeader("x-api-key", apiToken);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

     // get lenght of document (is -1 when Server sends no Content-Length header)
      int len = http.getSize();

      // create buffer for read
      uint8_t buff[128] = { 0 };

      // get tcp stream
      WiFiClient * stream = http.getStreamPtr();

      // read all data from server
      while(http.connected() && (len > 0 || len == -1)) {
          // get available data size
          size_t size = stream->available();

          if(size) {
              // read up to 128 byte
              int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

              // write it to Serial
              Serial.write(buff, c);

              if(len > 0) {
                  len -= c;
              }
          }
          delay(1);
      }

      Serial.println();
      Serial.print("[HTTP] connection closed or file end.\n");
//    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
