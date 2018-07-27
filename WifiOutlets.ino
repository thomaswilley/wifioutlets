/* Copyright (c) 2018, Thomas Willey
 * WifiOutlets.ino
 * Simple http server to control wifi outlets (WifiLights project)
 * Built for the NodeMCU/ESP8266 microcontroller
 * https://github.com/thomaswilley/wifioutlets
 * 
 * Please read/re-read the indemnification below!
 * Improper project setup or software use could be very dangerous (e.g., fire).
 * 
 * 
 * Base HTTP Server implementation is based on upon work which is Copyright (c) 2015, Majenko Technologies
     * All rights reserved.
     *
     * Redistribution and use in source and binary forms, with or without modification,
     * are permitted provided that the following conditions are met:
     *
     * * Redistributions of source code must retain the above copyright notice, this
     *   list of conditions and the following disclaimer.
     *
     * * Redistributions in binary form must reproduce the above copyright notice, this
     *   list of conditions and the following disclaimer in the documentation and/or
     *   other materials provided with the distribution.
     *
     * * Neither the name of Majenko Technologies nor the names of its
     *   contributors may be used to endorse or promote products derived from
     *   this software without specific prior written permission.
     *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "wifi_credentials.h" // contains ssid & password both as const char*'s

const float VERSION = 0.1;

const int PORT = 730;
ESP8266WebServer server (PORT);

/* Map Gpio, such that:
 *    target pin is located at OUTLET_TO_PIN_MAP[outlet number][on or off]
 *    example:
 *     - pin corresponding to OFF for outlet #1:  OUTLET_TO_PIN_MAP[1][0]
 *     - and corresponding to ON for same outlet: OUTLET_TO_PIN_MAP[1][1]
*/
const int NUM_VALID_OUTLETS = 4; // would be five but for now we're just using 8 GPIO (i.e., valid_pins)
const int PINS_PER_OUTLET = 2; // one pin each for on & off..
const int OUTLET_TO_PIN_MAP[NUM_VALID_OUTLETS+1][PINS_PER_OUTLET] = {
  { -1, -1 }, /* spacer so we can index on actual outlet number later */
  { 0,  4  },
  { 15, 5  },
  { 12, 13 },
  { 9,  14 },
};

bool isValidOutlet(int outlet) {
  return ((outlet <= NUM_VALID_OUTLETS) and (outlet > 0)) ? true : false; 
}

bool isValidOutletState(int outletState) {
  return ( outletState == HIGH || outletState == LOW ? true : false );
}

void sendPulseToPin(int pin) {
  digitalWrite(pin, LOW);
  digitalWrite(pin, HIGH);
  delay(500);
  digitalWrite(pin, LOW);
}

String getUptime() {
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  String uptime = String(hr) + "h:" + String(min % 60) + "m:" + String(sec % 60) + "s";
  return uptime;
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void returnRedirect(String path) {
  server.sendHeader("Location", path, true);
  server.send ( 302, "text/plain", "");
}

void handleGetValidOutletsJSON() {
  String message = "{ ";
  message += "valid_outlets: [";
  for ( uint8_t outletNum = 1; outletNum < NUM_VALID_OUTLETS+1; outletNum++ ) {
    message = message + String(outletNum) + ",";
  }
  message += "] }";
  server.send(200, "application/json", message);
}

String getOutletToggleForm(int outlet) {
  if(!isValidOutlet(outlet)) return String(outlet + " is not a valid outlet number");
  String message = "<FORM action=\"/updateOutlet\" method=\"post\">";
  message += "<P>Outlet # " + String(outlet);
  message += "<INPUT type=\"radio\" name=\"outletState\" value=\"1\">On";
  message += "<INPUT type=\"radio\" name=\"outletState\" value=\"0\">Off";
  message += "<INPUT type=\"hidden\" name=\"outletNum\" value=\""+ String(outlet) + "\">";
  message += "<INPUT type=\"submit\" value=\"Set\">";
  message += "</P></FORM>";
  return message;
}

void handleUpdateOutlet() {
  // fail if post is malformatted or outlet is not valid
  if (!server.hasArg("outletState") or !server.hasArg("outletNum")) return returnFail("Bad args");
  int outlet = server.arg("outletNum").toInt();
  if (!isValidOutlet(outlet)) return returnFail("Invalid outlet");
  int outletState = server.arg("outletState").toInt();
  if (!isValidOutletState(outletState)) return returnFail("Invalid outlet state");

  sendPulseToPin(OUTLET_TO_PIN_MAP[outlet][outletState]);

  return returnRedirect("/");
}

void handleRoot() {
  String uptime = getUptime();

  String message = "<html> \
  <head> \
    <title>WiFi Lights</title> \
    <style> \
      body { background-color: \#fff; font-family: Arial, Helvetica, Sans-Serif; Color: \#000; } \
    </style> \
  </head> \
  <body> \
    <h1>This is a server for the WiFiLights project</h1>";
  message += uptime;

  for ( uint8_t outletNum = 1; outletNum < NUM_VALID_OUTLETS+1; outletNum++ ) {
    message += getOutletToggleForm(outletNum);
  }
  
  message += "</body></html>";

  server.send ( 200, "text/html", message );
}

void setupGpio() {
  for ( uint8_t outletNum = 1; outletNum < NUM_VALID_OUTLETS+1; outletNum++) {
    for ( uint8_t pinNum = 0; pinNum < PINS_PER_OUTLET; pinNum++ ) {
       int valid_pin = OUTLET_TO_PIN_MAP[outletNum][pinNum];
       pinMode ( valid_pin, OUTPUT );
       digitalWrite ( valid_pin, LOW );
    }
  }
}

void setupWifi() {
  Serial.begin ( 115200 );
  WiFi.mode ( WIFI_STA );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );

  if ( MDNS.begin ( "wifilights" ) ) {
    Serial.println ( "mDNS responder started" );
    Serial.println ( String("Available at http://wifilights.local:"+PORT) );
  } else {
    Serial.println ( "Unable to start mDNS!" );
  }
}

void setupServer() {
  server.on ( "/", handleRoot );
  server.on ( "/uptime", HTTP_GET, []() { server.send ( 200, "text/plain", getUptime() ); } );
  server.on ( "/version", HTTP_GET, []() { server.send ( 200, "text/plain", String(VERSION) ); } );
  server.on ( "/outletsJSON", HTTP_GET, handleGetValidOutletsJSON );
  server.on ( "/updateOutlet", HTTP_POST, handleUpdateOutlet );
  server.onNotFound ( handleNotFound );
 
  server.begin();
  Serial.println ( "HTTP server started" );
}

void setup ( void ) {
  setupGpio();
  setupWifi();
  setupServer();
}

void loop ( void ) {
	server.handleClient();
}
