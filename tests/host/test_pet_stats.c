/* Host tests for PetStats.h: values must match legacy Mode.c exactly. */
#include <stdio.h>
#include "PetStats.h"

static int failures = 0, assertions = 0;
#define CHECK(c) do { assertions++; if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)

int main(void)
{
    int s, h; uint16_t rep = 0; uint8_t prev = '0'; int i;

    /* Walking forward: -5 stamina each cycle, unhappy from the 6th repeat */
    s = 500; h = 100; rep = 0; prev = '0';
    for (i = 0; i < 5; ++i) { PetStats_Apply('f', prev, &rep, &s, &h); prev = 'f'; }
    CHECK(s == 475); CHECK(h == 100); CHECK(rep == 4);
    PetStats_Apply('f', prev, &rep, &s, &h);
    CHECK(s == 470); CHECK(h == 97); CHECK(rep == 5);

    /* Changing gait resets the boredom counter */
    PetStats_Apply('l', 'f', &rep, &s, &h);
    CHECK(rep == 0); CHECK(h == 97);
    PetStats_Apply('d', 'l', &rep, &s, &h);
    CHECK(rep == 0); CHECK(h == 102);

    /* Sleep restores stamina to 100 and +15 happiness */
    s = 3; h = 10; PetStats_Apply('p', '0', &rep, &s, &h);
    CHECK(s == 100); CHECK(h == 25);

    /* Clamping */
    s = 498; h = 199; PetStats_Apply('s', '0', &rep, &s, &h);
    CHECK(s == PET_STAMINA_MAX); CHECK(h == PET_HAPPINESS_MAX);

    /* Unknown / status commands change nothing */
    s = 50; h = 50; PetStats_Apply('K', '0', &rep, &s, &h);
    CHECK(s == 50); CHECK(h == 50);

    CHECK(PetStats_IsContinuous('f')); CHECK(PetStats_IsContinuous('d'));
    CHECK(!PetStats_IsContinuous('o')); CHECK(!PetStats_IsContinuous('5'));

    printf("pet_stats_tests: %d assertions, %d failures\n", assertions, failures);
    return failures != 0;
}
