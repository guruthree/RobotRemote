# RobotRemote
Arduino Robot Remote Control

Use an ESP8266 as a receiver for a two motor robot, and a program running on a Pi/Laptop/whatever as the transmitter.

## Protocol

Each packet starts with a sequentially increasing 4 byte unsigned long ID. The next packet is the command (CMD), a 2 byte int. The next 4 bytes are any arguments (ARG), either one 4 byte unsigned long or two 2 byte ints. This will usually be either empty or a PWM value. Multiple commands can be contained in the same packet.

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

## Compile & Run RobotController

Plug in a controller, like an Xbox 360 or Xbox One controller. Make sure the system detects it, either via `jstest` or `Set up USB game controllers`.

### Linux

Install the development libraries for SDL2 and SDL2_net, e.g. `sudo apt-get build-essential install libsdl2-dev libsdl2-net-dev`. Then in the `RobotController` folder, type `make` and `make run` to compile and run. To run in debug mode, type `make debug` and `make debugrun`.

### Windows

As always, compiling under Windows is a little bit trickier. You can either setup the development environment of your choice with SDL2 and SDL2_net and import robotcontroller.c, or follow these directions to setup a minimal compilation environment without any installation.

1. Download and extract `codeblocks-17.12mingw-nosetup.zip` from [http://www.codeblocks.org/downloads/26] to a new folder, e.g. `codeblocks-17.12mingw-nosetup`.
2. Download `SDL2-devel-2.0.9-mingw.tar.gz` and `SDL2_net-devel-2.0.1-mingw.tar.gz` from [https://www.libsdl.org/download-2.0.php] and [https://www.libsdl.org/projects/SDL_net/]. (Or alternatively download the latest versions.)
3. From the SDL*.tar.gz files, extract the contents of the `i686-w64-mingw32\` folders to the codeblocks folder `MinGW\` folder, e.g. `codeblocks-17.12mingw-nosetup\MinGW\`.
4. Edit `compile.bat` to so that line 2, staring with `set PATH=` points to the  `codeblocks-17.12mingw-nosetupMinGW\bin` folder.
5. Copy `SDL2.dll` and `SDL2_net.dll` from that same folder to the `RobotContoller\` folder.
6. Run `compile.bat` by double-clicking. A console dialog with any compile-time output should appear.
7. Run `robotconroller.exe`.


