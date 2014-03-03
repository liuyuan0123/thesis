/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

#define THISFIRMWARE "ArduCopter V3.1-rc2"
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 *  ArduCopter Version 3.0
 *  Creator:        Jason Short
 *  Lead Developer: Randy Mackay
 *  Based on code and ideas from the Arducopter team: Pat Hickey, Jose Julio, Jani Hirvinen, Andrew Tridgell, Justin Beech, Adam Rivera, Jean-Louis Naudin, Roberto Navoni
 *  Thanks to:	Chris Anderson, Mike Smith, Jordi Munoz, Doug Weibel, James Goppert, Benjamin Pelletier, Robert Lefebvre, Marco Robustini
 *
 *  Special Thanks for Contributors (in alphabetical order by first name):
 *
 *  Adam M Rivera		:Auto Compass Declination
 *  Amilcar Lucas		:Camera mount library
 *  Andrew Tridgell		:General development, Mavlink Support
 *  Angel Fernandez		:Alpha testing
 *  Doug Weibel			:Libraries
 *  Christof Schmid		:Alpha testing
 *  Dani Saez           :V Octo Support
 *  Gregory Fletcher	:Camera mount orientation math
 *  Guntars				:Arming safety suggestion
 *  HappyKillmore		:Mavlink GCS
 *  Hein Hollander      :Octo Support
 *  Igor van Airde      :Control Law optimization
 *  Leonard Hall 		:Flight Dynamics, Throttle, Loiter and Navigation Controllers
 *  Jonathan Challinger :Inertial Navigation
 *  Jean-Louis Naudin   :Auto Landing
 *  Max Levine			:Tri Support, Graphics
 *  Jack Dunkle			:Alpha testing
 *  James Goppert		:Mavlink Support
 *  Jani Hiriven		:Testing feedback
 *  John Arne Birkeland	:PPM Encoder
 *  Jose Julio			:Stabilization Control laws
 *  Marco Robustini		:Lead tester
 *  Michael Oborne		:Mission Planner GCS
 *  Mike Smith			:Libraries, Coding support
 *  Oliver				:Piezo support
 *  Olivier Adler       :PPM Encoder
 *  Robert Lefebvre		:Heli Support & LEDs
 *  Sandro Benigno      :Camera support
 *
 *  And much more so PLEASE PM me on DIYDRONES to add your contribution to the List
 *
 *  Requires modified "mrelax" version of Arduino, which can be found here:
 *  http://code.google.com/p/ardupilot-mega/downloads/list
 *
 */

////////////////////////////////////////////////////////////////////////////////
// Header includes
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdarg.h>

// Common dependencies
#include <AP_Common.h>
#include <AP_Progmem.h>
#include <AP_Menu.h>
#include <AP_Param.h>
// AP_HAL
#include <AP_HAL.h>
#include <AP_HAL_AVR.h>
#include <AP_HAL_AVR_SITL.h>
#include <AP_HAL_SMACCM.h>
#include <AP_HAL_PX4.h>
#include <AP_HAL_FLYMAPLE.h>
#include <AP_HAL_Empty.h>

// Application dependencies
#include <GCS_MAVLink.h>        // MAVLink GCS definitions
#include <AP_GPS.h>             // ArduPilot GPS library
#include <AP_GPS_Glitch.h>      // GPS glitch protection library
#include <DataFlash.h>          // ArduPilot Mega Flash Memory Library
#include <AP_ADC.h>             // ArduPilot Mega Analog to Digital Converter Library
#include <AP_ADC_AnalogSource.h>
#include <AP_Baro.h>
#include <AP_Compass.h>         // ArduPilot Mega Magnetometer Library
#include <AP_Math.h>            // ArduPilot Mega Vector/Matrix math Library
#include <AP_Curve.h>           // Curve used to linearlise throttle pwm to thrust
#include <AP_InertialSensor.h>  // ArduPilot Mega Inertial Sensor (accel & gyro) Library
#include <AP_AHRS.h>
#include <APM_PI.h>             // PI library
#include <AC_PID.h>             // PID library
#include <RC_Channel.h>         // RC Channel Library
#include <AP_Motors.h>          // AP Motors library
#include <AP_RangeFinder.h>     // Range finder library
#include <AP_OpticalFlow.h>     // Optical Flow library
#include <Filter.h>             // Filter library
#include <AP_Buffer.h>          // APM FIFO Buffer
#include <AP_Relay.h>           // APM relay
#include <AP_Camera.h>          // Photo or video camera
#include <AP_Mount.h>           // Camera/Antenna mount
#include <AP_Airspeed.h>        // needed for AHRS build
#include <AP_Vehicle.h>         // needed for AHRS build
#include <AP_InertialNav.h>     // ArduPilot Mega inertial navigation library
#include <AC_WPNav.h>     		// ArduCopter waypoint navigation library
#include <AP_Declination.h>     // ArduPilot Mega Declination Helper Library
#include <AC_Fence.h>           // Arducopter Fence library
#include <memcheck.h>           // memory limit checker
#include <SITL.h>               // software in the loop support
#include <AP_Scheduler.h>       // main loop scheduler
#include <AP_RCMapper.h>        // RC input mapping library
#include <AP_Notify.h>          // Notify library
#if SPRAYER == ENABLED
#include <AC_Sprayer.h>         // crop sprayer library
#endif

// AP_HAL to Arduino compatibility layer
#include "compat.h"
// Configuration
#include "defines.h"
#include "config.h"
#include "config_channels.h"

// Local modules
#include "Parameters.h"
#include "GCS.h"

////////////////////////////////////////////////////////////////////////////////
// cliSerial
////////////////////////////////////////////////////////////////////////////////
// cliSerial isn't strictly necessary - it is an alias for hal.console. It may
// be deprecated in favor of hal.console in later releases.
static AP_HAL::BetterStream* cliSerial;

// N.B. we need to keep a static declaration which isn't guarded by macros
// at the top to cooperate with the prototype mangler. 

////////////////////////////////////////////////////////////////////////////////
// AP_HAL instance
////////////////////////////////////////////////////////////////////////////////

const AP_HAL::HAL& hal = AP_HAL_BOARD_DRIVER;

////////////////////////////////////////////////////////////////////////////////
// Parameters
////////////////////////////////////////////////////////////////////////////////
//
// Global parameters are all contained within the 'g' class.
//
static Parameters g;

// main loop scheduler
static AP_Scheduler scheduler;

// AP_Notify instance
static AP_Notify notify;



////////////////////////////////////////////////////////////////////////////////
// prototypes
////////////////////////////////////////////////////////////////////////////////
static void update_events(void);
static void print_flight_mode(AP_HAL::BetterStream *port, uint8_t mode);

////////////////////////////////////////////////////////////////////////////////
// Dataflash
////////////////////////////////////////////////////////////////////////////////
#if CONFIG_HAL_BOARD == HAL_BOARD_APM2
static DataFlash_APM2 DataFlash;
#elif CONFIG_HAL_BOARD == HAL_BOARD_APM1
static DataFlash_APM1 DataFlash;
#elif CONFIG_HAL_BOARD == HAL_BOARD_AVR_SITL
//static DataFlash_File DataFlash("/tmp/APMlogs");
static DataFlash_SITL DataFlash;
#elif CONFIG_HAL_BOARD == HAL_BOARD_PX4
static DataFlash_File DataFlash("/fs/microsd/APM/logs");
#else
static DataFlash_Empty DataFlash;
#endif


////////////////////////////////////////////////////////////////////////////////
// the rate we run the main loop at
////////////////////////////////////////////////////////////////////////////////
static const AP_InertialSensor::Sample_rate ins_sample_rate = AP_InertialSensor::RATE_100HZ;

////////////////////////////////////////////////////////////////////////////////
// Sensors
////////////////////////////////////////////////////////////////////////////////
//
// There are three basic options related to flight sensor selection.
//
// - Normal flight mode. Real sensors are used.
// - HIL Attitude mode. Most sensors are disabled, as the HIL
//   protocol supplies attitude information directly.
// - HIL Sensors mode. Synthetic sensors are configured that
//   supply data from the simulation.
//

// All GPS access should be through this pointer.
static GPS         *g_gps;
static GPS_Glitch   gps_glitch(g_gps);

// flight modes convenience array
static AP_Int8 *flight_modes = &g.flight_mode1;

#if HIL_MODE == HIL_MODE_DISABLED

 #if CONFIG_ADC == ENABLED
static AP_ADC_ADS7844 adc;
 #endif

 #if CONFIG_IMU_TYPE == CONFIG_IMU_MPU6000
static AP_InertialSensor_MPU6000 ins;
#elif CONFIG_IMU_TYPE == CONFIG_IMU_OILPAN
static AP_InertialSensor_Oilpan ins(&adc);
#elif CONFIG_IMU_TYPE == CONFIG_IMU_SITL
static AP_InertialSensor_Stub ins;
#elif CONFIG_IMU_TYPE == CONFIG_IMU_PX4
static AP_InertialSensor_PX4 ins;
#elif CONFIG_IMU_TYPE == CONFIG_IMU_FLYMAPLE
AP_InertialSensor_Flymaple ins;
#endif

 #if CONFIG_HAL_BOARD == HAL_BOARD_AVR_SITL
 // When building for SITL we use the HIL barometer and compass drivers
