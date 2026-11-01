#ifndef PET_STATS_H
#define PET_STATS_H

/*
 * Pet "stamina" / "happiness" bookkeeping and continuous-gait policy.
 *
 * Before this module the stat changes lived inside the blocking legacy
 * mode_xxx() functions in Mode.c. After the PoseStep refactor those
 * functions are no longer reached for motion commands, so the stats must be
 * applied where ActionTask accepts a command. The numbers below are copied
 * 1:1 from the legacy Mode.c so gameplay is unchanged.
 *
 * Header-only (static inline) so it needs no Keil project change and can be
 * unit-tested on the host.
 */

#include <stdint.h>

#define PET_STAMINA_MAX      500
#define PET_HAPPINESS_MAX    200
#define PET_SLEEP_STAMINA    100
#define PET_WALK_BORED_AFTER 5U   /* same gait this many times -> unhappy */

/* Legacy behaviour: walking, swinging and dancing keep repeating until a new
 * command arrives. Everything else is a one-shot action. */
static inline uint8_t PetStats_IsContinuous(uint8_t cmd)
{
    switch (cmd)
    {
        case 'f': case 'b': case 'l': case 'r':
        case 'w': case 'z': case 'd':
            return 1U;
        default:
            return 0U;
    }
}

static inline void PetStats_Clamp(int *stamina, int *happiness)
{
    if (*stamina > PET_STAMINA_MAX) { *stamina = PET_STAMINA_MAX; }
    if (*happiness > PET_HAPPINESS_MAX) { *happiness = PET_HAPPINESS_MAX; }
}

/*
 * Apply the stat change for one accepted command.
 * repeat_count tracks how many times the same gait ran back-to-back
 * (replaces the four legacy counters ff/bb/ll/rr, which were mutually
 * exclusive anyway).
 */
static inline void PetStats_Apply(uint8_t cmd, uint8_t previous,
                                  uint16_t *repeat_count,
                                  int *stamina, int *happiness)
{
    int ds = 0;
    int dh = 0;

    switch (cmd)
    {
        case 'f': case 'b': case 'l': case 'r':
            if (previous == cmd)
            {
                if (*repeat_count < 0xFFFFU) { (*repeat_count)++; }
            }
            else
            {
                *repeat_count = 0U;
            }
            if (*repeat_count >= PET_WALK_BORED_AFTER) { dh = -3; }
            ds = -5;
            break;
        case 'w': case 'z': case 'd': ds = -5; dh = +5; break;
        case '5': case 'q':           ds = -1;          break;
        case 's':                     ds = +5; dh = +2; break;
        case 'o':                     ds = -2; dh = +2; break;
        case 'j':                     ds = -5; dh = +2; break;
        case 'y':                     ds = +5; dh = +1; break;
        case '1':                     ds = -5;          break;
        case 'p': case '2':
            *stamina = PET_SLEEP_STAMINA;
            dh = +15;
            break;
        case 'B':                     ds = -2; dh = +5; break;
        case 'Y':                     ds = -2; dh = +2; break;
        case 'X':                     ds = -2;          break;
        default:
            break;
    }

    if ((cmd != 'f') && (cmd != 'b') && (cmd != 'l') && (cmd != 'r'))
    {
        *repeat_count = 0U;
    }

    *stamina += ds;
    *happiness += dh;
    PetStats_Clamp(stamina, happiness);
}

#endif /* PET_STATS_H */
