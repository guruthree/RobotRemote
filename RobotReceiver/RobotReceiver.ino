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
#include "../robot.h"

//#define DEBUG

// Pin constants
#define E_L D8 // "1,2EN" enable driver channels for left motor
#define E_R D1 // "3,4EN" enable driver channels for right motor
#define L_F D7 // "1A" left motor forwards
#define L_R D0 // "2A" left motor reverse
#define R_F D2 // "3A" right motor forwards
#define R_R D3 // "4A" right motor reverse

// Set SSID and password for the robot
const char *ssid = SSID;
const char *password = WIFIPASS;

// Server port
unsigned int localPort = SERVER_PORT;

// UDP support
WiFiUDP Udp;
// Buffer to hold incoming packet
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
// Reply buffer
char replyBuffer[PACKET_LENGTH];

unsigned long nextpacket = 0; // ID of the next outbound packet
unsigned long lastPacketTime = 0; // time at which the last packet was recieved
#define PACKET_LENGTH 12
unsigned long lastEmergencyStop = 0;
int stopped = 1;

// Battery stuff (check frequency should be higher to avoid WiFi/analogRead() issues it seems)
#define BATTERY_CHECK_FREQUENCY 10000
#define BATTERY_CUTOFF_VOLTAGE 7
unsigned long lastBatteryCheck = 0;

// Declare reset function
void(*resetFunc) (void) = 0; // https://www.instructables.com/id/two-ways-to-reset-arduino-in-software/

int LEDstate = HIGH;
void setLED(int instate) {
  LEDstate = instate;
  //digitalWrite(LED_BUILTIN, LEDstate);
  if (LEDstate == HIGH)
    analogWrite(LED_BUILTIN, MYPWMRANGE);
  else
    analogWrite(LED_BUILTIN, MYPWMRANGE-8);
#ifdef DEBUG
  Serial.print("LED: ");
  Serial.println(instate);
#endif
}

// Disable H-bridge and stop motors
void emergencyStop() {
  digitalWrite(E_L, LOW);
  digitalWrite(E_R, LOW);
  digitalWrite(L_F, LOW);
  digitalWrite(L_R, LOW);
  digitalWrite(R_F, LOW);
  digitalWrite(R_R, LOW);

  lastEmergencyStop = millis();
  setLED(HIGH);
  stopped = 1;
#ifdef DEBUG
  Serial.println("Emergency stopped.");
#endif
}

void leftForward(unsigned long velocity) {
  digitalWrite(L_R, LOW);
  analogWrite(L_F, velocity);
#ifdef DEBUG
  Serial.print("left forward: ");
  Serial.println(velocity);
#endif
}

void leftReverse(unsigned long velocity) {
  digitalWrite(L_F, LOW);
  analogWrite(L_R, velocity);
#ifdef DEBUG
  Serial.print("left reverse: ");
  Serial.println(velocity);
#endif
}

void rightForward(unsigned long velocity) {
  digitalWrite(R_R, LOW);
  analogWrite(R_F, velocity);
#ifdef DEBUG
  Serial.print("right forward: ");
  Serial.println(velocity);
#endif
}

void rightReverse(unsigned long velocity) {
  digitalWrite(R_F, LOW);
  analogWrite(R_R, velocity);
#ifdef DEBUG
  Serial.print("right reverse: ");
  Serial.println(velocity);
#endif
}

