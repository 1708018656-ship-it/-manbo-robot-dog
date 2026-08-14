# Manbo Robot Dog

> 🐕 *An STM32 quadruped that walks around and occasionally falls over.*

## Why this exists
Building a quadruped robot from scratch is a rite of passage. I wanted to understand inverse kinematics, servo synchronization, and how difficult it is to keep a four-legged machine balanced when real-world physics gets in the way of math.

## The Real Deal
- **STM32 Firmware**: Written in C/C++ with bare-metal and RTOS considerations. Managing interrupt priorities for servo PWM pulses while handling sensor inputs is a constant balancing act.
- **Inverse Kinematics**: The math looks neat on paper, but mechanical slack in cheap servos means the legs never quite land where the equations predict. Calibrating offsets was 80% of the work.
- **Voice & OLED**: Added voice interaction and expressions just to give the robot some personality when it's sitting idle on the desk.

## Lessons Learned
Servo jitter, power drops when all four legs load up simultaneously, and the eternal search for a battery that doesn't sag under high discharge. It's a messy, satisfying hardware-software sandbox.