static AP_Baro_HIL barometer;
static AP_Compass_HIL compass;
static SITL sitl;
 #else
// Otherwise, instantiate a real barometer and compass driver
  #if CONFIG_BARO == AP_BARO_BMP085
static AP_Baro_BMP085 barometer;
  #elif CONFIG_BARO == AP_BARO_PX4
static AP_Baro_PX4 barometer;
  #elif CONFIG_BARO == AP_BARO_MS5611
   #if CONFIG_MS5611_SERIAL == AP_BARO_MS5611_SPI
static AP_Baro_MS5611 barometer(&AP_Baro_MS5611::spi);
   #elif CONFIG_MS5611_SERIAL == AP_BARO_MS5611_I2C
static AP_Baro_MS5611 barometer(&AP_Baro_MS5611::i2c);
   #else
    #error Unrecognized CONFIG_MS5611_SERIAL setting.
   #endif
  #endif

 #if CONFIG_HAL_BOARD == HAL_BOARD_PX4
static AP_Compass_PX4 compass;
 #else
static AP_Compass_HMC5843 compass;
 #endif
 #endif

// real GPS selection
 #if   GPS_PROTOCOL == GPS_PROTOCOL_AUTO
AP_GPS_Auto     g_gps_driver(&g_gps);

 #elif GPS_PROTOCOL == GPS_PROTOCOL_NMEA
AP_GPS_NMEA     g_gps_driver;

 #elif GPS_PROTOCOL == GPS_PROTOCOL_SIRF
AP_GPS_SIRF     g_gps_driver;

 #elif GPS_PROTOCOL == GPS_PROTOCOL_UBLOX
AP_GPS_UBLOX    g_gps_driver;

 #elif GPS_PROTOCOL == GPS_PROTOCOL_MTK
AP_GPS_MTK      g_gps_driver;

 #elif GPS_PROTOCOL == GPS_PROTOCOL_MTK19
AP_GPS_MTK19    g_gps_driver;

 #elif GPS_PROTOCOL == GPS_PROTOCOL_NONE
AP_GPS_None     g_gps_driver;

 #else
  #error Unrecognised GPS_PROTOCOL setting.
 #endif // GPS PROTOCOL

 #if DMP_ENABLED == ENABLED && CONFIG_HAL_BOARD == HAL_BOARD_APM2
static AP_AHRS_MPU6000  ahrs(&ins, g_gps);               // only works with APM2
 #else
static AP_AHRS_DCM ahrs(&ins, g_gps);
 #endif

// ahrs2 object is the secondary ahrs to allow running DMP in parallel with DCM
  #if SECONDARY_DMP_ENABLED == ENABLED && CONFIG_HAL_BOARD == HAL_BOARD_APM2
static AP_AHRS_MPU6000  ahrs2(&ins, g_gps);               // only works with APM2
  #endif

#elif HIL_MODE == HIL_MODE_SENSORS
// sensor emulators
static AP_ADC_HIL              adc;
static AP_Baro_HIL      barometer;
static AP_Compass_HIL          compass;
static AP_GPS_HIL              g_gps_driver;
static AP_InertialSensor_Stub  ins;
static AP_AHRS_DCM             ahrs(&ins, g_gps);

static int32_t gps_base_alt;

 #if CONFIG_HAL_BOARD == HAL_BOARD_AVR_SITL
 // When building for SITL we use the HIL barometer and compass drivers
static SITL sitl;
#endif

#elif HIL_MODE == HIL_MODE_ATTITUDE
static AP_ADC_HIL              adc;
static AP_InertialSensor_Stub  ins;
static AP_AHRS_HIL             ahrs(&ins, g_gps);
static AP_GPS_HIL              g_gps_driver;
static AP_Compass_HIL          compass;                  // never used
static AP_Baro_HIL      barometer;

static int32_t gps_base_alt;

#if CONFIG_HAL_BOARD == HAL_BOARD_AVR_SITL
 // When building for SITL we use the HIL barometer and compass drivers
static SITL sitl;
#endif

#else
 #error Unrecognised HIL_MODE setting.
#endif // HIL MODE

////////////////////////////////////////////////////////////////////////////////
// Optical flow sensor
////////////////////////////////////////////////////////////////////////////////
 #if OPTFLOW == ENABLED
static AP_OpticalFlow_ADNS3080 optflow;
 #else
static AP_OpticalFlow optflow;
 #endif

////////////////////////////////////////////////////////////////////////////////
// GCS selection
////////////////////////////////////////////////////////////////////////////////
static GCS_MAVLINK gcs0;
static GCS_MAVLINK gcs3;

////////////////////////////////////////////////////////////////////////////////
// SONAR selection
////////////////////////////////////////////////////////////////////////////////
//
ModeFilterInt16_Size3 sonar_mode_filter(1);
#if CONFIG_SONAR == ENABLED
static AP_RangeFinder_MaxsonarXL *sonar;
#endif

////////////////////////////////////////////////////////////////////////////////
// User variables
////////////////////////////////////////////////////////////////////////////////
#ifdef USERHOOK_VARIABLES
 #include USERHOOK_VARIABLES
#endif


#include <quaternion_diogo.h>
#include "saturating_controller.h"

////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////

/* Radio values
 *               Channel assignments
 *                       1	Ailerons (rudder if no ailerons)
 *                       2	Elevator
 *                       3	Throttle
 *                       4	Rudder (if we have ailerons)
 *                       5	Mode - 3 position switch
 *                       6  User assignable
 *                       7	trainer switch - sets throttle nominal (toggle switch), sets accels to Level (hold > 1 second)
 *                       8	TBD
 *               Each Aux channel can be configured to have any of the available auxiliary functions assigned to it.
 *               See libraries/RC_Channel/RC_Channel_aux.h for more information
 */

//Documentation of GLobals:
static union {
    struct {
        uint8_t home_is_set         : 1; // 0
        uint8_t simple_mode         : 2; // 1,2  // This is the state of simple mode : 0 = disabled ; 1 = SIMPLE ; 2 = SUPERSIMPLE

        uint8_t pre_arm_rc_check    : 1; // 5    // true if rc input pre-arm checks have been completed successfully
        uint8_t pre_arm_check       : 1; // 6    // true if all pre-arm checks (rc, accel calibration, gps lock) have been performed
        uint8_t auto_armed          : 1; // 7    // stops auto missions from beginning until throttle is raised
        uint8_t logging_started     : 1; // 8    // true if dataflash logging has started

        uint8_t low_battery         : 1; // 9    // Used to track if the battery is low - LED output flashes when the batt is low
        uint8_t failsafe_radio      : 1; // 10   // A status flag for the radio failsafe
        uint8_t failsafe_batt       : 1; // 11   // A status flag for the battery failsafe
        uint8_t failsafe_gps        : 1; // 12   // A status flag for the gps failsafe
        uint8_t failsafe_gcs        : 1; // 13   // A status flag for the ground station failsafe
        uint8_t rc_override_active  : 1; // 14   // true if rc control are overwritten by ground station
        uint8_t do_flip             : 1; // 15   // Used to enable flip code
        uint8_t takeoff_complete    : 1; // 16
        uint8_t land_complete       : 1; // 17   // true if we have detected a landing
        uint8_t compass_status      : 1; // 18
        uint8_t gps_status          : 1; // 19
    };
    uint32_t value;
} ap;


static struct AP_System{
    uint8_t GPS_light               : 1; // 0   // Solid indicates we have full 3D lock and can navigate, flash = read
    uint8_t arming_light            : 1; // 1   // Solid indicates armed state, flashing is disarmed, double flashing is disarmed and failing pre-arm checks
    uint8_t new_radio_frame         : 1; // 2   // Set true if we have new PWM data to act on from the Radio
    uint8_t CH7_flag                : 2; // 3,4 // ch7 aux switch : 0 is low or false, 1 is center or true, 2 is high
    uint8_t CH8_flag                : 2; // 5,6 // ch8 aux switch : 0 is low or false, 1 is center or true, 2 is high
    uint8_t usb_connected           : 1; // 7   // true if APM is powered from USB connection
    uint8_t yaw_stopped             : 1; // 8   // Used to manage the Yaw hold capabilities
    uint8_t                         : 7; // 9-15 // Fill bit field to 16 bits

} ap_system;

////////////////////////////////////////////////////////////////////////////////
// Radio
////////////////////////////////////////////////////////////////////////////////
// This is the state of the flight control system
// There are multiple states defined such as STABILIZE, ACRO,
static int8_t control_mode = STABILIZE;
// Used to maintain the state of the previous control switch position
// This is set to -1 when we need to re-read the switch
static uint8_t oldSwitchPosition;
static RCMapper rcmap;

// receiver RSSI
static uint8_t receiver_rssi;


////////////////////////////////////////////////////////////////////////////////
// Motor Output
////////////////////////////////////////////////////////////////////////////////
#if FRAME_CONFIG == QUAD_FRAME
 #define MOTOR_CLASS AP_MotorsQuad
