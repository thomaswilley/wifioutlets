/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "wifi_credentials.h" // contains ssid & password both as const char*'s

char hostString[16] = {0};

const int PIN_STATUS = LED_BUILTIN;

const int SWITCHES[] = { 4, 5, 13, 14 }; // SWITCHES[i] is switch for outlet #i+1

/* WIFIOUTLET_BASE is the base station for the home */
String WIFIOUTLET_BASE_HOST;
IPAddress WIFIOUTLET_BASE_IP; 
int WIFIOUTLET_BASE_PORT;

void setupGpio() {
  for (uint8_t i = 0; i < sizeof(SWITCHES)/sizeof(int); i++) {
    int valid_pin = SWITCHES[i+1];
    pinMode (valid_pin, INPUT);
  }
  pinMode (PIN_STATUS, OUTPUT);
}

void beginStatusLED() {
  for (uint8_t i = 0; i < 3; i++) {
    digitalWrite( PIN_STATUS, LOW );
    delay(250);
    digitalWrite( PIN_STATUS, HIGH );
    delay(250);
  }
}

void endStatusLED() {
  digitalWrite( PIN_STATUS, LOW );
}

void setupWifi() {
  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("Hostname: ");
  Serial.println(hostString);
  
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setupMDNS() {
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
    return;
  }
  
  int n = MDNS.queryService("wifioutlets", "tcp"); // Send out query for esp tcp services
  Serial.println("mDNS query done");
  if (n == 0) {
    Serial.println("no services found");
  }
  else {
    Serial.print(n);
    Serial.println(" service(s) found");
    for (int i = 0; i < n; ++i) {
      // Print details for each service found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(MDNS.hostname(i));
      Serial.print(" (");
      Serial.print(MDNS.IP(i));
      Serial.print(":");
      Serial.print(MDNS.port(i));
      Serial.print(")\r\n");
    }

    WIFIOUTLET_BASE_HOST = String(MDNS.hostname(0));
    WIFIOUTLET_BASE_IP = MDNS.IP(0);
    WIFIOUTLET_BASE_PORT = MDNS.port(0);

    Serial.println("Selecting first entry: " + 
        String(WIFIOUTLET_BASE_HOST + "\t" + WIFIOUTLET_BASE_IP.toString() + ":" + WIFIOUTLET_BASE_PORT));
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  beginStatusLED();

  setupWifi();

  setupMDNS();

  setupGpio();
  
  endStatusLED();
}

String getUrlRequest(int outletNum, int outletState) {
  String data = "outletNum=" + String(outletNum);
  data += "&outletState=" + String(outletState);
  
  // This will send the request to the server
  String request = "POST /updateOutlet HTTP/1.1\r\n" +
               String("Host: " + WIFIOUTLET_BASE_HOST + ":"+ WIFIOUTLET_BASE_PORT + " \r\n") +
               "Accept: */*\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + String(data.length()) + "\r\n\r\n" +
               data;

  Serial.println("Requesting: ");
  Serial.println(request);
  
  return request;
}

bool changeOutletState(int outletNum, int outletState) {
  Serial.println("Changing outlet " + String(outletNum) + " to state " + String(outletState));
  return true;
/*
  Serial.print("connecting to " + String(WIFIOUTLET_BASE_IP.toString()));
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(WIFIOUTLET_BASE_IP, WIFIOUTLET_BASE_PORT)) {
    Serial.println("connection failed");
    return false;
  }

  String request = getUrlRequest(outletNum, outletState);
  
  // This will send the request to the server
  client.print(request);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    if (line == String("HTTP/1.1 302 Found")) {
      Serial.println("success!");
      return true;
    }
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");
  return false;
*/
}

int ledState = LOW;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

int num_retries = 0;
int MAX_RETRIES = 10;

void loop() {
  int reading = digitalRead(SWITCHES[0]);
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      ledState = !ledState;
      digitalWrite(PIN_STATUS, ledState);
      Serial.println("Switch toggled to " + String(ledState));

      int num_retries = 0;
      bool succeeded = false;
      while (!succeeded && (num_retries < MAX_RETRIES)) {
        beginStatusLED();
        succeeded = changeOutletState(1, ledState);
        num_retries++;
      }
      if (!succeeded || num_retries >= MAX_RETRIES) {
        digitalWrite(PIN_STATUS, HIGH);
      } else {
        endStatusLED();
      }
    }
  }
  lastButtonState = reading;
}
