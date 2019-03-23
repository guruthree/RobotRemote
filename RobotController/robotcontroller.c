/*

RobotController

Listen for input on a JoyStick, then send UDP packets to RobotReceiver.

Copyright (c) 2019 guruthree

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#define JOYSTICK_MAX 32768
#define DEADZONE (JOYSTICK_MAX/10)

#define MYPWMRANGE 255

#define REMOTE_HOST "192.168.4.1"
#define REMOTE_PORT 7245
#define HEARTBEAT_TIMEOUT 500

SDL_Joystick *joystick;
UDPsocket socket;
UDPpacket *packet;
Uint32 nextpacket = 0;
unsigned long lastPacketTime = 0;

void cleanup() {
    printf("Exiting...\n");
    if (packet)
        SDLNet_FreePacket(packet);
    packet = NULL;
    if (socket)
        SDLNet_UDP_Close(socket);
    socket = NULL;
    if (joystick)
        SDL_JoystickClose(joystick);
    joystick = NULL;
    SDLNet_Quit();
    SDL_Quit();
    exit(0);
}

int main(){ //int argc, char **argv) {
    int i;

    // Handle internal quits nicely
    atexit(cleanup);

	printf("Initialising...\n");

    if (SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK ) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    if(SDLNet_Init() < 0) {
        fprintf(stderr, "Couldn't initialise SDLNet: %s\n", SDLNet_GetError());
        exit(1);
    }

    i = SDL_NumJoysticks();
    printf("%i joysticks were found.\n", i);
    if (i == 0) {
        exit(3);
    }

    printf("The names of the joysticks are:\n");
	
    for (i = 0; i < SDL_NumJoysticks(); i++) {
        printf("    %s\n", SDL_JoystickName(i));
    }

    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(0);
    if (joystick == NULL) {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(1);
    }

    // Networking...
    IPaddress remoteAddr;
    SDLNet_ResolveHost(&remoteAddr, REMOTE_HOST, REMOTE_PORT);
    socket = SDLNet_UDP_Open(0);
    if (!socket) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        exit(4);
    }
    packet = SDLNet_AllocPacket(48);
    if (!packet) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        exit(5);
    }

    SDL_Event event;

    // Main loop
    int running = 1;
    while (running) {

        if (SDL_GetTicks() - lastPacketTime > HEARTBEAT_TIMEOUT) {
            lastPacketTime = SDL_GetTicks();

            packet->address.host = remoteAddr.host;
            packet->address.port = remoteAddr.port;

            nextpacket++;
            SDLNet_Write32(nextpacket, packet->data);
            SDLNet_Write32(0, packet->data+4); // HELO
            SDLNet_Write32(0, packet->data+8); // no arguments to HELO*/

            packet->len = 12;
            SDLNet_UDP_Send(socket, -1, packet);
            printf("Sending heartbeat (%d)!\n", nextpacket);
        }

        // recieve all waiting packets
        while (SDLNet_UDP_Recv(socket, packet) == 1) {
            printf("Packet recieved...\n");
        }

        while(SDL_PollEvent(&event) != 0)
        {  
            switch(event.type)
            {  
                case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
                    if ((event.jaxis.value < -DEADZONE ) || (event.jaxis.value > DEADZONE)) {
                        printf("axis: %i value: %i\n", event.jaxis.axis, event.jaxis.value);
                    }
                    break;

                case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Presses */
                    printf("button %i pressed\n", event.jbutton.button);
                break;

                case SDL_JOYHATMOTION:  /* Handle Hat Motion */
                    printf("dpad %i pressed\n", event.jhat.value);
                break;

                case SDL_QUIT:
                    running = 0;
                    cleanup();
                    break;
            }
        }
   }

	return 0;
}
