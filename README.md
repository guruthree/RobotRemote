# RobotRemote
Arduino Robot Remote Control

Use an ESP8266 as a reciever for a two motor robot, and a program running on a Pi/Laptop/whatever as the transmitter.

## Protocol

Each packet starts with a unique 8 byte float ID, based off of the timestap and run-time. The next packet is the command (CD), a 2 byte short int. The next 8 bytes are any arguments (ARG), for now either the acknowledged packet ID or the PWM value.

```
  Byte: 0           10
        01234567 89 01234567
Packet:       ID CD      ARG
```

**Command list**

* Command 0: HELO (Heartbeat) 
* Command 1: EHLO (Heartbeat response)
* Command 2: Acknowledgement
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
