/*

RobotReceiver

Setup up an ESP8266 as a hotspot, then listen for UDP packets
to direct the attached motors.

Copyright (c) 2019 guruthree

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


// Pin constants
#define E_L D8 // "1,2EN" enable driver channels for left motor
#define E_R D1 // "3,4EN" enable driver channels for right motor
#define L_F D7 // "1A" left motor forwards
#define L_R D0 // "2A" left motor reverse
#define R_F D2 // "3A" right motor forwards
#define R_R D3 // "4A" right motor reverse

// Local PWM range to match what the remote sents
#define MYPWMRANGE 255

// Set SSID and password for the robot
const char *ssid = "RobotzRul";
const char *password = "omglolwut";

// Server port
unsigned int localPort = 7245;

// UDP support
WiFiUDP Udp;
// Buffer to hold incoming packet
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; 
// Reply buffer
#define REPLYBUFFER_LENGTH 32
char replyBuffer[REPLYBUFFER_LENGTH];

// Networking protocol things
#define CONTROLLER_TIMEOUT 3000 // timeout in milliseconds of last packet recieved
unsigned long nextpacket = 0; // ID of the next outbound packet
unsigned long lastPacketTime = 0; // time at which the last packet was recieved


// Disable H-bridge and stop motors
void emergencyStop() {
  digitalWrite(E_L, 0);
  digitalWrite(E_R, 0);
  digitalWrite(L_F, 0);
  digitalWrite(L_R, 0);
  digitalWrite(R_F, 0);
  digitalWrite(R_R, 0);
}


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

  // Enable output
  digitalWrite(E_L, HIGH);
  digitalWrite(E_R, HIGH);

  // Setup PWM resolution
  analogWriteRange(MYPWMRANGE);

  // Setup access point
  WiFi.softAP(ssid, password);

  // Listen for incoming packets
  Udp.begin(localPort);
  // Make sure reply packet is blank
  memset(replyBuffer, 0, REPLYBUFFER_LENGTH);
}


// the loop function runs over and over again forever
void loop() {
  // Activate emergency stop if the controller has lost the connection
  if (millis() - lastPacketTime > CONTROLLER_TIMEOUT) {
    emergencyStop();
  }
  
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    lastPacketTime = millis();
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);


    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyBuffer);
    Udp.endPacket();
  }
  
  analogWrite(L_F, 127);
  analogWrite(R_F , 127);
  delay(2500);
  
  analogWrite(L_F, 31);
  analogWrite(R_F, 31);
  delay(2500);
}
