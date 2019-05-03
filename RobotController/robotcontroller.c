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
UDPremote remote;
const char *buttonnames[] = {"a", "b", "x", "y", "lb", "rb", "view", "menu", 
#ifndef __WIN32__
    "xbox", 
#endif
"ls", "rs", "up", "down", "left", "right"};

void cleanup() {
    printf("Exiting...\n");
    if (remote.packet && remote.udpsocket)
        sendPacket(&remote, 255, 0); // make sure the motors are stopped before we go
    if (remote.packet)
        SDLNet_FreePacket(remote.packet);
    remote.packet = NULL;
    if (remote.udpsocket)
        SDLNet_UDP_Close(remote.udpsocket);
    remote.udpsocket = NULL;
    if (joystick)
        SDL_JoystickClose(joystick);
    joystick = NULL;
    SDLNet_Quit();
    SDL_Quit();
    exit(0);
}


int main(){ //int argc, char **argv) {
    int i, j;
    unsigned long now;
    int axismap[MAX_NUM_MOTORS];
    robotState robotstate;
    robotstate.speed = 1; // fast
    robotstate.invert = 1;
    robotstate.enabled = 0;
    for (i = 0; i < MAX_NUM_MOTORS; i++) {
        robotstate.axis[i] = 0;
        axismap[i] = -1;
    }
    robotstate.axismap = axismap;
    robotState laststate;
    copystate(&robotstate, &laststate);

    // Handle internal quits nicely
    atexit(cleanup);


    printTime();
    printf("Initialising RobotController, from %s...\n", MYDATE);

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
    char *remote_host;
    int server_port;
#ifdef  __linux__
    remote_host = getStringFromConfig(gkf, "network", "remote_host", REMOTE_HOST);
    server_port = getIntFromConfig(gkf, "network", "server_port", SERVER_PORT);
#elif __WIN32__
    remote_host = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
    GetPrivateProfileString("network", "remote_host", REMOTE_HOST, remote_host, STRING_BUFFER_LENGTH, CONFIG_FILE);
    server_port = GetPrivateProfileInt("network", "server_port", SERVER_PORT, CONFIG_FILE);
#endif
    if (server_port < 0 || server_port > 65534) {
        server_port = SERVER_PORT;
    }
    printTime();
    printf("Sending to %s:%i\n", remote_host, server_port);

    remote.nextpacket = 0;
    remote.lastPacketTime = SDL_GetTicks();
    SDLNet_ResolveHost(&remote.remoteAddr, remote_host, server_port);
    remote.udpsocket = SDLNet_UDP_Open(0);
    if (!remote.udpsocket) {
        fprintf(stderr, "SDLNet_UDP_Open: %s\n", SDLNet_GetError());
        exit(4);
    }
    remote.packet = SDLNet_AllocPacket(PACKET_LENGTH);
    if (!remote.packet) {
        fprintf(stderr, "SDLNet_AllocPacket: %s\n", SDLNet_GetError());
        exit(5);
    }


    // read in robot configurations
    int numMotors = 2;
#ifdef  __linux__
    numMotors = getIntFromConfig(gkf, "robot", "num_motors", 2);
#elif __WIN32__
    numMotors = GetPrivateProfileInt("robot", "num_motors", 2, CONFIG_FILE);
#endif
    if (numMotors < 0 || numMotors > MAX_NUM_MOTORS) {
        numMotors = SERVER_PORT;
    }
    printTime();
    printf("Using %i motors\n", numMotors);
    robotstate.numMotors = numMotors;
    laststate.numMotors = numMotors;


    // setup trim
    int trim_min[MAX_NUM_MOTORS], trim_max[MAX_NUM_MOTORS];
    char *keyname = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
    char *keyname2 = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
    for (i = 0; i < numMotors; i++) {
        snprintf(keyname, STRING_BUFFER_LENGTH, "%s_min", motornames[i]);
        snprintf(keyname2, STRING_BUFFER_LENGTH, "%s_max", motornames[i]);
#ifdef  __linux__
        trim_min[i] = getIntFromConfig(gkf, "trim", keyname, 0);
        trim_max[i] = getIntFromConfig(gkf, "trim", keyname2, MYPWMRANGE);
#elif __WIN32__
        trim_min[i] = GetPrivateProfileInt("trim", keyname, 0, CONFIG_FILE);
        trim_max[i] = GetPrivateProfileInt("trim", keyname2, MYPWMRANGE, CONFIG_FILE);
#endif
        if (trim_min[i] < 0) trim_min[i] = 0;
        if (trim_max[i] > MYPWMRANGE) trim_max[i] = MYPWMRANGE;
    }
    free(keyname);
    free(keyname2);

    printTime();
    printf("Using ");
    for (i = 0; i < numMotors; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%s_min = %i, %s_max = %i", motornames[i], trim_min[i], motornames[i], trim_max[i]);
    }
    printf("\n");


    // read in motor directions
    int axis_dir[MAX_NUM_MOTORS];
    keyname = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
    for (i = 0; i < numMotors; i++) {
        snprintf(keyname, STRING_BUFFER_LENGTH, "%s_dir", motornames[i]);
#ifdef  __linux__
        axis_dir[i] = getIntFromConfig(gkf, "trim", keyname, 1);
#elif __WIN32__
        axis_dir[i] = GetPrivateProfileInt("trim", keyname, 1, CONFIG_FILE);
#endif
        if (axis_dir[i] != -1 && axis_dir[i] != 1) axis_dir[i] = 1;
    }
    free(keyname);

    printTime();
    printf("Using dir config ");
    for (i = 0; i < numMotors; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%s_dir=%i", motornames[i], axis_dir[i]);
    }
    printf("\n");


    // read in axis mapping
    keyname = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
    for (i = 0; i < numMotors; i++) {
        snprintf(keyname, STRING_BUFFER_LENGTH, "%s_axis", motornames[i]);
#ifdef  __linux__
        axismap[i] = getIntFromConfig(gkf, "axis", keyname, -1);
#elif __WIN32__
        axismap[i] = GetPrivateProfileInt("axis", keyname, -1, CONFIG_FILE);
#endif
        if (axismap[i] < 0 || axismap[i] > 6) axismap[i] = -1;
    }
    free(keyname);

    printTime();
    printf("Using axis mapping ");
    for (i = 0; i < numMotors; i++) {
        if (i > 0) {
            printf(", ");
        }
        printf("%s_axis=%i", motornames[i], axismap[i]);
    }
    printf("\n");


    // setup button config variables
    // a (0), b (1), x (2), y (3), lb (4), rb (5), view (6), menu (7), xbox (8), ls (9), rs (10), up (11), down (12), left (13), right (14)
    buttonDefinition a_button, b_button, x_button, y_button, 
        lb_button, rb_button, 
        view_button, menu_button, 
#ifndef __WIN32__
        xbox_button, 
#endif 
        ls_button, rs_button, 
        up_button, down_button, left_button, right_button;
    buttonDefinition *(allbuttons[]) = {&a_button, &b_button, &x_button, &y_button, 
        &lb_button, &rb_button, 
        &view_button, &menu_button, 
#ifndef __WIN32__
        &xbox_button, 
#endif
        &ls_button, &rs_button, 
        &up_button, &down_button, &left_button, &right_button};

    Macro macros[NUM_BUTTONS];
    robotstate.macros = &macros[0];
    laststate.macros = &macros[0];

    // read in button config options
#ifdef  __linux__
    for (i = 0; i < NUM_BUTTONS; i++) {
        allbuttons[i]->value = getStringFromConfig(gkf, "buttons", buttonnames[i], NULL);
    }

    g_key_file_free(gkf); // this is the last config we need to read, so close it
#elif __WIN32__
    for (i = 0; i < NUM_BUTTONS; i++) {
        allbuttons[i]->value = (char *)malloc(STRING_BUFFER_LENGTH * sizeof(char));
        GetPrivateProfileString("buttons", buttonnames[i], "", allbuttons[i]->value, STRING_BUFFER_LENGTH, CONFIG_FILE);
    }
#endif

    // for each button, read in its macro or set its type appropriately
    for (i = 0; i < NUM_BUTTONS; i++) {
        macros[i].length = -1;
        macros[i].running = 0;
        allbuttons[i]->macro = &macros[i];

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
                fprintf(stderr, "file %s doesn't exist, assuming no macro\n", allbuttons[i]->value);
                allbuttons[i]->value = "";
                allbuttons[i]->type = NONE;
            }
            else {
                allbuttons[i]->type = MACRO;
                if (readMacro(allbuttons[i]->value, allbuttons[i]->macro, numMotors) == 0) {
                    fprintf(stderr, "formatting error reading macro %s\n", allbuttons[i]->value); 
                    exit(6);
                }
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
#ifndef __WIN32__
    printf("Using xbox=%s, ls=%s, rs=%s\n", \
        xbox_button.value, ls_button.value, rs_button.value);
#else
    printf("Using ls=%s, rs=%s\n", \
        ls_button.value, rs_button.value);
#endif
    printTime();
    printf("Using up=%s, down=%s, left=%s, right=%s\n", \
        up_button.value, down_button.value, left_button.value, right_button.value);


    // Main loop
    SDL_Event event;
    int running = 1;
    while (running) {
        copystate(&robotstate, &laststate);

        if (SDL_GetTicks() - remote.lastPacketTime > HEARTBEAT_TIMEOUT) {
            sendPacket(&remote, 0, 0);
//            printf("Sending heartbeat (%d)!\n", nextpacket);
        }

        // recieve all waiting packets
        while (SDLNet_UDP_Recv(remote.udpsocket, remote.packet) == 1) {
            printTime();
            printf("Packet recieved...\n");
        }

        while(SDL_PollEvent(&event) != 0) {
            switch(event.type) {
                case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
                    for (i = 0; i < numMotors; i++) {
                        if (event.jaxis.axis == axismap[i]) {
                            robotstate.axis[i] = axisvalueconversion(event.jaxis.value);
                        }
                    }
                    break;

                case SDL_JOYBUTTONDOWN:  /* Handle Joystick Button Presses */
                    printTime();
                    printf("%s button: ", buttonnames[event.jbutton.button]);
                    executeButton(&remote, &robotstate, allbuttons[event.jbutton.button]);
                    break;

                case SDL_JOYHATMOTION:  /* Handle Hat Motion */
                    if (event.jhat.value == 1) {
                        printTime();
                        printf("up button: ");
                        executeButton(&remote, &robotstate, &up_button);
                    }
                    else if (event.jhat.value == 4) {
                        printTime();
                        printf("down button: ");
                        executeButton(&remote, &robotstate, &down_button);
                    }
                    else if (event.jhat.value == 8) {
                        printTime();
                        printf("left button: ");
                        executeButton(&remote, &robotstate, &left_button);
                    }
                    else if (event.jhat.value == 2) {
                        printTime();
                        printf("right button: ");
                        executeButton(&remote, &robotstate, &right_button);
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

        now = SDL_GetTicks();
        for (i = 0; i < NUM_BUTTONS; i++) {
            if (macros[i].length > 0 && macros[i].running > 0) {
                if (macros[i].at == 0 || ((now - macros[i].running < macros[i].times[macros[i].at] && now - macros[i].running > macros[i].times[macros[i].at-1]) && macros[i].at < macros[i].length)) {
                    printTime();
                    printf("Macro %s command %i/%i (", allbuttons[i]->value, macros[i].at+1, macros[i].length);
                    for (j = 0; j < numMotors; j++) {
                        if (j > 0) {
                            printf(", ");
                        }
                        printf("%f", macros[i].velocities[j][macros[i].at]);
                    }
                    printf(")\n");
//                    robotstate.axis[LEFT] = macros[i].left[macros[i].at];
//                    robotstate.axis[RIGHT] = macros[i].right[macros[i].at];
                    for (j = 0; j < numMotors; j++) {
                        robotstate.axis[j] = macros[i].velocities[j][macros[i].at];
                    }
                    macros[i].at++;
                }                        
                else if (now - macros[i].running > macros[i].times[macros[i].at-1]) {
                    printTime();
                    printf("Macro %s finished\n", allbuttons[i]->value);
                    for (j = 0; j < numMotors; j++) {
                        robotstate.axis[j] = axisvalueconversion(SDL_JoystickGetAxis(joystick, axismap[j]));
                    }
                    macros[i].running = 0;
                }
            }
        }


        if (robotstate.enabled == 1) {    
            if (robotstate.axis[LEFT] != laststate.axis[LEFT] || robotstate.speed != laststate.speed || robotstate.invert != laststate.invert) {
                if (robotstate.invert == 1) {
                    updateMotor(&remote, 10, robotstate.axis[LEFT] / robotstate.speed, trim_min[LEFT], trim_max[LEFT], axis_dir[LEFT]);
                }
                else {
                    updateMotor(&remote, 20, -robotstate.axis[LEFT] / robotstate.speed, trim_min[RIGHT], trim_max[RIGHT], axis_dir[RIGHT]);
                }
            }
            if (robotstate.axis[RIGHT] != laststate.axis[RIGHT] || robotstate.speed != laststate.speed || robotstate.invert != laststate.invert) {
                if (robotstate.invert == 1) {
                    updateMotor(&remote, 20, robotstate.axis[RIGHT] / robotstate.speed, trim_min[RIGHT], trim_max[RIGHT], axis_dir[RIGHT]);
                }
                else {
                    updateMotor(&remote, 10, -robotstate.axis[RIGHT] / robotstate.speed, trim_min[LEFT], trim_max[LEFT], axis_dir[LEFT]);
                }
            }
        }
    }

    // we're quitting, stop everything!
    sendPacket(&remote, 255, 0);

    for (i = 0; i < NUM_BUTTONS; i++) {
        if (allbuttons[i]->macro->length > 0) {
            if (allbuttons[i]->macro->times != NULL) {
                free(allbuttons[i]->macro->times);
            }
            for (j = 0; j < numMotors; j++) {
                if (allbuttons[i]->macro->velocities[j] != NULL) {
                    free(allbuttons[i]->macro->velocities[j]);
                }
            }
        }
#if __WIN32__
        if (allbuttons[i]->value != NULL) {
            free(allbuttons[i]->value);
        }
#endif
    }


    return 0;
}
