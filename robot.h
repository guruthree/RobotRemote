/* constants for both the controller and receiver */

#ifndef _ROBOT_H_
#define _ROBOT_H_ 1


// Local PWM range to match what the remote sends
#define MYPWMRANGE 255

// Wifi settings
#define SSID "RobotzRul"
#define WIFIPASS "omglolwut"

// Server settings
#define SERVER_PORT 7245

// Networking protocol settings
#define CONTROLLER_TIMEOUT 3000 // timeout in milliseconds of last packet recieved
#define EMERGENCY_STOP_TIMEOUT 1000 // accept no new packets after an emergency stop for X ms
#define HEARTBEAT_TIMEOUT 500 // send a heart beat every X ms
#define PACKET_LENGTH 48 // maximum number of bytes in a packet

// Define motors
#define MAX_NUM_MOTORS 5 // arduino code doesn't use this yet
typedef enum {LEFT, RIGHT, MOTOR2, MOTOR3, MOTOR4} motor;
static char *motorsnames[] = {"left", "right", "motor2", "motor3", "motor4"};


#endif /* _ROBOT_H_ */
