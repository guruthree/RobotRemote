# RobotRemote
Arduino Robot Remote Control

Use an ESP8266 as a reciever for a two motor robot, and a program running on a Pi/Laptop/whatever as the transmitter.

## Protocol

Each packet starts with a sequentuially increasing 4 byte unsigned long ID, based off of the timestap and run-time. The next packet is the command (CD), a 2 byte int. The next 4 bytes are any arguments (ARG), either one 4 byte unsigned long or two 2 byte ints. This will usually be either empty or a PWM value. Multiple commands can be contained in the same packet.

```
  Byte: 0           10 [     ...
        0123 4567 8901 [2345 6789 ...
Packet:   ID   CD  ARG [  CD  ARG ...
```

**Command list**

* Command 0: HELO (Heartbeat) 
* Command 1: EHLO (Heartbeat response)
* Command 10: Left motor enable
* Command 11: Left motor disable
* Command 15: Left motor forwards
* Command 16: Left motor reverse
* Command 20: Right motor enable
* Command 21: Right motor disable
* Command 25: Right motor forwards
* Command 26: Right motor reverse
* Command 254: Reset
* Command 255: Emergency stop
