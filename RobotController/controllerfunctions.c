#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>

#include "../robot.h"
#include "robotcontroller.h"
#include "controllerfunctions.h"

#ifdef DEBUG // so that we can get around debug compile error for defined but not used
char** getMotorNames() {
    return motornames;
}
#endif

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
            for (int i = 0; i < robotstate->numMotors; i++) {
                sendPacket(remote, (i+1)*10, 0); // enable (motor IDs start at 0, but packets start at 10)
            }
            robotstate->enabled = 1;
            printf("Enabling motors...\n");
            break;

        case DISABLE:
            for (int i = 0; i < robotstate->numMotors; i++) {
                sendPacket(remote, (i+1)*10+1, 0); // disable motor
            }
            robotstate->enabled = 0;
            printf("Disabling motors\n");
            break;

        case STOP:
            for (int i = 0; i < NUM_BUTTONS; i++) {
                if (robotstate->macros[i].length != 0) {
                    robotstate->macros[i].running = 0;
                }
            }
            for (int i = 0; i < robotstate->numMotors; i++) {
                robotstate->axis[i] = axisvalueconversion(SDL_JoystickGetAxis(joystick, robotstate->axismap[i]));
            }
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

        case INVERTOFF:
            robotstate->invert = 0;
            printf("Invert Off\n");
            break;

        case INVERTON:
            robotstate->invert = 1;
            printf("Invert On\n");
            break;

        case MACRO:
            if (button->macro->length != 0) {
                if (robotstate->enabled == 1) {
                    button->macro->running = SDL_GetTicks();
                    button->macro->at = 0;
                    printf("Macro %s started\n", button->value);
                }
                else {
                    printf("Macro %s would have been started, but motors disabled\n", button->value);
                }
            }
            break;

        case NONE:
            sendPacket(remote, 255, 0); // any other button, stop!
            robotstate->enabled = 0;
            for (int i = 0; i < robotstate->numMotors; i++) {
                robotstate->axis[i] = 0;
            }
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
int getIntFromConfig(GKeyFile* gkf, const char *section, const char *key, const int def) {
    if (gkf == NULL) {
        return def;
    }
    GError *gerror = NULL;
    int temp = g_key_file_get_integer(gkf, section, key, &gerror);
    if (gerror != NULL) {
//        fprintf(stderr, "%s, assuming [%s] %s value %i\n", gerror->message, section, key, def);
        temp = def;
        g_error_free(gerror);
    }
    return temp;
}

char* getStringFromConfig(GKeyFile* gkf, const char *section, const char *key, char *def) {
    if (gkf == NULL) {
        return def;
    }
    GError *gerror = NULL;
    char* temp = g_key_file_get_value(gkf, section, key, &gerror);
    if (gerror != NULL) {
//        fprintf(stderr, "%s, assuming [%s] %s value %s\n", (gerror->message), section, key, def);
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
    for (int i = 0; i < src->numMotors; i++) {
        dest->axis[i] = src->axis[i];
    }
    dest->macros = src->macros;
    dest->axismap = src->axismap;
    dest->numMotors = src->numMotors;
}

void updateMotor(UDPremote *remote, int id, float value, int min, int max, int dir) {
    value *= dir;
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

int readMacro(char filename[], Macro *macro, int numMotors) {
    FILE *fid = fopen(filename, "r");
    int ret = fscanf(fid, "%i", &macro->length);
    if (ret != 1 || macro->length <= 0) {
        fclose(fid);
        return 0;
    }

    printTime();
    printf("Reading macro %s, length %i\n", filename, macro->length);
    macro->times = (unsigned long*)malloc(sizeof(int)*macro->length);
    for (int i = 0; i < numMotors; i++) {
        macro->velocities[i] = (float*)malloc(sizeof(float)*macro->length);
    }

    for (int i = 0; i < macro->length; i++) {
        ret = fscanf(fid, "%li", &macro->times[i]);
        if (ret != 1) {
            fclose(fid);
            return 0;
        }
        for (int j = 0; j < numMotors; j++) {
            ret = fscanf(fid, ",%f", &macro->velocities[j][i]);
            if (ret != 1) {
                fclose(fid);
                return 0;
            }
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
