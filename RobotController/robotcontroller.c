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
#include <time.h>
#include <sys/timeb.h>

#define JOYSTICK_MAX 32768
#define DEADZONE (JOYSTICK_MAX/10)

// the relative path here is required for the Windows INI functions
#define CONFIG_FILE "./config.ini"

#define MYPWMRANGE 255

#define REMOTE_HOST "192.168.4.1"
#define REMOTE_PORT 7245
#define HEARTBEAT_TIMEOUT 500

void cleanup();
void sendPacket(Uint32 command, Uint32 argument);
void printTime();

SDL_Joystick *joystick;
UDPsocket udpsocket;
IPaddress remoteAddr;
UDPpacket *packet;
Uint32 nextpacket = 0;
unsigned long lastPacketTime = 0;

#ifdef __linux__
    GKeyFile* gkf;
    GError *gerror;
#endif

struct buttonDefinition {
    char *value;
    int type; // 0 - macro, 1 - fast, 2 - slow, 3 - invert1, 4 - invert2
};

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
#ifdef __linux__
    if (gkf)
        g_key_file_free(gkf);
    gkf = NULL;
    if (gerror)
        g_error_free(gerror);
    gerror = NULL;
#endif
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

void printTime() {
    struct timeb now;
    char buffer[26];
    ftime(&now);
    struct tm *mytime = localtime(&now.time);
    strftime(buffer, 26, "%H:%M:%S.", mytime);
    printf("%s%03i ", buffer, now.millitm);
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
    gerror = NULL;
    gkf = g_key_file_new();
    if (!g_key_file_load_from_file(gkf, CONFIG_FILE, G_KEY_FILE_NONE, &gerror)) {
        fprintf(stderr, "Could not read config file %s, %s\n", CONFIG_FILE, gerror->message);
        g_error_free(gerror);
        gerror = NULL;
        exit(2);
    }

    gerror = NULL;
    joystickID = g_key_file_get_integer(gkf, "controller", "id", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming joystick 0\n",(gerror->message));
        joystickID = 0;
        g_error_free(gerror);
        gerror = NULL;
    }
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
    SDLNet_ResolveHost(&remoteAddr, REMOTE_HOST, REMOTE_PORT);
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
    gerror = NULL;
    left_min = g_key_file_get_integer(gkf, "trim", "left_min", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming left_min=0\n", gerror->message);
        left_min = 0;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    left_max = g_key_file_get_integer(gkf, "trim", "left_max", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming left_max=%i\n", gerror->message, MYPWMRANGE);
        left_max = MYPWMRANGE;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    right_min = g_key_file_get_integer(gkf, "trim", "right_min", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming right_min=0\n", gerror->message);
        right_min = 0;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    right_max = g_key_file_get_integer(gkf, "trim", "right_max", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming right_max=%i\n", gerror->message, MYPWMRANGE);
        right_max = MYPWMRANGE;
        g_error_free(gerror);
        gerror = NULL;
    }
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


    // read in button config options
    struct buttonDefinition a_button, b_button, x_button, y_button, up_button, down_button, left_button, right_button;
#ifdef  __linux__
    gerror = NULL;
    a_button.value = g_key_file_get_value(gkf, "buttons", "a", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming a is unset\n", gerror->message);
        a_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    b_button.value = g_key_file_get_value(gkf, "buttons", "b", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming b is unset\n", gerror->message);
        b_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    x_button.value = g_key_file_get_value(gkf, "buttons", "x", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming x is unset\n", gerror->message);
        x_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    y_button.value = g_key_file_get_value(gkf, "buttons", "y", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming y is unset\n", gerror->message);
        y_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    up_button.value = g_key_file_get_value(gkf, "buttons", "up", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming up is unset\n", gerror->message);
        up_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    down_button.value = g_key_file_get_value(gkf, "buttons", "down", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming down is unset\n", gerror->message);
        down_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    left_button.value = g_key_file_get_value(gkf, "buttons", "left", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming left is unset\n", gerror->message);
        left_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }

    gerror = NULL;
    right_button.value = g_key_file_get_value(gkf, "buttons", "right", &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming right is unset\n", gerror->message);
        right_button.value = NULL;
        g_error_free(gerror);
        gerror = NULL;
    }
#elif __WIN32__
    a_button.value = GetPrivateProfileString("buttons", "a", 0, CONFIG_FILE);
    b_button.value = GetPrivateProfileString("buttons", "b", 0, CONFIG_FILE);
    x_button.value = GetPrivateProfileString("buttons", "x", 0, CONFIG_FILE);
    y_button.value = GetPrivateProfileString("buttons", "y", 0, CONFIG_FILE);
    up_button.value = GetPrivateProfileString("buttons", "up", 0, CONFIG_FILE);
    down_button.value = GetPrivateProfileString("buttons", "down", 0, CONFIG_FILE);
    left_button.value = GetPrivateProfileString("buttons", "left", 0, CONFIG_FILE);
    right_button.value = GetPrivateProfileString("buttons", "right", 0, CONFIG_FILE);
#endif


    printTime();
    printf("Using a=%s, b=%s, x=%s, y=%s up=%s, down=%s, left=%s, right=%s\n", \
        a_button.value, b_button.value, x_button.value, y_button.value, \
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
                            if (event.jaxis.value > 0) { // down
                                sendPacket(16, ((left_max - left_min) * (event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) + left_min);
                            }
                            else { // up
                                sendPacket(15, ((left_max - left_min) * (-event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) + left_min);
                            }
                        }
                        else {
                            sendPacket(15, 0);
                            sendPacket(16, 0);
                        }
                    }
                    else if (event.jaxis.axis == 4) { // Right up/down
                        if ((event.jaxis.value < -DEADZONE ) || (event.jaxis.value > DEADZONE)) {
                            if (event.jaxis.value > 0) { // down
                                sendPacket(26, ((right_max - right_min) * (event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) + right_min);
                            }
                            else { // up
                                sendPacket(25, ((right_max - right_min) * (-event.jaxis.value - DEADZONE)) / (JOYSTICK_MAX - DEADZONE) + right_min);
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
                    if (event.jbutton.button == 5) { // RB
                        sendPacket(10, 0); // enable left motor
                        sendPacket(20, 0); // enable right motor
                        printTime();
                        printf("Enabling motors...\n");
                    }
                    else if (event.jbutton.button == 6 || event.jbutton.button == 8) { // View or XBox Button
                        running = 0;
                        printTime();
                        cleanup();
                    }
                    else {
                        sendPacket(255, 0); // any other button, stop!
                        printTime();
                        printf("Button %i pressed, assuming emergency stop!\n", event.jbutton.button);
                    }
                    
                break;

                case SDL_JOYHATMOTION:  /* Handle Hat Motion */
                    printf("dpad %i pressed\n", event.jhat.value);
                break;

                case SDL_QUIT:
                    running = 0;
                    printTime();
                    cleanup();
                    break;
            }
        }
   }

	return 0;
}
