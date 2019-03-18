/*

RobotReceiver

Setup up an ESP8266 as a hotspot, then listen for UDP packets
to direct the attached motors.

Copyright 2019 guruthree

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

// Pin constants
#define E_L D8 // "1,2EN" enable driver channels for left motor
#define E_R D1 // "3,4EN" enable driver channels for right motor
#define L_F D7 // "1A" left motor forwards
#define L_R D0 // "2A" left motor reverse
#define R_F D2 // "3A" right motor forwards
#define R_R D3 // "4A" right motor reverse

// the setup function runs once when you press reset or power the board
void setup() {
  // Setup output pin directions
  // Enable pins
  pinMode(E_L, OUTPUT); // 1,2 EN LEFT
  pinMode(E_R, OUTPUT); // 3,4 EN RIGHT
  // Left wheel pins
  pinMode(L_F, OUTPUT); // 1A (FORWARDS)
  pinMode(L_R, OUTPUT); // 2A (BACKWARDS)
  // Right wheel pin
  pinMode(R_F, OUTPUT); // 3A (FORWARDS)
  pinMode(R_R, OUTPUT); // 4A (BACKWARDS)
  
  digitalWrite(D8, HIGH);
  digitalWrite(D1, HIGH);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(L_F, HIGH);
  digitalWrite(R_F , HIGH);
  delay(2500);
  
  digitalWrite(L_F, LOW);
  digitalWrite(R_F, LOW);
  delay(2500);
}
