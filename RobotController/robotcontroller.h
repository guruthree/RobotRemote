#ifndef _ROBOTCONTROLLER_H_
#define _ROBOTCONTROLLER_H_ 1


#include <SDL2/SDL.h>

#define JOYSTICK_MAX 32768
#define DEADZONE (JOYSTICK_MAX/10)

// the relative path here is required for the Windows INI functions
#define CONFIG_FILE "./config.ini"

#define REMOTE_HOST "192.168.4.1"

// the number of buttons on the Xbox controller plus the D-pad directions
#define NUM_BUTTONS (11+4)

extern SDL_Joystick *joystick;

typedef struct {
    int length;
    int *times;
    float *left;
    float *right;
    int running;
    int at;
} Macro;

// things each Xbox controller button can do
typedef enum {NONE, MACRO, FAST, SLOW, INVERT1, INVERT2, ENABLE, DISABLE, STOP, EXIT} buttonType;
typedef struct {
    char *value;
    buttonType type; // value from enum buttonType
    Macro *macro;
} buttonDefinition;
extern const char *buttonnames[];

typedef struct {
    int speed; // = 1; // 1 - fast, 2 - slow
    int invert; // = 1; // 1 or -1
    float leftaxis;
    float rightaxis;
    Macro *macros;
} robotState;


#endif /* _ROBOTCONTROLLER_H_ */
