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

void executeButton(UDPremote *remote, robotState *robottsate, const buttonDefinition *button);

#ifdef __linux__
int getIntFromConfig(GKeyFile* gkf, const char *section, const char *key, const int def);
const char* getStringFromConfig(GKeyFile* gkf, const char *section, const char *key, char *def);
#endif

void copystate(robotState *src, robotState *dest);

void updateMotor(UDPremote *remote, int id, float value, int min, int max, int dir);

int readMacro(char filename[], Macro *macro);

float axisvalueconversion(Sint16 value);

#endif /* _CONTROLLERFUNCTIONS_H_ */
