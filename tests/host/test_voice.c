/* Host test: SU-03T reply frame layout must match docs/su03t.md. */
#include <stdio.h>
#include <string.h>
#include "Voice.h"

int main(void)
{
    uint8_t f[VOICE_FRAME_LEN];
    const uint8_t boot[] = {0xAA, 0x55, 0x01, 0x55, 0xAA};
    const uint8_t stop[] = {0xAA, 0x55, 0x02, 0x55, 0xAA};
    int failures = 0;

    Voice_BuildFrame(VOICE_BOOT, f);
    failures += memcmp(f, boot, sizeof boot) != 0;
    Voice_BuildFrame(VOICE_AUTO_STOP, f);
    failures += memcmp(f, stop, sizeof stop) != 0;
    printf("voice_tests: 2 assertions, %d failures\n", failures);
    return failures != 0;
}
