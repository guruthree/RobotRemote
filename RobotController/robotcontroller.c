#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

SDL_Joystick *joystick;

void cleanup() {
    if (joystick)
        SDL_JoystickClose(joystick);
    joystick = NULL;
    SDL_Quit();
    exit(0);
}

int main(){ //int argc, char **argv) {
    int i;

    // Handle internal quits nicely
    atexit(cleanup);

	printf("Initialising...\n");

    if (SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK ) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    printf("%i joysticks were found.\n\n", SDL_NumJoysticks() );
    printf("The names of the joysticks are:\n");
	
    for (i = 0; i < SDL_NumJoysticks(); i++) {
        printf("    %s\n", SDL_JoystickName(i));
    }

    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(0);
    if (joystick == NULL) {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Event event;

    // Main loop
    int running = 1;
    while (running) {

        while(SDL_PollEvent(&event) != 0)
        {  
            switch(event.type)
            {  
                case SDL_JOYAXISMOTION:  /* Handle Joystick Motion */
                    printf("axis: %i value: %i\n", event.jaxis.axis, event.jaxis.value);
/*                    if ((event.jaxis.value < -3200 ) || (event.jaxis.value > 3200)) {
                        if (event.jaxis.axis == 0) {
                            printf("LR: %i\n", event.jaxis.value);
                        }

                        if (event.jaxis.axis == 1) {
                            printf("UD: %i\n", event.jaxis.value);
                        }
                    }*/
                    break;

                case SDL_QUIT:
                    running = 0;
                    cleanup();
                    break;
            }
        }
   }

	return 0;
}
