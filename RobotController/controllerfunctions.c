#include <time.h>
#include <sys/timeb.h>

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
