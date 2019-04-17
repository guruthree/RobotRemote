#ifndef _CONTROLLERFUNCTIONS_H_
#define _CONTROLLERFUNCTIONS_H_ 1

#include <SDL2/SDL.h>
#ifdef __linux__
    #include <glib.h>
#endif

void cleanup();
void sendPacket(Uint32 command, Uint32 argument);
void printTime();


#ifdef __linux__

int getIntFromConfig(GKeyFile* gkf, char *section, char *key, int def);

#endif

#endif /* _CONTROLLERFUNCTIONS_H_ */
