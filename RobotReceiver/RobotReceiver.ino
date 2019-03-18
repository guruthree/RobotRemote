/*

RobotReceiver

Setup up an ESP8266 as a hotspot, then listen for UDP packets
to direct the attached motors.

Copyright 2019 guruthree

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(D4, OUTPUT); // LED
  pinMode(D8, OUTPUT); // 1,2 EN LEFT
  pinMode(D1, OUTPUT); // 3,4 EN RIGHT

  // LEFT WHEEL
  pinMode(D7, OUTPUT); // 1A (FORWARDS)
  pinMode(D0, OUTPUT); // 2A (BACKWARDS)

  // RIGHT WHEEL
  pinMode(D2, OUTPUT); // 3A (FORWARDS)
  pinMode(D3, OUTPUT); // 4A (BACKWARDS)
  
  digitalWrite(D8, HIGH);
  digitalWrite(D1, HIGH);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(D4, HIGH);
  
  digitalWrite(D7, HIGH);
  digitalWrite(D2 , HIGH);
  
  delay(2500);
  digitalWrite(D4, LOW);
  
  digitalWrite(D7, LOW);
  digitalWrite(D2, LOW);
  delay(2500);
}