#endif
#if FRAME_CONFIG == TRI_FRAME
 #define MOTOR_CLASS AP_MotorsTri
#endif
#if FRAME_CONFIG == HEXA_FRAME
 #define MOTOR_CLASS AP_MotorsHexa
#endif
#if FRAME_CONFIG == Y6_FRAME
 #define MOTOR_CLASS AP_MotorsY6
#endif
#if FRAME_CONFIG == OCTA_FRAME
 #define MOTOR_CLASS AP_MotorsOcta
#endif
#if FRAME_CONFIG == OCTA_QUAD_FRAME
 #define MOTOR_CLASS AP_MotorsOctaQuad
#endif
#if FRAME_CONFIG == HELI_FRAME
 #define MOTOR_CLASS AP_MotorsHeli
#endif

#if FRAME_CONFIG == HELI_FRAME  // helicopter constructor requires more arguments
static MOTOR_CLASS motors(&g.rc_1, &g.rc_2, &g.rc_3, &g.rc_4, &g.rc_8, &g.heli_servo_1, &g.heli_servo_2, &g.heli_servo_3, &g.heli_servo_4);
#elif FRAME_CONFIG == TRI_FRAME  // tri constructor requires additional rc_7 argument to allow tail servo reversing
static MOTOR_CLASS motors(&g.rc_1, &g.rc_2, &g.rc_3, &g.rc_4, &g.rc_7);
#else
static MOTOR_CLASS motors(&g.rc_1, &g.rc_2, &g.rc_3, &g.rc_4);
#endif

////////////////////////////////////////////////////////////////////////////////
// PIDs
////////////////////////////////////////////////////////////////////////////////
// This is a convienience accessor for the IMU roll rates. It's currently the raw IMU rates
// and not the adjusted omega rates, but the name is stuck
static Vector3f omega;
// This is used to hold radio tuning values for in-flight CH6 tuning
float tuning_value;
// used to limit the rate that the pid controller output is logged so that it doesn't negatively affect performance
static uint8_t pid_log_counter;

////////////////////////////////////////////////////////////////////////////////
// LED output
////////////////////////////////////////////////////////////////////////////////
// Blinking indicates GPS status
static uint8_t copter_leds_GPS_blink;
// Blinking indicates battery status
static uint8_t copter_leds_motor_blink;
// Navigation confirmation blinks
static int8_t copter_leds_nav_blink;

////////////////////////////////////////////////////////////////////////////////
// GPS variables
////////////////////////////////////////////////////////////////////////////////
// This is used to scale GPS values for EEPROM storage
// 10^7 times Decimal GPS means 1 == 1cm
// This approximation makes calculations integer and it's easy to read
static const float t7 = 10000000.0;
// We use atan2 and other trig techniques to calaculate angles
// We need to scale the longitude up to make these calcs work
// to account for decreasing distance between lines of longitude away from the equator
static float scaleLongUp = 1;
// Sometimes we need to remove the scaling for distance calcs
static float scaleLongDown = 1;

////////////////////////////////////////////////////////////////////////////////
// Location & Navigation
////////////////////////////////////////////////////////////////////////////////
// This is the angle from the copter to the next waypoint in centi-degrees
static int32_t wp_bearing;
// The original bearing to the next waypoint.  used to point the nose of the copter at the next waypoint
static int32_t original_wp_bearing;
// The location of home in relation to the copter in centi-degrees
static int32_t home_bearing;
// distance between plane and home in cm
static int32_t home_distance;
// distance between plane and next waypoint in cm.
static uint32_t wp_distance;
// navigation mode - options include NAV_NONE, NAV_LOITER, NAV_CIRCLE, NAV_WP
static uint8_t nav_mode;
// Register containing the index of the current navigation command in the mission script
static int16_t command_nav_index;
// Register containing the index of the previous navigation command in the mission script
// Used to manage the execution of conditional commands
static uint8_t prev_nav_index;
// Register containing the index of the current conditional command in the mission script
static uint8_t command_cond_index;
// Used to track the required WP navigation information
// options include
// NAV_ALTITUDE - have we reached the desired altitude?
// NAV_LOCATION - have we reached the desired location?
// NAV_DELAY    - have we waited at the waypoint the desired time?
static float lon_error, lat_error;      // Used to report how many cm we are from the next waypoint or loiter target position
static int16_t control_roll;
static int16_t control_pitch;
static uint8_t rtl_state;               // records state of rtl (initial climb, returning home, etc)
static uint8_t land_state;              // records state of land (flying to location, descending)

////////////////////////////////////////////////////////////////////////////////
// Orientation
////////////////////////////////////////////////////////////////////////////////
// Convienience accessors for commonly used trig functions. These values are generated
// by the DCM through a few simple equations. They are used throughout the code where cos and sin
// would normally be used.
// The cos values are defaulted to 1 to get a decent initial value for a level state
static float cos_roll_x         = 1.0;
static float cos_pitch_x        = 1.0;
static float cos_yaw            = 1.0;
static float sin_yaw;
static float sin_roll;
static float sin_pitch;

////////////////////////////////////////////////////////////////////////////////
// SIMPLE Mode
////////////////////////////////////////////////////////////////////////////////
// Used to track the orientation of the copter for Simple mode. This value is reset at each arming
// or in SuperSimple mode when the copter leaves a 20m radius from home.
static int32_t initial_simple_bearing;

// Stores initial bearing when armed - initial simple bearing is modified in super simple mode so not suitable
static int32_t initial_armed_bearing;


////////////////////////////////////////////////////////////////////////////////
// Rate contoller targets
////////////////////////////////////////////////////////////////////////////////
static uint8_t rate_targets_frame = EARTH_FRAME;    // indicates whether rate targets provided in earth or body frame
static int32_t roll_rate_target_ef;
static int32_t pitch_rate_target_ef;
static int32_t yaw_rate_target_ef;
static int32_t roll_rate_target_bf;     // body frame roll rate target
static int32_t pitch_rate_target_bf;    // body frame pitch rate target
static int32_t yaw_rate_target_bf;      // body frame yaw rate target

////////////////////////////////////////////////////////////////////////////////
// Throttle variables
////////////////////////////////////////////////////////////////////////////////
static int16_t throttle_accel_target_ef;    // earth frame throttle acceleration target
static bool throttle_accel_controller_active;   // true when accel based throttle controller is active, false when higher level throttle controllers are providing throttle output directly
static float throttle_avg;                  // g.throttle_cruise as a float
static int16_t desired_climb_rate;          // pilot desired climb rate - for logging purposes only
static float target_alt_for_reporting;      // target altitude in cm for reporting (logs and ground station)


////////////////////////////////////////////////////////////////////////////////
// ACRO Mode
////////////////////////////////////////////////////////////////////////////////
// Used to control Axis lock
static int32_t acro_roll;                   // desired roll angle while sport mode
static int32_t acro_roll_rate;              // desired roll rate while in acro mode
static int32_t acro_pitch;                  // desired pitch angle while sport mode
static int32_t acro_pitch_rate;             // desired pitch rate while acro mode
static int32_t acro_yaw_rate;               // desired yaw rate while acro mode
static float acro_level_mix;                // scales back roll, pitch and yaw inversely proportional to input from pilot

// Filters
#if FRAME_CONFIG == HELI_FRAME
//static LowPassFilterFloat rate_roll_filter;    // Rate Roll filter
//static LowPassFilterFloat rate_pitch_filter;   // Rate Pitch filter
#endif // HELI_FRAME

////////////////////////////////////////////////////////////////////////////////
// Circle Mode / Loiter control
////////////////////////////////////////////////////////////////////////////////
Vector3f circle_center;     // circle position expressed in cm from home location.  x = lat, y = lon
// angle from the circle center to the copter's desired location.  Incremented at circle_rate / second
static float circle_angle;
// the total angle (in radians) travelled
static float circle_angle_total;
// deg : how many times to circle as specified by mission command
static uint8_t circle_desired_rotations;
static float circle_angular_acceleration;       // circle mode's angular acceleration
static float circle_angular_velocity;           // circle mode's angular velocity
static float circle_angular_velocity_max;       // circle mode's max angular velocity
// How long we should stay in Loiter Mode for mission scripting (time in seconds)
static uint16_t loiter_time_max;
// How long have we been loitering - The start time in millis
static uint32_t loiter_time;


////////////////////////////////////////////////////////////////////////////////
// CH7 and CH8 save waypoint control
////////////////////////////////////////////////////////////////////////////////
// This register tracks the current Mission Command index when writing
// a mission using Ch7 or Ch8 aux switches in flight
static int8_t aux_switch_wp_index;


////////////////////////////////////////////////////////////////////////////////
// Battery Sensors
////////////////////////////////////////////////////////////////////////////////
// Battery Voltage of battery, initialized above threshold for filter
static float battery_voltage1 = LOW_VOLTAGE * 1.05f;
// refers to the instant amp draw – based on an Attopilot Current sensor
static float current_amps1;
// refers to the total amps drawn – based on an Attopilot Current sensor
static float current_total1;


