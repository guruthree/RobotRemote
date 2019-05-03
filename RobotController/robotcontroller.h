#ifndef _ROBOTCONTROLLER_H_
#define _ROBOTCONTROLLER_H_ 1

#include "../robot.h"
#include <SDL2/SDL.h>

#define JOYSTICK_MAX 32768
#define DEADZONE (JOYSTICK_MAX/10)

// the relative path here is required for the Windows INI functions
#define CONFIG_FILE "./config.ini"
#define STRING_BUFFER_LENGTH 32

#define REMOTE_HOST "192.168.4.1"

// the number of buttons on the Xbox controller plus the D-pad directions
#ifndef __WIN32__
    #define NUM_BUTTONS (11+4)
#else // there is no xbox button under windows
    #define NUM_BUTTONS (10+4)
#endif

#ifndef MYDATE
    #define MYDATE "unknown"
#endif

extern SDL_Joystick *joystick;

typedef struct {
    int length;
    int *times;
//    float *left;
//    float *right;
    float *velocities[MAX_NUM_MOTORS];
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
    int enabled;
    float axis[MAX_NUM_MOTORS];
    Macro *macros;
    int *axismap;
    int numMotors;
} robotState;


#endif /* _ROBOTCONTROLLER_H_ */
