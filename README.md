# RobotRemote
Arduino Robot Remote Control

Use an ESP8266 as a remote receiver for a two motor robot. Use a program running on a Pi/Laptop/whatever with a Xbox (or any XInput) controller as the remote transmitter.

The ESP forms a network hotspot that the laptop connects to. The ESP then listens on a UDP port for packets sent from the laptop control program, which tell the ESP how the attached motors should move. The ESP should be connected to something like a dual H-bridge chip, with four output pins dedicated to left motor forward, left motor back, right motor forwards, right motor back.

This software probably works with an ESP32 as well.

## The UDP packet structure and protocol

Each packet consists of three four unsigned long 4 byte integers. The first long is a sequentially increasing packet ID. The second long is the command (CMD). The last long is the command argument (ARG). This will usually be either empty or a PWM value.

```
  Byte: 0           10
        0123 4567 8901
Packet:   ID  CMD  ARG
```

### Command list

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
* Command 254: Soft reset
* Command 255: Emergency stop

## Compile & run RobotReceiver
RobotReceiver is the control software running on the ESP8266.

Open the RobotReceiver sketch with the Arduino IDE. Make sure the [ESP8266 Core](https://github.com/esp8266/Arduino) is installed and configured for your chip. Edit the sketch to define which pins are connected to the motors, and to configure the WiFi ESSID and password. Then compile and send as usual.

## Compile & run RobotController
RobotController is the transmitter software running on the PC.

Plug in a controller, like an Xbox 360 or Xbox One controller. Make sure the system detects it, either via `jstest` or `Set up USB game controllers`.

### Linux

Install the development libraries for SDL2 and SDL2_net, e.g. `sudo apt-get build-essential install libsdl2-dev libsdl2-net-dev`. Then in the `RobotController` folder, type `make` and `make run` to compile and run. To run in debug mode, type `make debug` and `make debugrun`.

### Windows

As always, compiling under Windows is a little bit trickier. You can either setup the development environment of your choice with SDL2 and SDL2_net and import robotcontroller.c, or follow these directions to setup a minimal compilation environment without any installation.

1. Download and extract `codeblocks-17.12mingw-nosetup.zip` from http://www.codeblocks.org/downloads/26 to a new folder, e.g. `codeblocks-17.12mingw-nosetup`.
2. Download `SDL2-devel-2.0.9-mingw.tar.gz` and `SDL2_net-devel-2.0.1-mingw.tar.gz` from https://www.libsdl.org/download-2.0.php and https://www.libsdl.org/projects/SDL_net/. (Or alternatively download the latest versions.)
3. From the SDL*.tar.gz files, extract the contents of the `i686-w64-mingw32\` folders to the codeblocks folder `MinGW\` folder, e.g. `codeblocks-17.12mingw-nosetup\MinGW\`.
4. Edit `compile.bat` to so that line 2, staring with `set PATH=` points to the `codeblocks-17.12mingw-nosetup\MinGW\bin` folder.
5. Copy `SDL2.dll` and `SDL2_net.dll` from the `bin\` folder to the `RobotController\` folder.
6. Run `compile.bat` by double-clicking. A console dialog with any compile-time output should appear.
7. Run `robotcontroller.exe`.

## Controls

* Left analog stick up/down: Left motor
* Right analog stick up/down: Right motor
* RB: Unlock motors
* Any other button: Lock motors (i.e. emergency stop)

## LED Status

* Off: Waiting for connection
* On: Low battery
* Short flash: Locked, connected to remote
* Slow blink: Unlocked, waiting for commands
* Rapid flashing: Receiving commands