// Send a packet
void sendPacket(unsigned long command, unsigned long argument) {
  // Make sure reply packet is blank
  memset(replyBuffer, 0, REPLYBUFFER_LENGTH);

  // Make so we can maniuplate as unsigned long
  unsigned long* longReplyBuffer = (unsigned long*)replyBuffer;
  nextpacket += 1;
  // SDLNet_Write32 uses opposite byte order, so we need to swap it back for the ESP
  longReplyBuffer[0] = __builtin_bswap32(nextpacket);
  longReplyBuffer[1] = __builtin_bswap32(command);
  longReplyBuffer[2] = __builtin_bswap32(argument);

  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(replyBuffer, PACKET_LENGTH);
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
      digitalWrite(E_L, HIGH);
      setLED(LOW);
      stopped = 0;
#ifdef DEBUG
      Serial.println("left enable");
#endif
      break;
    case 11: // Left motor disable
      digitalWrite(E_L, LOW);
      break;
    case 15: // Left motor forwards
      leftForward(argument);
      break;
    case 16: // Left motor reverse
      leftReverse(argument);
      break;

    case 20: // Right motor enable
      digitalWrite(E_R, HIGH);
      setLED(LOW);
      stopped = 0;
#ifdef DEBUG
      Serial.println("right enable");
#endif
      break;
    case 21: // Right motor disable
      digitalWrite(E_R, LOW);
      break;
    case 25: // Right motor forwards
      rightForward(argument);
      break;
    case 26: // Right motor reverse
      rightReverse(argument);
      break;

    case 254: // Soft reset
      emergencyStop();
      resetFunc();
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
  // Built-in LED for status
  pinMode(LED_BUILTIN, OUTPUT);

  // make sure nothing's going on boot
  emergencyStop();

  // Setup PWM resolution
  analogWriteRange(MYPWMRANGE);

  // Setup access point
  WiFi.softAP(ssid, password);

  // Listen for incoming packets
  Udp.begin(localPort);

  // LED HIGH is off, showing no connection
  setLED(HIGH);

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("\nRunning...");
  
  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());
#endif
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
    // Flash LED to show recieved
    setLED(!LEDstate);

    // Packet probably is expected, process
    if (packetSize == PACKET_LENGTH) {

      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);

#ifdef DEBUG
      for (int ii = 0; ii < packetSize; ii++) {
        Serial.print((unsigned short int)packetBuffer[ii]);
        Serial.print(" ");
        if ((ii+1) % 4 == 0)
          Serial.print(" ");
      }
      Serial.print("- ");
#endif

      // Make so we can maniuplate as unsigned long
      unsigned long* longPacketBuffer = (unsigned long*)packetBuffer;

      // SDLNet_Write32 uses opposite byte order, so we need to swap it back for the ESP
      unsigned long packetID = __builtin_bswap32(longPacketBuffer[0]);
      unsigned long packetCommand = __builtin_bswap32(longPacketBuffer[1]);

      // Emergency stop as early as possible
      if (packetCommand == 255) {
        emergencyStop();
#ifdef DEBUG
        Serial.println("emergency stop");
#endif
      }
      else {
        unsigned long packetArg = __builtin_bswap32(longPacketBuffer[2]);

#ifdef DEBUG        
        Serial.print(packetID);
        Serial.print(": ");
        Serial.print(packetCommand);
        Serial.print(", ");
        Serial.println(packetArg);
#endif
       
        if (millis() - lastEmergencyStop > EMERGENCY_STOP_TIMEOUT) {
          // Only process if we have not just emergency stopped
          processPacket(packetID, packetCommand, packetArg);
        }
        if (stopped) {
           setLED(!LEDstate);
        }
      }
    }
  }

  // a little bit of sleep
  //delayMicroseconds(10);
  yield();

  // check the battery level
  if (millis() - lastBatteryCheck > BATTERY_CHECK_FREQUENCY) {
    float voltage = analogRead(A0);
    //float voltage = 183;
    voltage = voltage / 1024 * 3.25;
    voltage = voltage / 0.07563025210084;
    
  #ifdef DEBUG
    Serial.print("Battery :");
    Serial.print(voltage);
    Serial.println(" V");
  #endif
  
    if (voltage < BATTERY_CUTOFF_VOLTAGE) {
      emergencyStop();
      digitalWrite(LED_BUILTIN, LOW);
      Udp.stop();
      WiFi.softAPdisconnect(true);
      while (1) {
        delay(1000);
      }
    }
    
    lastBatteryCheck = millis();
  }

}
