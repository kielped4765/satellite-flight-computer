#ifndef FLIGHT_TYPES_H
#define FLIGHT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ===========================================================
    ATTITUDE STATE
 Written by: Kiel_Pedersen
 Read by: telemetry_task, health_monitor, command_handler
 =========================================================== */

typedef struct {
        float quaternion[4];        /*  Attitude quaternion [w, x, y, z] */
        float omega[3];             /*  Angular velocity in body frame [rad/s] */
        float euler[3];             /*  Euler angles in radians [roll, pitch, yaw] */
        uint32_t timestamp_ms;      /* Mission elapsed time in milliseconds */
        uint8_t mode;               /* Current operating mode (FlightMode_t) */
}    ADCSState_t;

/* ===========================================================
    TELEMETRY PACKET (binary wire format sent over UART)
    __attribute__((packed)) = no padding bytes between fields.
    This MUST match the struct.unpack format in telemetry_parser.py
    ========================================================== */
#define TLM_MAGIC 0xA5C3

typedef struct {
    uint16_t magic;             /* Always TLM_MAGIC - marks packet start */
    uint16_t sequence;         /* Incrementing counter, detects drops */
    uint32_t timestamp_ms;
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float omega_x;
    float omega_y;
    float omega_z;
    uint8_t mode;
    uint8_t fault_flags;     /* Bitmask - see Fault_ * defines below */
    int16_t temperature;    /* Degrees C * 10 e.g. 235 = 23.5 C */
    uint16_t checksum;      /* XOR of all prteceding bytes */
}   TelemetryPacket_t;

#pragma pack(pop)

/* ===========================================================
 FAULT FLAGS (bitmask stored in fault_flags field)
 Multiple faults can be active simultaneously.
 =========================================================== */

 #define FAULT_NONE             0x00
 #define FAULT_ADCS_TIMEOUT     0x01   /* ADCS missed its heartbeat */
 #define FAULT_SENSOR_INVALID   0x02   /* Sensor reading out of range */
 #define FAULT_WATCHDOG         0x04   /* Watchdog nearing expiry */
 #define FAULT_STACK_LOW        0x08   /* A task stack is nearly full */
 #define FAULT_QUEUE_FULL       0x10   /* Telemetry queue overflowed */

/* ===========================================================
   OPERATING MODES
   =========================================================== */

typedef enum {
    MODE_SAFE       = 0,   /* Emergency: minimal ops, await ground command */
    MODE_DETUMBLE   = 1,   /* Reduce rotation after launch */
    MODE_NADIR      = 2,   /* Point camera/antenna toward Earth */
    MODE_NOMINAL    = 3,   /* Full science/operations mode */
} FlightMode_t;

#endif /* FLIGHT_TYPES_H */