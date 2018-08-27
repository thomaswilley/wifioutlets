/*
 * Copyright (c) 2018, Thomas Willey. All rights reserved.
 * https://github.com/thomaswilley/wifioutlets
 * 
 * Please read/re-read the indemnification below!
 * Improper project setup or software use could be very dangerous (e.g., fire).
 * 
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of the copyright holder or the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission from the copyright holder.
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

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();

const int OUTLETS[] = { 1, 2, 3, 4 }; // SWITCHES[i] is the gpio for WiFiOutlet #i+1
const int NUM_OUTLETS = sizeof(OUTLETS)/sizeof(int);
const char* R433_CODE_MAP[NUM_OUTLETS][2] = { // index as R433_CODE_MAP[outlet#-1][0] for off and R433_CODE_MAP[outlet#-1][1] for on
  { "110111011000000000110100", "110111011000000000111100" },
  { "110111011000000000110010", "110111011000000000111010" },
  { "110111011000000000110001", "110111011000000000111001" },
  { "110111011000000000110101", "110111011000000000111101" },
};


const int PIN_IN = 2;
const int PIN_OUT = 0;

void setup() {
  for(uint8_t i = 0; i < OUTLETS; i++) {
    pinMode(OUTLETS[i], INPUT);
    digitalWrite(OUTLETS[i], HIGH); // pullup
  }

  pinMode(PIN_OUT, OUTPUT);
  digitalWrite(PIN_OUT, LOW);
  mySwitch.enableTransmit(PIN_OUT);
  mySwitch.setPulseLength(163);
}

int last_pin_readings[NUM_OUTLETS];
int current_pin_readings[NUM_OUTLETS];

void blinkPin(int times, int delayms) {
  for(uint8_t i = 0; i<times; i++) {
    digitalWrite(PIN_OUT, HIGH);
    delay(delayms);
    digitalWrite(PIN_OUT, LOW);
    delay(delayms);
  }
}

void loop() {
  delay(50);

  for (uint8_t i = 0; i < NUM_OUTLETS; i++) {
    current_pin_readings[i] = digitalRead(OUTLETS[i]);

    if (last_pin_readings[i] != current_pin_readings[i]) {
      if (current_pin_readings[i] == HIGH) {
        mySwitch.send(R433_CODE_MAP[i][1]);
        delay(50);
      }
      else if (current_pin_readings[i] == LOW) {
        mySwitch.send(R433_CODE_MAP[i][0]);
        delay(50);
      }
    }

    last_pin_readings[i] = current_pin_readings[i];
  }
}
