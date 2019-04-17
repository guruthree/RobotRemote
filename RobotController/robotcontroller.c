/*

RobotController

Listen for input on a JoyStick, then send UDP packets to RobotReceiver.

Copyright (c) 2019 guruthree

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include <stdlib.h>
#ifdef __WIN32__
    #define SDL_MAIN_HANDLED
    #include <windows.h>
    #define STRING_BUFFER_LENGTH 32
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#ifdef __linux__
    #include <glib.h>
#endif
#include <unistd.h>

#include "../robot.h"
#include "robotcontroller.h"
#include "controllerfunctions.h"


SDL_Joystick *joystick;
UDPsocket udpsocket;
IPaddress remoteAddr;
UDPpacket *packet;
Uint32 nextpacket = 0;
unsigned long lastPacketTime = 0;


int speed = 1; // 1 - fast, 2 - slow
int invert = 1; // 1 or -1

void cleanup() {
    printf("Exiting...\n");
    if (packet && udpsocket)
        sendPacket(255, 0); // make sure the motors are stopped before we go
    if (packet)
        SDLNet_FreePacket(packet);
    packet = NULL;
    if (udpsocket)
        SDLNet_UDP_Close(udpsocket);
    udpsocket = NULL;
    if (joystick)
        SDL_JoystickClose(joystick);
    joystick = NULL;
    SDLNet_Quit();
    SDL_Quit();
    exit(0);
}

void sendPacket(Uint32 command, Uint32 argument) {
    packet->address.host = remoteAddr.host;
    packet->address.port = remoteAddr.port;

    nextpacket++;
    SDLNet_Write32(nextpacket, packet->data);
    SDLNet_Write32(command, packet->data+4);
    SDLNet_Write32(argument, packet->data+8);

    packet->len = 12;
    SDLNet_UDP_Send(udpsocket, -1, packet);
    lastPacketTime = SDL_GetTicks();
//    printf("sending packet %i, %i, %i\n", nextpacket, command, argument);
}


void executeButton(struct buttonDefinition *button) {
    switch (button->type) {
        case ENABLE:
            sendPacket(10, 0); // enable left motor
            sendPacket(20, 0); // enable right motor
            printf("Enabling motors...\n");
            break;

        case DISABLE:
            sendPacket(11, 0); // disable left motor
            sendPacket(21, 0); // disable right motor
            printf("Disabling motors\n");
            break;

        case STOP:
            printf("Interrupting running macros\n");
            break;

        case EXIT:
            printf("Exit?\n");
            SDL_Event sdlevent;
            sdlevent.type = SDL_QUIT;
            SDL_PushEvent(&sdlevent);
            break;

        case FAST:
            speed = 1;
            printf("Speeding up\n");
            break;

        case SLOW:
            speed = 2;
            printf("Slowing down\n");
            break;

        case INVERT1:
            invert = 1;
            printf("Invert 1\n");
            break;

        case INVERT2:
            invert = -1;
            printf("Invert 2\n");
            break;

        case MACRO:
            break;

        case NONE:
            sendPacket(255, 0); // any other button, stop!
            printf("Assuming emergency stop!\n");
            break;
    }
}

int main(){ //int argc, char **argv) {
    int i;

    // Handle internal quits nicely
    atexit(cleanup);

    printTime();
	printf("Initialising...\n");

    if (SDL_Init( SDL_INIT_JOYSTICK ) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    if(SDLNet_Init() < 0) {
        fprintf(stderr, "Couldn't initialise SDLNet: %s\n", SDLNet_GetError());
        exit(1);
    }

    i = SDL_NumJoysticks();
    printTime();
    printf("%i joysticks were found.\n", i);
    if (i == 0) {
        exit(3);
    }

    printTime();
    printf("The names of the joysticks are:\n");
	
    for (i = 0; i < SDL_NumJoysticks(); i++) {
        printTime();
        printf("    (%i) %s\n", i, SDL_JoystickNameForIndex(i));
    }

    // load which joystick from file
    int joystickID = 0;
#ifdef  __linux__
    GKeyFile* gkf = g_key_file_new();
    GError *gerror = NULL;
    if (!g_key_file_load_from_file(gkf, CONFIG_FILE, G_KEY_FILE_NONE, &gerror)) {
        fprintf(stderr, "Could not read config file %s, %s\n", CONFIG_FILE, gerror->message);
        g_error_free(gerror);
        gerror = NULL;
        exit(2);
    }

    joystickID = getIntFromConfig(gkf, "controller", "id", 0);
#elif __WIN32__
    joystickID = GetPrivateProfileInt("controller", "id", 0, CONFIG_FILE);
#else
    fprintf(stderr, "Need a way to read a .ini file!\n");
#endif

    printTime();
    printf("Using joystick %i\n", joystickID);
    joystick = SDL_JoystickOpen(joystickID);
    if (joystick == NULL) {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_JoystickEventState(SDL_ENABLE);

    // Networking...
    SDLNet_ResolveHost(&remoteAddr, REMOTE_HOST, SERVER_PORT);
    udpsocket = SDLNet_UDP_Open(0);
    if (!udpsocket) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        exit(4);
    }
    packet = SDLNet_AllocPacket(48);
    if (!packet) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        exit(5);
    }

    // setup trim
    int left_min = 0, left_max = MYPWMRANGE, right_min = 0, right_max = MYPWMRANGE;
#ifdef  __linux__
    left_min = getIntFromConfig(gkf, "trim", "left_min", 0);
    left_max = getIntFromConfig(gkf, "trim", "left_max", MYPWMRANGE);
    right_min = getIntFromConfig(gkf, "trim", "right_min", 0);
    right_max = getIntFromConfig(gkf, "trim", "right_max", MYPWMRANGE);
#elif __WIN32__
    left_min = GetPrivateProfileInt("trim", "left_min", 0, CONFIG_FILE);
    left_max = GetPrivateProfileInt("trim", "left_max", MYPWMRANGE, CONFIG_FILE);
    right_min = GetPrivateProfileInt("trim", "right_min", 0, CONFIG_FILE);
    right_max = GetPrivateProfileInt("trim", "right_max", MYPWMRANGE, CONFIG_FILE);
#endif
    if (left_min < 0) left_min = 0;
    if (left_max > MYPWMRANGE) left_max = MYPWMRANGE;
    if (right_min < 0) right_min = 0;
    if (right_max > MYPWMRANGE) right_max = MYPWMRANGE;
    printTime();
    printf("Using trim config left_min=%i, left_max=%i, right_min=%i, right_max=%i\n", left_min, left_max, right_min, right_max);


    // setup button config variables
    // a (0), b (1), x (2), y (3), lb (4), rb (5), view (6), menu (7), xbox (8), ls (9), rs (10), up (11), down (12), left (13), right (14)
    struct buttonDefinition a_button, b_button, x_button, y_button, \
        lb_button, rb_button, \
        view_button, menu_button, xbox_button, \
        ls_button, rs_button, \
        up_button, down_button, left_button, right_button;
    struct buttonDefinition *(allbuttons[]) = {&a_button, &b_button, &x_button, &y_button, \
        &lb_button, &rb_button, \
        &view_button, &menu_button, &xbox_button, \
        &ls_button, &rs_button, \
        &up_button, &down_button, &left_button, &right_button};
    const char *buttonnames[] = {"a", "b", "x", "y", "lb", "rb", "view", "menu", "xbox", "ls", "rs", "up", "down", "left", "right"};

    // read in button config options
#ifdef  __linux__
    for (i = 0; i < NUM_BUTTONS; i++) {
        gerror = NULL;
        allbuttons[i]->value = g_key_file_get_value(gkf, "buttons", buttonnames[i], &gerror);
        if (gerror != NULL) {
            fprintf(stderr, "%s, assuming a is unset\n", gerror->message);
            a_button.value = NULL;
            g_error_free(gerror);
            gerror = NULL;
        }
    }

    g_key_file_free(gkf);
#elif __WIN32__
    for (i = 0; i < NUM_BUTTONS; i++) {
        allbuttons[i]->value = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
        GetPrivateProfileString("buttons", buttonnames[i], "", allbuttons[i]->value, STRING_BUFFER_LENGTH, CONFIG_FILE);
    });
#endif

    // for each button, read in its macro or set its type appropriately
    for (i = 0; i < NUM_BUTTONS; i++) {
        // start by handling special cases
        if (strcmp(allbuttons[i]->value, "fast") == 0) { //strcmp returns 0 when the strings match
            allbuttons[i]->type = FAST;
        }
        else if (strcmp(allbuttons[i]->value, "slow") == 0) {
            allbuttons[i]->type = SLOW;
        }
        else if (strcmp(allbuttons[i]->value, "invert1") == 0) {
            allbuttons[i]->type = INVERT1;
        }
        else if (strcmp(allbuttons[i]->value, "invert2") == 0) {
            allbuttons[i]->type = INVERT2;
        }
        else if (strcmp(allbuttons[i]->value, "enable") == 0) {
            allbuttons[i]->type = ENABLE;
        }
        else if (strcmp(allbuttons[i]->value, "stop") == 0) {
            allbuttons[i]->type = STOP;
        }
        else if (strcmp(allbuttons[i]->value, "exit") == 0) {
            allbuttons[i]->type = EXIT;
        }
        else if (strlen(allbuttons[i]->value) == 0) { // nothing is programmed
            allbuttons[i]->type = NONE;
        }
        else { // read in macro
        	if (access(allbuttons[i]->value, F_OK) != 0) { // the macro file does not exist!
                fprintf(stderr, "error reading %s, assuming no function\n", allbuttons[i]->value);
                allbuttons[i]->value = "";
                allbuttons[i]->type = NONE;
            }
            else {
                FILE *fid = fopen(allbuttons[i]->value, "r");
                fclose(fid);
            }
        }
    }

    // print a summary of the buttons
    printTime();
    printf("Using a=%s, b=%s, x=%s, y=%s\n", \
        a_button.value, b_button.value, x_button.value, y_button.value);
    printTime();
    printf("Using lb=%s, rb=%s, view=%s, menu=%s\n", \
        lb_button.value, rb_button.value, view_button.value, menu_button.value);
    printTime();
    printf("Using xbox=%s, ls=%s, rs=%s\n", \
        xbox_button.value, ls_button.value, rs_button.value);
    printTime();
    printf("Using up=%s, down=%s, left=%s, right=%s\n", \
        up_button.value, down_button.value, left_button.value, right_button.value);



    SDL_Event event;

    // Main loop
    int running = 1;
    while (running) {

        if (SDL_GetTicks() - lastPacketTime > HEARTBEAT_TIMEOUT) {
            sendPacket(0, 0);
//            printf("Sending heartbeat (%d)!\n", nextpacket);
        }

        // recieve all waiting packets
        while (SDLNet_UDP_Recv(udpsocket, packet) == 1) {
            printTime();
            printf("Packet recieved...\n");
        }

        while(SDL_PollEvent(&event) != 0)
        {
            switch(event.type)
            {
                case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
                    if (event.jaxis.axis == 1) { // Left up/down
                        if ((event.jaxis.value < -DEADZONE ) || (event.jaxis.value > DEADZONE)) {
                            if (invert*event.jaxis.value > 0) { // down
                                sendPacket(16, ((left_max - left_min) * (invert*event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) / speed  + left_min);
                            }
                            else { // up
                                sendPacket(15, ((left_max - left_min) * (invert*-event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) / speed  + left_min);
                            }
                        }
                        else {
                            sendPacket(15, 0);
                            sendPacket(16, 0);
                        }
                    }
                    else if (event.jaxis.axis == 4) { // Right up/down
                        if ((event.jaxis.value < -DEADZONE ) || (event.jaxis.value > DEADZONE)) {
                            if (invert*event.jaxis.value > 0) { // down
                                sendPacket(26, ((right_max - right_min) * (invert*event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) / speed + right_min);
                            }
                            else { // up
                                sendPacket(25, ((right_max - right_min) * (invert*-event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) / speed  + right_min);
                            }
                        }
                        else {
                            sendPacket(25, 0);
                            sendPacket(26, 0);
                        }
                    }
                    else {
                        if ((event.jaxis.value < -DEADZONE ) || (event.jaxis.value > DEADZONE)) {
//                            printf("axis: %i value: %i\n", event.jaxis.axis, event.jaxis.value);
                        }
                    }
                    break;

                case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Presses */
                    printTime();
                    printf("%s button: ", buttonnames[event.jbutton.button]);
                    executeButton(allbuttons[event.jbutton.button]);
                    break;

                case SDL_JOYHATMOTION:  /* Handle Hat Motion */
                    if (event.jhat.value == 1) {
                        printTime();
                        printf("up button: ");
                        executeButton(&up_button);
                    }
                    else if (event.jhat.value == 4) {
                        printTime();
                        printf("down button: ");
                        executeButton(&down_button);
                    }
                    else if (event.jhat.value == 8) {
                        printTime();
                        printf("left button: ");
                        executeButton(&left_button);
                    }
                    else if (event.jhat.value == 2) {
                        printTime();
                        printf("right button: ");
                        executeButton(&right_button);
                    }
                    else {
//                        printf("dpad %i pressed\n", event.jhat.value);
                    }
                    break;

                case SDL_QUIT:
                    running = 0;
                    printTime();
                    break;
            }
        }
   }

	return 0;
}
