#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>

#include "robotcontroller.h"
#include "controllerfunctions.h"


void printTime() {
    struct timeb now;
    char buffer[26];
    ftime(&now);
    struct tm *mytime = localtime(&now.time);
    strftime(buffer, 26, "%H:%M:%S.", mytime);
    printf("%s%03i ", buffer, now.millitm);
}

void sendPacket(UDPremote *remote, Uint32 command, Uint32 argument) {
    remote->packet->address.host = remote->remoteAddr.host;
    remote->packet->address.port = remote->remoteAddr.port;

    remote->nextpacket++;
    SDLNet_Write32(remote->nextpacket, remote->packet->data);
    SDLNet_Write32(command, remote->packet->data+4);
    SDLNet_Write32(argument, remote->packet->data+8);

    remote->packet->len = 12;
    SDLNet_UDP_Send(remote->udpsocket, -1, remote->packet);
    remote->lastPacketTime = SDL_GetTicks();
//    printf("sending packet %i, %i, %i\n", remote->nextpacket, command, argument);
}

void executeButton(UDPremote *remote, robotState *robotstate, const buttonDefinition *button) {
    switch (button->type) {
        case ENABLE:
            sendPacket(remote, 10, 0); // enable left motor
            sendPacket(remote, 20, 0); // enable right motor
            robotstate->enabled = 1;
            printf("Enabling motors...\n");
            break;

        case DISABLE:
            sendPacket(remote, 11, 0); // disable left motor
            sendPacket(remote, 21, 0); // disable right motor
            robotstate->enabled = 0;
            printf("Disabling motors\n");
            break;

        case STOP:
            for (int i = 0; i < NUM_BUTTONS; i++) {
                if (robotstate->macros[i].length != 0) {
                    robotstate->macros[i].running = 0;
                }
            }
            robotstate->leftaxis = axisvalueconversion(SDL_JoystickGetAxis(joystick, 1));
            robotstate->rightaxis = axisvalueconversion(SDL_JoystickGetAxis(joystick, 4));
            printf("Interrupting running macros\n");
            break;

        case EXIT:
            printf("Exit?\n");
            SDL_Event sdlevent;
            sdlevent.type = SDL_QUIT;
            SDL_PushEvent(&sdlevent);
            break;

        case FAST:
            robotstate->speed = 1;
            printf("Speeding up\n");
            break;

        case SLOW:
            robotstate->speed = 2;
            printf("Slowing down\n");
            break;

        case INVERT1:
            robotstate->invert = 1;
            printf("Invert 1\n");
            break;

        case INVERT2:
            robotstate->invert = -1;
            printf("Invert 2\n");
            break;

        case MACRO:
            if (button->macro->length != 0) {
                button->macro->running = SDL_GetTicks();
                button->macro->at = 0;
                printf("Macro %s started\n", button->value);
            }
            break;

        case NONE:
            sendPacket(remote, 255, 0); // any other button, stop!
            robotstate->enabled = 0;
            robotstate->leftaxis = 0;
            robotstate->rightaxis = 0;
            for (int i = 0; i < NUM_BUTTONS; i++) {
                if (robotstate->macros[i].length != 0) {
                    robotstate->macros[i].running = 0;
                }
            }
            printf("Assuming emergency stop! Disabling & stopping motors and macros.\n");
            break;
    }
}

#ifdef __linux__
int getIntFromConfig(GKeyFile* gkf, char *section, char *key, int def) {
    if (gkf == NULL) {
        return def;
    }
    GError *gerror = NULL;
    int temp = g_key_file_get_integer(gkf, section, key, &gerror);
    if (gerror != NULL) {
        fprintf(stderr, "%s, assuming joystick 0\n",(gerror->message));
        temp = def;
        g_error_free(gerror);
    }
    return temp;
}
#endif

void copystate(robotState *src, robotState *dest) {
    dest->speed = src->speed;
    dest->invert = src->invert;
    dest->enabled = src->enabled;
    dest->leftaxis = src->leftaxis;
    dest->rightaxis = src->rightaxis;
    dest->macros = src->macros;
}

void updateMotor(UDPremote *remote, int id, float value, int min, int max) {
    if (value > 0) {
        sendPacket(remote, id+5, (int)((max - min) * value + min));
    }
    else if (value == 0) {
        sendPacket(remote, id+5, 0);
        sendPacket(remote, id+6, 0);
    }
    else { // value < 0
        sendPacket(remote, id+6, (int)((max - min) * -value + min));
    }
}

int readMacro(char filename[], Macro *macro) {
    FILE *fid = fopen(filename, "r");
    int ret = fscanf(fid, "%i", &macro->length);
    if (ret != 1 || macro->length <= 0) {
        fclose(fid);
        return 0;
    }

    printTime();
    printf("Reading macro %s, length %i\n", filename, macro->length);
    macro->times = (int*)malloc(sizeof(int)*macro->length);
    macro->left = (float*)malloc(sizeof(float)*macro->length);
    macro->right = (float*)malloc(sizeof(float)*macro->length);

    for (int i = 0; i < macro->length; i++) {
        ret = fscanf(fid, "%i,%f,%f", &macro->times[i], &macro->left[i], &macro->right[i]);
        if (ret != 3) {
            fclose(fid);
            return 0;
        }
        if (i > 0) {
            macro->times[i] += macro->times[i-1];
        }
    }
    fclose(fid);

    return 1;
}

float axisvalueconversion(Sint16 value) {
    if (value < -DEADZONE ) { // up
        return -((float)value + DEADZONE) / (JOYSTICK_MAX - DEADZONE);
    }
    else if (value > DEADZONE) { // down
        return -((float)value - DEADZONE) / (JOYSTICK_MAX - DEADZONE);
    }
    else {
        return 0;
    }
}
