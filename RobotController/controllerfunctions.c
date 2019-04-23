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
            printf("Enabling motors...\n");
            break;

        case DISABLE:
            sendPacket(remote, 11, 0); // disable left motor
            sendPacket(remote, 21, 0); // disable right motor
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
            break;

        case NONE:
            sendPacket(remote, 255, 0); // any other button, stop!
            printf("Assuming emergency stop!\n");
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
    dest->leftaxis = src->leftaxis;
    dest->rightaxis = src->rightaxis;
}

void updateMotor(UDPremote *remote, int id, float value, int min, int max) {
}
