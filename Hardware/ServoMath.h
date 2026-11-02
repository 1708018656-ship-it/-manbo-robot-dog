#ifndef SERVO_MATH_H
#define SERVO_MATH_H

/*
 * Servo angle -> TIM3 compare conversion with per-servo calibration.
 * Header-only so it is host-testable.
 *
 * TIM3 ticks at 1 MHz with a 20 ms period, so the compare value is the
 * pulse width in microseconds: 500 us = 0 deg, 2500 us = 180 deg (SG90).
 *
 * Calibration (override with -D in Keil / Makefile, or edit here):
 *   SERVO_TRIM_DEG  per-servo offset in degrees. SG90 horns only fit every
 *                   ~12 deg on the spline, so a freshly assembled dog almost
 *                   never stands level; trim instead of re-seating horns.
 *   SERVO_SAFE_MIN_DEG / SERVO_SAFE_MAX_DEG
 *                   soft end stops. Driving an SG90 into its hard stop
 *                   stalls it (~700 mA each) and is the usual cause of
 *                   brown-out resets when all four do it at once.
 */

#include <stdint.h>

#ifndef SERVO_TRIM_DEG
#define SERVO_TRIM_DEG { 0, 0, 0, 0 }
#endif
#ifndef SERVO_SAFE_MIN_DEG
#define SERVO_SAFE_MIN_DEG 0     /* gaits use 10..180; tighten if your linkage binds */
#endif
#ifndef SERVO_SAFE_MAX_DEG
#define SERVO_SAFE_MAX_DEG 180   /* hello/stretch wave the paw at 180 */
#endif

#define SERVO_PULSE_MIN_US 500
#define SERVO_PULSE_MAX_US 2500

static const int8_t g_servo_trim_deg[4] = SERVO_TRIM_DEG;

#if (SERVO_SAFE_MIN_DEG < 0) || (SERVO_SAFE_MAX_DEG > 180) || (SERVO_SAFE_MIN_DEG >= SERVO_SAFE_MAX_DEG)
#error "SERVO_SAFE_MIN_DEG/SERVO_SAFE_MAX_DEG must satisfy 0 <= min < max <= 180"
#endif

static inline int16_t ServoMath_Clamp(int16_t deg)
{
    if (deg < SERVO_SAFE_MIN_DEG) { return SERVO_SAFE_MIN_DEG; }
    if (deg > SERVO_SAFE_MAX_DEG) { return SERVO_SAFE_MAX_DEG; }
    return deg;
}

/* index 0..3 = servo 1..4; angle in whole degrees as used by the gaits */
static inline uint16_t ServoMath_AngleToCompare(uint8_t index, int16_t angle_deg)
{
    int16_t deg = angle_deg;
    if (index < 4U) { deg = (int16_t)(deg + g_servo_trim_deg[index]); }
    deg = ServoMath_Clamp(deg);
    /* integer, rounded: 500 + deg * 2000 / 180 */
    return (uint16_t)(SERVO_PULSE_MIN_US +
                      ((int32_t)deg * (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US) + 90) / 180);
}

static inline uint8_t ServoMath_CompareToAngle(uint16_t compare)
{
    if (compare <= SERVO_PULSE_MIN_US) { return 0U; }
    if (compare >= SERVO_PULSE_MAX_US) { return 180U; }
    return (uint8_t)(((uint32_t)(compare - SERVO_PULSE_MIN_US) * 180U + 1000U) /
                     (SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US));
}

#endif /* SERVO_MATH_H */