////////////////////////////////////////////////////////////////////////////////
// Altitude
////////////////////////////////////////////////////////////////////////////////
// The (throttle) controller desired altitude in cm
static float controller_desired_alt;
// The cm we are off in altitude from next_WP.alt – Positive value means we are below the WP
static int32_t altitude_error;
// The cm/s we are moving up or down based on filtered data - Positive = UP
static int16_t climb_rate;
// The altitude as reported by Sonar in cm – Values are 20 to 700 generally.
static int16_t sonar_alt;
static uint8_t sonar_alt_health;   // true if we can trust the altitude from the sonar
static float target_sonar_alt;      // desired altitude in cm above the ground
// The altitude as reported by Baro in cm – Values can be quite high
static int32_t baro_alt;

static int16_t saved_toy_throttle;


////////////////////////////////////////////////////////////////////////////////
// flight modes
////////////////////////////////////////////////////////////////////////////////
// Flight modes are combinations of Roll/Pitch, Yaw and Throttle control modes
// Each Flight mode is a unique combination of these modes
//
// The current desired control scheme for Yaw
static uint8_t yaw_mode;
// The current desired control scheme for roll and pitch / navigation
static uint8_t roll_pitch_mode;
// The current desired control scheme for altitude hold
static uint8_t throttle_mode;


////////////////////////////////////////////////////////////////////////////////
// flight specific
////////////////////////////////////////////////////////////////////////////////
// An additional throttle added to keep the copter at the same altitude when banking
static int16_t angle_boost;
// counter to verify landings
static uint16_t land_detector;


////////////////////////////////////////////////////////////////////////////////
// 3D Location vectors
////////////////////////////////////////////////////////////////////////////////
// home location is stored when we have a good GPS lock and arm the copter
// Can be reset each the copter is re-armed
static struct   Location home;
// Current location of the copter
static struct   Location current_loc;
// Holds the current loaded command from the EEPROM for navigation
static struct   Location command_nav_queue;
// Holds the current loaded command from the EEPROM for conditional scripts
static struct   Location command_cond_queue;


////////////////////////////////////////////////////////////////////////////////
// Navigation Roll/Pitch functions
////////////////////////////////////////////////////////////////////////////////
// all angles are deg * 100 : target yaw angle
// The Commanded ROll from the autopilot.
static int32_t nav_roll;
// The Commanded pitch from the autopilot. negative Pitch means go forward.
static int32_t nav_pitch;

// The Commanded ROll from the autopilot based on optical flow sensor.
static int32_t of_roll;
// The Commanded pitch from the autopilot based on optical flow sensor. negative Pitch means go forward.
static int32_t of_pitch;


////////////////////////////////////////////////////////////////////////////////
// Navigation Throttle control
////////////////////////////////////////////////////////////////////////////////
// The Commanded Throttle from the autopilot.
static int16_t nav_throttle;    // 0-1000 for throttle control
// This is a simple counter to track the amount of throttle used during flight
// This could be useful later in determining and debuging current usage and predicting battery life
static uint32_t throttle_integrator;


////////////////////////////////////////////////////////////////////////////////
// Navigation Yaw control
////////////////////////////////////////////////////////////////////////////////
// The Commanded Yaw from the autopilot.
static int32_t nav_yaw;
static uint8_t yaw_timer;
// Yaw will point at this location if yaw_mode is set to YAW_LOOK_AT_LOCATION
static Vector3f yaw_look_at_WP;
// bearing from current location to the yaw_look_at_WP
static int32_t yaw_look_at_WP_bearing;
// yaw used for YAW_LOOK_AT_HEADING yaw_mode
static int32_t yaw_look_at_heading;
// Deg/s we should turn
static int16_t yaw_look_at_heading_slew;



////////////////////////////////////////////////////////////////////////////////
// Repeat Mission Scripting Command
////////////////////////////////////////////////////////////////////////////////
// The type of repeating event - Toggle a servo channel, Toggle the APM1 relay, etc
static uint8_t event_id;
// Used to manage the timimng of repeating events
static uint32_t event_timer;
// How long to delay the next firing of event in millis
static uint16_t event_delay;
// how many times to fire : 0 = forever, 1 = do once, 2 = do twice
static int16_t event_repeat;
// per command value, such as PWM for servos
static int16_t event_value;
// the stored value used to undo commands - such as original PWM command
static int16_t event_undo_value;

////////////////////////////////////////////////////////////////////////////////
// Delay Mission Scripting Command
////////////////////////////////////////////////////////////////////////////////
static int32_t condition_value;  // used in condition commands (eg delay, change alt, etc.)
static uint32_t condition_start;


////////////////////////////////////////////////////////////////////////////////
// IMU variables
////////////////////////////////////////////////////////////////////////////////
// Integration time for the gyros (DCM algorithm)
// Updated with the fast loop
static float G_Dt = 0.02;

////////////////////////////////////////////////////////////////////////////////
// Inertial Navigation
////////////////////////////////////////////////////////////////////////////////
static AP_InertialNav inertial_nav(&ahrs, &ins, &barometer, g_gps, gps_glitch);

////////////////////////////////////////////////////////////////////////////////
// Waypoint navigation object
// To-Do: move inertial nav up or other navigation variables down here
////////////////////////////////////////////////////////////////////////////////
static AC_WPNav wp_nav(&inertial_nav, &ahrs, &g.pi_loiter_lat, &g.pi_loiter_lon, &g.pid_loiter_rate_lat, &g.pid_loiter_rate_lon);

////////////////////////////////////////////////////////////////////////////////
// Performance monitoring
////////////////////////////////////////////////////////////////////////////////
// The number of GPS fixes we have had
static uint8_t gps_fix_count;
static int16_t pmTest1;

// System Timers
// --------------
// Time in microseconds of main control loop
static uint32_t fast_loopTimer;
// Counters for branching from 10 hz control loop
static uint8_t medium_loopCounter;
// Counters for branching from 3 1/3hz control loop
static uint8_t slow_loopCounter;
// Counter of main loop executions.  Used for performance monitoring and failsafe processing
static uint16_t mainLoop_count;
// Counters for branching from 4 minute control loop used to save Compass offsets
static int16_t superslow_loopCounter;
// Loiter timer - Records how long we have been in loiter
static uint32_t rtl_loiter_start_time;
// prevents duplicate GPS messages from entering system
static uint32_t last_gps_time;
// the time when the last HEARTBEAT message arrived from a GCS - used for triggering gcs failsafe
static uint32_t last_heartbeat_ms;

// Used to exit the roll and pitch auto trim function
static uint8_t auto_trim_counter;

// Reference to the relay object (APM1 -> PORTL 2) (APM2 -> PORTB 7)
static AP_Relay relay;

//Reference to the camera object (it uses the relay object inside it)
#if CAMERA == ENABLED
  static AP_Camera camera(&relay);
#endif

// a pin for reading the receiver RSSI voltage.
static AP_HAL::AnalogSource* rssi_analog_source;


// Input sources for battery voltage, battery current, board vcc
static AP_HAL::AnalogSource* batt_volt_analog_source;
static AP_HAL::AnalogSource* batt_curr_analog_source;
static AP_HAL::AnalogSource* board_vcc_analog_source;


#if CLI_ENABLED == ENABLED
    static int8_t   setup_show (uint8_t argc, const Menu::arg *argv);
#endif

// Camera/Antenna mount tracking and stabilisation stuff
// --------------------------------------
#if MOUNT == ENABLED
// current_loc uses the baro/gps soloution for altitude rather than gps only.
// mabe one could use current_loc for lat/lon too and eliminate g_gps alltogether?
static AP_Mount camera_mount(&current_loc, g_gps, &ahrs, 0);
#endif

#if MOUNT2 == ENABLED
// current_loc uses the baro/gps soloution for altitude rather than gps only.
// mabe one could use current_loc for lat/lon too and eliminate g_gps alltogether?
static AP_Mount camera_mount2(&current_loc, g_gps, &ahrs, 1);
#endif

////////////////////////////////////////////////////////////////////////////////
// AC_Fence library to reduce fly-aways
////////////////////////////////////////////////////////////////////////////////
#if AC_FENCE == ENABLED
AC_Fence    fence(&inertial_nav);
#endif

////////////////////////////////////////////////////////////////////////////////
// Crop Sprayer
////////////////////////////////////////////////////////////////////////////////
#if SPRAYER == ENABLED
static AC_Sprayer sprayer(&inertial_nav);
#endif

////////////////////////////////////////////////////////////////////////////////
// function definitions to keep compiler from complaining about undeclared functions
////////////////////////////////////////////////////////////////////////////////
void get_throttle_althold(int32_t target_alt, int16_t min_climb_rate, int16_t max_climb_rate);
static void pre_arm_checks(bool display_failure);

////////////////////////////////////////////////////////////////////////////////
// Top-level logic
////////////////////////////////////////////////////////////////////////////////

// setup the var_info table
AP_Param param_loader(var_info, WP_START_BYTE);



/*
  if the compass is enabled then try to accumulate a reading
 */
static void compass_accumulate(void)
{
    if (g.compass_enabled) {
        compass.accumulate();
    }    
}

/*
  try to accumulate a baro reading
 */
static void barometer_accumulate(void)
{
    barometer.accumulate();
}

