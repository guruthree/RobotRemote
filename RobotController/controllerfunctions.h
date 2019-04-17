#ifndef _CONTROLLERFUNCTIONS_H_
#define _CONTROLLERFUNCTIONS_H_ 1

#include <SDL2/SDL.h>

void cleanup();
void sendPacket(Uint32 command, Uint32 argument);
void printTime();

#endif /* _CONTROLLERFUNCTIONS_H_ */
