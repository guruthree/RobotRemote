#ifndef _CONTROLLERFUNCTIONS_H_
#define _CONTROLLERFUNCTIONS_H_ 1

#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>
#ifdef __linux__
    #include <glib.h>
#endif

void printTime();

typedef struct {
    UDPsocket udpsocket;
    IPaddress remoteAddr;
    UDPpacket *packet;
    Uint32 nextpacket;
    unsigned long lastPacketTime;
} UDPremote;

void sendPacket(UDPremote *remote, Uint32 command, Uint32 argument);

void cleanup();


#ifdef __linux__

int getIntFromConfig(GKeyFile* gkf, char *section, char *key, int def);

#endif

#endif /* _CONTROLLERFUNCTIONS_H_ */