static void perf_update(void)
{
    if (g.log_bitmask & MASK_LOG_PM)
        Log_Write_Performance();
    if (scheduler.debug()) {
        cliSerial->printf_P(PSTR("PERF: %u/%u %lu\n"), 
                            (unsigned)perf_info_get_num_long_running(),
                            (unsigned)perf_info_get_num_loops(),
                            (unsigned long)perf_info_get_max_time());
    }
    perf_info_reset();
    gps_fix_count = 0;
    pmTest1 = 0;
}

static const AP_Scheduler::Task scheduler_tasks[] PROGMEM = {
  // no tasks
};

#define MOTOR_F   2    // Front     
#define MOTOR_B   3    // Back
#define MOTOR_L   1   // Left
#define MOTOR_R   0    // Right

#define LOGGING_ENABLED ENABLED // To allow logging

void setup() {
    // this needs to be the first call, as it fills memory with
    // sentinel values
    memcheck_init();
    cliSerial = hal.console;

    // Load the default values of variables listed in var_info[]s
    AP_Param::setup_sketch_defaults();

    // initialise notify system
    notify.init();

   
    
    // Initialise sensor
    ins.init(AP_InertialSensor::COLD_START, AP_InertialSensor::RATE_100HZ);
    ahrs.init();
    
    // Initialise motor write
    hal.rcout->set_freq(0xF, 490);
    hal.rcout->enable_mask(0xFF);
    
    
    // Disable barometer to stop it corrupting bus
    hal.gpio->pinMode(40, GPIO_OUTPUT);
    hal.gpio->write(40, 1);
    
    /*rssi_analog_source      = hal.analogin->channel(g.rssi_pin);
    batt_volt_analog_source = hal.analogin->channel(g.battery_volt_pin);
    batt_curr_analog_source = hal.analogin->channel(g.battery_curr_pin);
    board_vcc_analog_source = hal.analogin->channel(ANALOG_INPUT_BOARD_VCC);*/

    init_ardupilot();
    motors.armed(true);
    start_logging();
    update_auto_armed();
    AP_Notify::flags.armed = true;

    scheduler.init(&scheduler_tasks[0], sizeof(scheduler_tasks)/sizeof(scheduler_tasks[0]));

}

#define FREQ_OP 70

int i=0,j=0, faults = 0;
uint16_t channels[8];  // array for raw channel values 
uint16_t  motor_val[4]={1000,1000,1000,1000}; // array with motor inputs: [F,B,L,R]
uint32_t timer = 0;
uint16_t rcthr, rcyaw, rcpit, rcroll;   // Variables to store rc input
float roll_off = 0, pitch_off = 0, yaw_off = 0;
Quaternion_D q_1,q_2,q_3,q_4,q_5;
Vector3<float> tau;

void loop()
{

  
  
  timer = micros();
  //if (ins.num_samples_available() >= 1) {
  
    // MY CODE HERE

    read_AHRS();
    if(j==0){
      roll_off = ahrs.roll;
      pitch_off = ahrs.pitch;
      yaw_off = ahrs.yaw;
      j=1;
    }
    
    // RADIO CODE
    hal.rcin->read(channels, 8);
    
    // Copy from channels array to something human readable - array entry 0 = input 1, etc.
    
    rcthr = channels[2];  
    rcyaw = channels[3];
    rcpit = channels[1];
    rcroll = channels[0];

    q_1.from_euler(ahrs.roll,ahrs.pitch,ahrs.yaw-yaw_off);//ahrs.roll,ahrs.pitch,ahrs.yaw); // current attitude
    //DEBUG
    /*q_1.q1=-0.2710171;
    q_1.q2=-0.9086057;
    q_1.q3=0.3172428;
    q_1.q4=0.0185002;*/
    q_2.from_euler(0,0,0); // desired attitude
    
    //DEBUG
    /*omega.x = 1.749156;
    omega.y = 0.9093719;
    omega.z = 5.860670;*/

    //hal.console->printf_P(PSTR("A: [%.2f,%.2f,%.2f]\r\n"),ToDeg(ahrs.roll),ToDeg(ahrs.pitch),ToDeg(ahrs.yaw-yaw_off));
    //hal.console->printf_P(PSTR("q: [%.7f,%.7f,%.7f,%.7f]\r\n"),q_1.q1,q_1.q2,q_1.q3,q_1.q4);
    //hal.console->printf_P(PSTR("o: [%.7f,%.7f,%.7f]\r\n"),omega.x,omega.y,omega.z);
    //hal.console->printf_P(PSTR("t: [%.7f,%.7f,%.7f]\r\n"),tau.x,tau.y,tau.z);
    // needs conversion from radio input to thrust.

    //hal.console->printf_P(PSTR("T: %.7f\r\n"),map_f(rcthr*1.0f,1032,1986,0.5,3.5));
    if(rcthr > 1033){
      tau = fast_and_saturating_controller(q_1,q_2,omega);
      to_motors(map_f(rcthr*1.0f,1032,1986,0.5,3.0), tau, &motor_val[MOTOR_F],&motor_val[MOTOR_B],&motor_val[MOTOR_L],&motor_val[MOTOR_R]);
    }else{
      tau.x = 0;
      tau.y = 0;
      tau.z = 0;
      to_motors(0, tau, &motor_val[MOTOR_F],&motor_val[MOTOR_B],&motor_val[MOTOR_L],&motor_val[MOTOR_R]);
    }

    //hal.console->printf_P(PSTR("u: [ %d %d %d %d]\r\n"),motor_val[MOTOR_F],motor_val[MOTOR_B],motor_val[MOTOR_L],motor_val[MOTOR_R] );

    //Log_Write_Motors(motor_val[MOTOR_F],motor_val[MOTOR_B],motor_val[MOTOR_R],motor_val[MOTOR_L]);
    Log_Write_torques(10000*tau.x,10000*tau.y,10000*tau.z);
    Log_Write_Attitude();
    //hal.console->printf_P(PSTR("t: [%.7f,%.7f,%.7f]\r\n"),tau.x,tau.y,tau.z);




    // END OF MY CODE


    while(micros()-timer < 1000000/FREQ_OP)
    {
      i=1;
      hal.rcout->write(MOTOR_F, motor_val[MOTOR_F]);
      hal.rcout->write(MOTOR_B, motor_val[MOTOR_B]);
      hal.rcout->write(MOTOR_R, motor_val[MOTOR_R]);
      hal.rcout->write(MOTOR_L, motor_val[MOTOR_L]);
    }
    hal.rcout->write(MOTOR_F, motor_val[MOTOR_F]);
    hal.rcout->write(MOTOR_B, motor_val[MOTOR_B]);
    hal.rcout->write(MOTOR_R, motor_val[MOTOR_R]);
    hal.rcout->write(MOTOR_L, motor_val[MOTOR_L]);
    
    if(i==0){
      faults++;
      //hal.console->printf_P(PSTR("f: %d\r\n"),faults);

    }else
      faults = 0;
      
    i=0;


 
  
}


// set_yaw_mode - update yaw mode and initialise any variables required
bool set_yaw_mode(uint8_t new_yaw_mode)
{
    // boolean to ensure proper initialisation of throttle modes
    bool yaw_initialised = false;

    // return immediately if no change
    if( new_yaw_mode == yaw_mode ) {
        return true;
    }

    switch( new_yaw_mode ) {
        case YAW_HOLD:
            yaw_initialised = true;
            break;
        case YAW_ACRO:
            yaw_initialised = true;
            acro_yaw_rate = 0;
            break;
        case YAW_LOOK_AT_NEXT_WP:
            if( ap.home_is_set ) {
                yaw_initialised = true;
            }
            break;
        case YAW_LOOK_AT_LOCATION:
            if( ap.home_is_set ) {
                // update bearing - assumes yaw_look_at_WP has been intialised before set_yaw_mode was called
                yaw_look_at_WP_bearing = pv_get_bearing_cd(inertial_nav.get_position(), yaw_look_at_WP);
                yaw_initialised = true;
            }
            break;
        case YAW_CIRCLE:
            if( ap.home_is_set ) {
                // set yaw to point to center of circle
                yaw_look_at_WP = circle_center;
                // initialise bearing to current heading
                yaw_look_at_WP_bearing = ahrs.yaw_sensor;
                yaw_initialised = true;
            }
            break;
        case YAW_LOOK_AT_HEADING:
            yaw_initialised = true;
            break;
        case YAW_LOOK_AT_HOME:
            if( ap.home_is_set ) {
                yaw_initialised = true;
            }
            break;
        case YAW_LOOK_AHEAD:
            if( ap.home_is_set ) {
                yaw_initialised = true;
            }
            break;
        case YAW_TOY:
            yaw_initialised = true;
            break;
        case YAW_RESETTOARMEDYAW:
            nav_yaw = ahrs.yaw_sensor; // store current yaw so we can start rotating back to correct one
            yaw_initialised = true;
            break;
    }

    // if initialisation has been successful update the yaw mode
    if( yaw_initialised ) {
        yaw_mode = new_yaw_mode;
    }

    // return success or failure
    return yaw_initialised;
}

