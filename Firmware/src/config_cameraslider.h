#ifndef __CAMERASLIDER_CFG__
#define __CAMERASLIDER_CFG__

// Configure oTA
#define MDNS_NAME       "sk_camera_slider"
#define OTA_NAME        "sk_camera_slider"
#define OTA_PASSWORD    "change_this_please"

// Various GPIO connections to control motor direction
// enable/disbale, reset, end/limit switch etc.
#define PIN_MOTOR_X_STEP			19
#define PIN_MOTOR_X_DIR             18
#define PIN_MOTOR_Z_STEP            17
#define PIN_MOTOR_Z_DIR             16
#define PIN_MTR_nRST                5
#define PIN_MTR_nEN                 4
#define PIN_LED 					2
#define PIN_END_SWICH_X			    27
#define PIN_OEND_SWICH_X			23
//#define PIN_SCL                     22
//#define PIN_SDA                     21
#define PIN_JOY_X			        32
#define PIN_JOY_Y			        33
#define PIN_JOY_SW			        25

#define PIN_A_D			            26

#define PIN_ENC_CLK			        14
#define PIN_ENC_DT			        13
#define PIN_ENC_SW			        12


// "Mechanical" configuration of the camera slider
#define RAIL_LENGTH_MM			    1244       // Rail (2020 extrusion) length that platform can slide along (in milimeters)
#define SLIDE_STEPS_PER_MM          187
#define MIN_STEP_SLIDER             5         // Minimum allowed step for sliding platform
#define PAN_STEPS_PER_DEGREE        78


// Positioning and speed default values
// In normal operation, these values will get overwritten by HTTP request
#define DEFAULT_SLIDE_TO_POS_SPEED  30.0
#define DEFAULT_SLIDE_TO_POS_ACCEL  60.0
#define DEFAULT_ROTATE_TO_POS_SPEED 30.0
#define DEFAULT_ROTATE_TO_POS_ACCEL 60.0


#define DEFAULT_HOMING_SPEED_SLIDE  DEFAULT_SLIDE_TO_POS_SPEED
#define DEFAULT_HOMING_SPEED_PAN    PAN_STEPS_PER_DEGREE

// Define the OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Homing settings

typedef enum 
{ 
	SLIDER_FIRST = 0,
    SLIDER_MOTORS_OFF, 
    SLIDER_IDLE, 
    SLIDER_HOMING,
    SLIDER_MOVING_TO_START,
    SLIDER_MOVING_TO_END,
    SLIDER_READY,
    SLIDER_WORKING,
    SLIDER_LAST
} sliderState_t;


typedef enum
{
    HOMING_DIRECTION = 0,
    SLIDING_DIRECTION,
    PAN_DIRECTION,
    EXACT_TIME,
    SLIDER_STEPS_PER_MM,
    ROTATION_STEPS_PER_DEG,
    HOMING_SPEED_SLIDE,
    HOMING_SPEED_PAN,
    MIN_SLIDER_STEP
} CameraSliderConfig_t;


typedef enum
{
    MOVE_RELATIVE = 0,
    MOVE_ABSOLUTE,
    MOVE_TO_STORED_POSITION_START,
    MOVE_TO_STORED_POSITION_END
} CameraSliderMovement_t;

#endif
