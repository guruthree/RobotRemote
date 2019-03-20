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
#define REPLYBUFFER_LENGTH 48
char replyBuffer[REPLYBUFFER_LENGTH];

// Networking protocol things
#define CONTROLLER_TIMEOUT 3000 // timeout in milliseconds of last packet recieved
unsigned long nextpacket = 0; // ID of the next outbound packet
unsigned long lastPacketTime = 0; // time at which the last packet was recieved
#define MESSAGE_LENGTH 8
#define EMERGENCY_STOP_TIMEOUT 1000 // accept no new packets after an emergency stop for X ms
unsigned long lastEmergencyStop = 0;


// Disable H-bridge and stop motors
void emergencyStop() {
  digitalWrite(E_L, 0);
  digitalWrite(E_R, 0);
  digitalWrite(L_F, 0);
  digitalWrite(L_R, 0);
  digitalWrite(R_F, 0);
  digitalWrite(R_R, 0);
  
  lastEmergencyStop = millis();
}

// Send a packet
void sendPacket(unsigned long command, unsigned long argument) {
  // Make sure reply packet is blank
  memset(replyBuffer, 0, REPLYBUFFER_LENGTH);

  // Make so we can maniuplate as unsigned long
  unsigned long* longReplyBuffer = (unsigned long*)replyBuffer;
  longReplyBuffer[0] = nextpacket++;
  longReplyBuffer[1] = command;
  longReplyBuffer[2] = argument;
  
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(replyBuffer, 12);
  Udp.endPacket();
}

// Process an incoming packet
void processPacket(unsigned long packetID, unsigned long command, unsigned long argument) {
  switch (command) {
    case 0: // HELO
      sendPacket(1, packetID); // Send EHLO packet
      break;
    case 1: // EHLO
      break; // Do nothing

    case 10: // Left motor enable
      break;
    case 11: // Left motor disable
      break;
    case 15: // Left motor forwards
      break;
    case 16: // Left motor reverse
      break;

    case 20: // Right motor enable
      break;
    case 21: // Right motor disable
      break;
    case 25: // Right motor forwards
      break;
    case 26: // Right motor reverse
      break;

    case 254: // Soft reset
      break;
    case 255: // Emergency stop
      // this is caught earlier so this should never be reached
      emergencyStop();
      break;
    
    default:
      break; // Unknown packet!
  }
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

    unsigned long packetID = 0;
    unsigned long packetCommand = 0;
    unsigned long packetArg = 0;
    unsigned int packetAt = 0;
    
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    // Make so we can maniuplate as unsigned long
    unsigned long* longPacketBuffer = (unsigned long*)packetBuffer;
    packetID = longPacketBuffer[packetAt++];
    packetSize -= 4;
    while (packetSize >= MESSAGE_LENGTH) {
      packetCommand = longPacketBuffer[packetAt++];

      // Emergency stop as early as possible
      if (packetCommand == 255) {
        emergencyStop();
        break; // You should never do anything else after an emergency stop!
      }
      
      packetArg = longPacketBuffer[packetAt++];
      packetSize -= MESSAGE_LENGTH;

      if (millis() - lastEmergencyStop > EMERGENCY_STOP_TIMEOUT) {
        // Only process if we have not just emergency stopped
        processPacket(packetID, packetCommand, packetArg);
      }
      else {
        break; // No point to continue processing this packet due to emergency stop timeout
      }
    }
  }
  
  analogWrite(L_F, 127);
  analogWrite(R_F , 127);
  delay(2500);
  
  analogWrite(L_F, 31);
  analogWrite(R_F, 31);
  delay(2500);
}