// update_yaw_mode - run high level yaw controllers
// 100hz update rate
void update_yaw_mode(void)
{
    switch(yaw_mode) {

    case YAW_HOLD:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }
        // heading hold at heading held in nav_yaw but allow input from pilot
        get_yaw_rate_stabilized_ef(g.rc_4.control_in);
        break;

    case YAW_ACRO:
        // pilot controlled yaw using rate controller
        get_yaw_rate_stabilized_bf(g.rc_4.control_in);
        break;

    case YAW_LOOK_AT_NEXT_WP:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }else{
            // point towards next waypoint (no pilot input accepted)
            // we don't use wp_bearing because we don't want the copter to turn too much during flight
            nav_yaw = get_yaw_slew(nav_yaw, original_wp_bearing, AUTO_YAW_SLEW_RATE);
        }
        get_stabilize_yaw(nav_yaw);

        // if there is any pilot input, switch to YAW_HOLD mode for the next iteration
        if( g.rc_4.control_in != 0 ) {
            set_yaw_mode(YAW_HOLD);
        }
        break;

    case YAW_LOOK_AT_LOCATION:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }
        // point towards a location held in yaw_look_at_WP
        get_look_at_yaw();

        // if there is any pilot input, switch to YAW_HOLD mode for the next iteration
        if( g.rc_4.control_in != 0 ) {
            set_yaw_mode(YAW_HOLD);
        }
        break;

    case YAW_CIRCLE:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }
        // points toward the center of the circle or does a panorama
        get_circle_yaw();

        // if there is any pilot input, switch to YAW_HOLD mode for the next iteration
        if( g.rc_4.control_in != 0 ) {
            set_yaw_mode(YAW_HOLD);
        }
        break;

    case YAW_LOOK_AT_HOME:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }else{
            // keep heading always pointing at home with no pilot input allowed
            nav_yaw = get_yaw_slew(nav_yaw, home_bearing, AUTO_YAW_SLEW_RATE);
        }
        get_stabilize_yaw(nav_yaw);

        // if there is any pilot input, switch to YAW_HOLD mode for the next iteration
        if( g.rc_4.control_in != 0 ) {
            set_yaw_mode(YAW_HOLD);
        }
        break;

    case YAW_LOOK_AT_HEADING:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }else{
            // keep heading pointing in the direction held in yaw_look_at_heading with no pilot input allowed
            nav_yaw = get_yaw_slew(nav_yaw, yaw_look_at_heading, yaw_look_at_heading_slew);
        }
        get_stabilize_yaw(nav_yaw);
        break;

	case YAW_LOOK_AHEAD:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }
		// Commanded Yaw to automatically look ahead.
        get_look_ahead_yaw(g.rc_4.control_in);
        break;

#if TOY_LOOKUP == TOY_EXTERNAL_MIXER
    case YAW_TOY:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }else{
            // update to allow external roll/yaw mixing
            // keep heading always pointing at home with no pilot input allowed
            nav_yaw = get_yaw_slew(nav_yaw, home_bearing, AUTO_YAW_SLEW_RATE);
        }
        get_stabilize_yaw(nav_yaw);
        break;
#endif

    case YAW_RESETTOARMEDYAW:
        // if we are landed reset yaw target to current heading
        if (ap.land_complete) {
            nav_yaw = ahrs.yaw_sensor;
        }else{
            // changes yaw to be same as when quad was armed
            nav_yaw = get_yaw_slew(nav_yaw, initial_armed_bearing, AUTO_YAW_SLEW_RATE);
        }
        get_stabilize_yaw(nav_yaw);

        // if there is any pilot input, switch to YAW_HOLD mode for the next iteration
        if( g.rc_4.control_in != 0 ) {
            set_yaw_mode(YAW_HOLD);
        }

        break;
    }
}

// get yaw mode based on WP_YAW_BEHAVIOR parameter
// set rtl parameter to true if this is during an RTL
uint8_t get_wp_yaw_mode(bool rtl)
{
    switch (g.wp_yaw_behavior) {
        case WP_YAW_BEHAVIOR_LOOK_AT_NEXT_WP:
            return YAW_LOOK_AT_NEXT_WP;
            break;

        case WP_YAW_BEHAVIOR_LOOK_AT_NEXT_WP_EXCEPT_RTL:
            if( rtl ) {
                return YAW_HOLD;
            }else{
                return YAW_LOOK_AT_NEXT_WP; 
            }
            break;

        case WP_YAW_BEHAVIOR_LOOK_AHEAD:
            return YAW_LOOK_AHEAD;
            break;

        default:
            return YAW_HOLD;
            break;
    }
}

// set_roll_pitch_mode - update roll/pitch mode and initialise any variables as required
bool set_roll_pitch_mode(uint8_t new_roll_pitch_mode)
{
    // boolean to ensure proper initialisation of throttle modes
    bool roll_pitch_initialised = false;

    // return immediately if no change
    if( new_roll_pitch_mode == roll_pitch_mode ) {
        return true;
    }

    switch( new_roll_pitch_mode ) {
        case ROLL_PITCH_STABLE:
            roll_pitch_initialised = true;
            break;
        case ROLL_PITCH_ACRO:
            // reset acro level rates
            acro_roll_rate = 0;
            acro_pitch_rate = 0;
            roll_pitch_initialised = true;
            break;
        case ROLL_PITCH_AUTO:
        case ROLL_PITCH_STABLE_OF:
        case ROLL_PITCH_TOY:
        case ROLL_PITCH_SPORT:
            roll_pitch_initialised = true;
            break;

        case ROLL_PITCH_LOITER:
            // require gps lock
            if( ap.home_is_set ) {
                roll_pitch_initialised = true;
            }
            break;
    }

    // if initialisation has been successful update the yaw mode
    if( roll_pitch_initialised ) {
        roll_pitch_mode = new_roll_pitch_mode;
    }

    // return success or failure
    return roll_pitch_initialised;
}

// update_roll_pitch_mode - run high level roll and pitch controllers
// 100hz update rate
void update_roll_pitch_mode(void)
{
    switch(roll_pitch_mode) {
    case ROLL_PITCH_ACRO:
        // copy user input for reporting purposes
        control_roll            = g.rc_1.control_in;
        control_pitch           = g.rc_2.control_in;

#if FRAME_CONFIG == HELI_FRAME
        // ACRO does not get SIMPLE mode ability
        if (motors.flybar_mode == 1) {
            g.rc_1.servo_out = g.rc_1.control_in;
            g.rc_2.servo_out = g.rc_2.control_in;
        }else{
            acro_level_mix = constrain_float(1-max(max(abs(g.rc_1.control_in), abs(g.rc_2.control_in)), abs(g.rc_4.control_in))/4500.0, 0, 1)*cos_pitch_x;
            get_roll_rate_stabilized_bf(g.rc_1.control_in);
            get_pitch_rate_stabilized_bf(g.rc_2.control_in);
            get_acro_level_rates();
        }
#else  // !HELI_FRAME
        acro_level_mix = constrain_float(1-max(max(abs(g.rc_1.control_in), abs(g.rc_2.control_in)), abs(g.rc_4.control_in))/4500.0, 0, 1)*cos_pitch_x;
        get_roll_rate_stabilized_bf(g.rc_1.control_in);
        get_pitch_rate_stabilized_bf(g.rc_2.control_in);
        get_acro_level_rates();
#endif  // HELI_FRAME
        break;

    case ROLL_PITCH_STABLE:
        // apply SIMPLE mode transform
        if(ap.simple_mode && ap_system.new_radio_frame) {
            update_simple_mode();
        }

        // convert pilot input to lean angles
        get_pilot_desired_lean_angles(g.rc_1.control_in, g.rc_2.control_in, control_roll, control_pitch);

        // pass desired roll, pitch to stabilize attitude controllers
        get_stabilize_roll(control_roll);
        get_stabilize_pitch(control_pitch);

        break;

    case ROLL_PITCH_AUTO:
        // copy latest output from nav controller to stabilize controller
        nav_roll = wp_nav.get_desired_roll();
        nav_pitch = wp_nav.get_desired_pitch();
        get_stabilize_roll(nav_roll);
        get_stabilize_pitch(nav_pitch);

        // user input, although ignored is put into control_roll and pitch for reporting purposes
        control_roll = g.rc_1.control_in;
        control_pitch = g.rc_2.control_in;
        break;

    case ROLL_PITCH_STABLE_OF:
        // apply SIMPLE mode transform
        if(ap.simple_mode && ap_system.new_radio_frame) {
            update_simple_mode();
        }

        // convert pilot input to lean angles
        get_pilot_desired_lean_angles(g.rc_1.control_in, g.rc_2.control_in, control_roll, control_pitch);

        // mix in user control with optical flow
        get_stabilize_roll(get_of_roll(control_roll));
        get_stabilize_pitch(get_of_pitch(control_pitch));
        break;

    // THOR
    // a call out to the main toy logic
    case ROLL_PITCH_TOY:
        roll_pitch_toy();
        break;

    case ROLL_PITCH_LOITER:
        // apply SIMPLE mode transform
        if(ap.simple_mode && ap_system.new_radio_frame) {
            update_simple_mode();
        }
        // copy user input for reporting purposes
        control_roll            = g.rc_1.control_in;
        control_pitch           = g.rc_2.control_in;

        // update loiter target from user controls
        wp_nav.move_loiter_target(control_roll, control_pitch,0.01f);

        // copy latest output from nav controller to stabilize controller
        nav_roll = wp_nav.get_desired_roll();
        nav_pitch = wp_nav.get_desired_pitch();

        get_stabilize_roll(nav_roll);
        get_stabilize_pitch(nav_pitch);
        break;

    case ROLL_PITCH_SPORT:
        // apply SIMPLE mode transform
        if(ap.simple_mode && ap_system.new_radio_frame) {
            update_simple_mode();
        }
        // copy user input for reporting purposes
        control_roll = g.rc_1.control_in;
        control_pitch = g.rc_2.control_in;
        get_roll_rate_stabilized_ef(g.rc_1.control_in);
        get_pitch_rate_stabilized_ef(g.rc_2.control_in);
        break;
    }

	#if FRAME_CONFIG != HELI_FRAME
    if(g.rc_3.control_in == 0 && control_mode <= ACRO) {
        reset_rate_I();
    }
	#endif //HELI_FRAME

    if(ap_system.new_radio_frame) {
        // clear new radio frame info
        ap_system.new_radio_frame = false;
    }
}

// new radio frame is used to make sure we only call this at 50hz
void update_simple_mode(void)
{
    static uint8_t simple_counter = 0;             // State machine counter for Simple Mode
    static float simple_sin_y=0, simple_cos_x=0;

    // used to manage state machine
    // which improves speed of function
    simple_counter++;

    int16_t delta = wrap_360_cd(ahrs.yaw_sensor - initial_simple_bearing)/100;

    if (simple_counter == 1) {
        // roll
        simple_cos_x = sinf(radians(90 - delta));

    }else if (simple_counter > 2) {
        // pitch
        simple_sin_y = cosf(radians(90 - delta));
        simple_counter = 0;
    }

    // Rotate input by the initial bearing
    int16_t _roll   = g.rc_1.control_in * simple_cos_x + g.rc_2.control_in * simple_sin_y;
    int16_t _pitch  = -(g.rc_1.control_in * simple_sin_y - g.rc_2.control_in * simple_cos_x);

    g.rc_1.control_in = _roll;
    g.rc_2.control_in = _pitch;
}

// update_super_simple_bearing - adjusts simple bearing based on location
// should be called after home_bearing has been updated
void update_super_simple_bearing()
{
    // are we in SUPERSIMPLE mode?
    if(ap.simple_mode == 2 || (ap.simple_mode && g.super_simple)) {
        // get distance to home
        if(home_distance > SUPER_SIMPLE_RADIUS) {        // 10m from home
            // we reset the angular offset to be a vector from home to the quad
            initial_simple_bearing = wrap_360_cd(home_bearing+18000);
        }
    }
}

// set_throttle_mode - sets the throttle mode and initialises any variables as required
bool set_throttle_mode( uint8_t new_throttle_mode )
{
    // boolean to ensure proper initialisation of throttle modes
    bool throttle_initialised = false;

    // return immediately if no change
    if( new_throttle_mode == throttle_mode ) {
        return true;
    }

    // initialise any variables required for the new throttle mode
    switch(new_throttle_mode) {
        case THROTTLE_MANUAL:
        case THROTTLE_MANUAL_TILT_COMPENSATED:
            throttle_accel_deactivate();                // this controller does not use accel based throttle controller
            altitude_error = 0;                         // clear altitude error reported to GCS
            throttle_initialised = true;
            break;

        case THROTTLE_HOLD:
        case THROTTLE_AUTO:
            controller_desired_alt = get_initial_alt_hold(current_loc.alt, climb_rate);     // reset controller desired altitude to current altitude
            wp_nav.set_desired_alt(controller_desired_alt);                                 // same as above but for loiter controller
            if ( throttle_mode <= THROTTLE_MANUAL_TILT_COMPENSATED ) {      // reset the alt hold I terms if previous throttle mode was manual
                reset_throttle_I();
                set_accel_throttle_I_from_pilot_throttle(get_pilot_desired_throttle(g.rc_3.control_in));
            }
            throttle_initialised = true;
            break;

        case THROTTLE_LAND:
            reset_land_detector();  // initialise land detector
            controller_desired_alt = get_initial_alt_hold(current_loc.alt, climb_rate);   // reset controller desired altitude to current altitude
            throttle_initialised = true;
            break;
    }

    // update the throttle mode
    if( throttle_initialised ) {
        throttle_mode = new_throttle_mode;

        // reset some variables used for logging
        desired_climb_rate = 0;
        nav_throttle = 0;
    }

    // return success or failure
    return throttle_initialised;
}

// update_throttle_mode - run high level throttle controllers
// 50 hz update rate
void update_throttle_mode(void)
{
    int16_t pilot_climb_rate;
    int16_t pilot_throttle_scaled;

    if(ap.do_flip)     // this is pretty bad but needed to flip in AP modes.
        return;

#if FRAME_CONFIG == HELI_FRAME

	if (control_mode == STABILIZE){
		motors.stab_throttle = true;
	} else {
		motors.stab_throttle = false;
	}
    
    // allow swash collective to move if we are in manual throttle modes, even if disarmed
    if( !motors.armed() ) {
        if ( !(throttle_mode == THROTTLE_MANUAL) && !(throttle_mode == THROTTLE_MANUAL_TILT_COMPENSATED)){
            throttle_accel_deactivate();    // do not allow the accel based throttle to override our command
            return;
        }
    }

#else // HELI_FRAME

// do not run throttle controllers if motors disarmed
    if( !motors.armed() ) {
        set_throttle_out(0, false);
        throttle_accel_deactivate();    // do not allow the accel based throttle to override our command
        set_target_alt_for_reporting(0);
        return;
    }
    
#endif // HELI_FRAME

    switch(throttle_mode) {

    case THROTTLE_MANUAL:
        // completely manual throttle
        if(g.rc_3.control_in <= 0){
            set_throttle_out(0, false);
        }else{
            // send pilot's output directly to motors
            pilot_throttle_scaled = get_pilot_desired_throttle(g.rc_3.control_in);
            set_throttle_out(pilot_throttle_scaled, false);

            // update estimate of throttle cruise
			#if FRAME_CONFIG == HELI_FRAME
            update_throttle_cruise(motors.coll_out);
			#else
			update_throttle_cruise(pilot_throttle_scaled);
			#endif  //HELI_FRAME


            // check if we've taken off yet
            if (!ap.takeoff_complete && motors.armed()) {
                if (pilot_throttle_scaled > g.throttle_cruise) {
                    // we must be in the air by now
                    set_takeoff_complete(true);
                }
            }
        }
        set_target_alt_for_reporting(0);
        break;

    case THROTTLE_MANUAL_TILT_COMPENSATED:
        // manual throttle but with angle boost
        if (g.rc_3.control_in <= 0) {
            set_throttle_out(0, false); // no need for angle boost with zero throttle
        }else{
            pilot_throttle_scaled = get_pilot_desired_throttle(g.rc_3.control_in);
            set_throttle_out(pilot_throttle_scaled, true);

            // update estimate of throttle cruise
            #if FRAME_CONFIG == HELI_FRAME
            update_throttle_cruise(motors.coll_out);
			#else
			update_throttle_cruise(pilot_throttle_scaled);
			#endif  //HELI_FRAME

            if (!ap.takeoff_complete && motors.armed()) {
                if (pilot_throttle_scaled > g.throttle_cruise) {
                    // we must be in the air by now
                    set_takeoff_complete(true);
                }
            }
        }
        set_target_alt_for_reporting(0);
        break;

    case THROTTLE_HOLD:
        if(ap.auto_armed) {
            // alt hold plus pilot input of climb rate
            pilot_climb_rate = get_pilot_desired_climb_rate(g.rc_3.control_in);

            // special handling if we have landed
            if (ap.land_complete) {
                if (pilot_climb_rate > 0) {
                    // indicate we are taking off
                    set_land_complete(false);
                    // clear i term when we're taking off
                    set_throttle_takeoff();
                }else{
                    // move throttle to minimum to keep us on the ground
                    set_throttle_out(0, false);
                    // deactivate accel based throttle controller (it will be automatically re-enabled when alt-hold controller next runs)
                    throttle_accel_deactivate();
                }
            }
            // check land_complete flag again in case it was changed above
            if (!ap.land_complete) {
                if( sonar_alt_health >= SONAR_ALT_HEALTH_MAX ) {
                    // if sonar is ok, use surface tracking
                    get_throttle_surface_tracking(pilot_climb_rate);    // this function calls set_target_alt_for_reporting for us
                }else{
                    // if no sonar fall back stabilize rate controller
                    get_throttle_rate_stabilized(pilot_climb_rate);     // this function calls set_target_alt_for_reporting for us
                }
            }
        }else{
            // pilot's throttle must be at zero so keep motors off
            set_throttle_out(0, false);
            // deactivate accel based throttle controller
            throttle_accel_deactivate();
            set_target_alt_for_reporting(0);
        }
        break;

    case THROTTLE_AUTO:
        // auto pilot altitude controller with target altitude held in wp_nav.get_desired_alt()
        if(ap.auto_armed) {
            // special handling if we are just taking off
            if (ap.land_complete) {
                // tell motors to do a slow start.
                motors.slow_start(true);
            }
            get_throttle_althold_with_slew(wp_nav.get_desired_alt(), -wp_nav.get_descent_velocity(), wp_nav.get_climb_velocity());
            set_target_alt_for_reporting(wp_nav.get_desired_alt()); // To-Do: return get_destination_alt if we are flying to a waypoint
        }else{
            // pilot's throttle must be at zero so keep motors off
            set_throttle_out(0, false);
            // deactivate accel based throttle controller
            throttle_accel_deactivate();
            set_target_alt_for_reporting(0);
        }
        break;

    case THROTTLE_LAND:
        // landing throttle controller
        get_throttle_land();
        set_target_alt_for_reporting(0);
        break;
    }
}

// set_target_alt_for_reporting - set target altitude in cm for reporting purposes (logs and gcs)
static void set_target_alt_for_reporting(float alt_cm)
{
    target_alt_for_reporting = alt_cm;
}

// get_target_alt_for_reporting - returns target altitude in cm for reporting purposes (logs and gcs)
static float get_target_alt_for_reporting()
{
    return target_alt_for_reporting;
}

static void read_AHRS(void)
{


    ahrs.update();
    omega = ins.get_gyro();

}

static void update_trig(void){
    Vector2f yawvector;
    const Matrix3f &temp   = ahrs.get_dcm_matrix();

    yawvector.x     = temp.a.x;     // sin
    yawvector.y     = temp.b.x;         // cos
    yawvector.normalize();

    cos_pitch_x     = safe_sqrt(1 - (temp.c.x * temp.c.x));     // level = 1
    cos_roll_x      = temp.c.z / cos_pitch_x;                       // level = 1

    cos_pitch_x     = constrain_float(cos_pitch_x, 0, 1.0);
    // this relies on constrain_float() of infinity doing the right thing,
    // which it does do in avr-libc
    cos_roll_x      = constrain_float(cos_roll_x, -1.0, 1.0);

    sin_yaw         = constrain_float(yawvector.y, -1.0, 1.0);
    cos_yaw         = constrain_float(yawvector.x, -1.0, 1.0);

    // added to convert earth frame to body frame for rate controllers
    sin_pitch       = -temp.c.x;
    sin_roll        = temp.c.y / cos_pitch_x;

    // update wp_nav controller with trig values
    wp_nav.set_cos_sin_yaw(cos_yaw, sin_yaw, cos_pitch_x);

    //flat:
    // 0 ° = cos_yaw:  1.00, sin_yaw:  0.00,
    // 90° = cos_yaw:  0.00, sin_yaw:  1.00,
    // 180 = cos_yaw: -1.00, sin_yaw:  0.00,
    // 270 = cos_yaw:  0.00, sin_yaw: -1.00,
}

// read baro and sonar altitude at 20hz
static void update_altitude()
{
#if HIL_MODE == HIL_MODE_ATTITUDE
    // we are in the SIM, fake out the baro and Sonar
    baro_alt                = g_gps->altitude_cm - gps_base_alt;

    if(g.sonar_enabled) {
        sonar_alt           = baro_alt;
    }
#else
    // read in baro altitude
    baro_alt            = read_barometer();

    // read in sonar altitude
    sonar_alt           = read_sonar();
#endif  // HIL_MODE == HIL_MODE_ATTITUDE

    // write altitude info to dataflash logs
    if ((g.log_bitmask & MASK_LOG_CTUN) && motors.armed()) {
        Log_Write_Control_Tuning();
    }
}

static void tuning(){
    tuning_value = (float)g.rc_6.control_in / 1000.0f;
    g.rc_6.set_range(g.radio_tuning_low,g.radio_tuning_high);                   // 0 to 1

    switch(g.radio_tuning) {

    // Roll, Pitch tuning
    case CH6_STABILIZE_ROLL_PITCH_KP:
        g.pi_stabilize_roll.kP(tuning_value);
        g.pi_stabilize_pitch.kP(tuning_value);
        break;

    case CH6_RATE_ROLL_PITCH_KP:
        g.pid_rate_roll.kP(tuning_value);
        g.pid_rate_pitch.kP(tuning_value);
        break;

    case CH6_RATE_ROLL_PITCH_KI:
        g.pid_rate_roll.kI(tuning_value);
        g.pid_rate_pitch.kI(tuning_value);
        break;

    case CH6_RATE_ROLL_PITCH_KD:
        g.pid_rate_roll.kD(tuning_value);
        g.pid_rate_pitch.kD(tuning_value);
        break;

    // Yaw tuning
    case CH6_STABILIZE_YAW_KP:
        g.pi_stabilize_yaw.kP(tuning_value);
        break;

    case CH6_YAW_RATE_KP:
        g.pid_rate_yaw.kP(tuning_value);
        break;

    case CH6_YAW_RATE_KD:
        g.pid_rate_yaw.kD(tuning_value);
        break;

    // Altitude and throttle tuning
    case CH6_ALTITUDE_HOLD_KP:
        g.pi_alt_hold.kP(tuning_value);
        break;

    case CH6_THROTTLE_RATE_KP:
        g.pid_throttle_rate.kP(tuning_value);
        break;

    case CH6_THROTTLE_RATE_KD:
        g.pid_throttle_rate.kD(tuning_value);
        break;

    case CH6_THROTTLE_ACCEL_KP:
        g.pid_throttle_accel.kP(tuning_value);
        break;

    case CH6_THROTTLE_ACCEL_KI:
        g.pid_throttle_accel.kI(tuning_value);
        break;

    case CH6_THROTTLE_ACCEL_KD:
        g.pid_throttle_accel.kD(tuning_value);
        break;

    // Loiter and navigation tuning
    case CH6_LOITER_POSITION_KP:
        g.pi_loiter_lat.kP(tuning_value);
        g.pi_loiter_lon.kP(tuning_value);
        break;

    case CH6_LOITER_RATE_KP:
        g.pid_loiter_rate_lon.kP(tuning_value);
        g.pid_loiter_rate_lat.kP(tuning_value);
        break;

    case CH6_LOITER_RATE_KI:
        g.pid_loiter_rate_lon.kI(tuning_value);
        g.pid_loiter_rate_lat.kI(tuning_value);
        break;

    case CH6_LOITER_RATE_KD:
        g.pid_loiter_rate_lon.kD(tuning_value);
        g.pid_loiter_rate_lat.kD(tuning_value);
        break;

    case CH6_WP_SPEED:
        // set waypoint navigation horizontal speed to 0 ~ 1000 cm/s
        wp_nav.set_horizontal_velocity(g.rc_6.control_in);
        break;

    // Acro roll pitch gain
    case CH6_ACRO_RP_KP:
        g.acro_rp_p = tuning_value;
        break;

    // Acro yaw gain
    case CH6_ACRO_YAW_KP:
        g.acro_yaw_p = tuning_value;
        break;

    case CH6_RELAY:
        if (g.rc_6.control_in > 525) relay.on();
        if (g.rc_6.control_in < 475) relay.off();
        break;

#if FRAME_CONFIG == HELI_FRAME
    case CH6_HELI_EXTERNAL_GYRO:
        motors.ext_gyro_gain = tuning_value;
        break;
#endif

    case CH6_OPTFLOW_KP:
        g.pid_optflow_roll.kP(tuning_value);
        g.pid_optflow_pitch.kP(tuning_value);
        break;

    case CH6_OPTFLOW_KI:
        g.pid_optflow_roll.kI(tuning_value);
        g.pid_optflow_pitch.kI(tuning_value);
        break;

    case CH6_OPTFLOW_KD:
        g.pid_optflow_roll.kD(tuning_value);
        g.pid_optflow_pitch.kD(tuning_value);
        break;

#if HIL_MODE != HIL_MODE_ATTITUDE                                       // do not allow modifying _kp or _kp_yaw gains in HIL mode
    case CH6_AHRS_YAW_KP:
        ahrs._kp_yaw.set(tuning_value);
        break;

    case CH6_AHRS_KP:
        ahrs._kp.set(tuning_value);
        break;
#endif

    case CH6_INAV_TC:
        // To-Do: allowing tuning TC for xy and z separately
        inertial_nav.set_time_constant_xy(tuning_value);
        inertial_nav.set_time_constant_z(tuning_value);
        break;

    case CH6_DECLINATION:
        // set declination to +-20degrees
        compass.set_declination(ToRad((2.0f * g.rc_6.control_in - g.radio_tuning_high)/100.0f), false);     // 2nd parameter is false because we do not want to save to eeprom because this would have a performance impact
        break;

    case CH6_CIRCLE_RATE:
        // set circle rate
        g.circle_rate.set(g.rc_6.control_in/25-20);     // allow approximately 45 degree turn rate in either direction
        break;

    case CH6_SONAR_GAIN:
        // set sonar gain
        g.sonar_gain.set(tuning_value);
        break;
    }
}

AP_HAL_MAIN();
