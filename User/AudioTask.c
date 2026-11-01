#include "AudioTask.h"
#include "stm32f10x.h"
#include "usart2.h"
#include "GbkText.h"

#define AUDIO_FRAME_CAPACITY SYN6288_FRAME_CAPACITY
#define AUDIO_MAX_TEXT_LENGTH SYN6288_MAX_TEXT_BYTES

typedef struct
{
    uint8_t frame[AUDIO_FRAME_CAPACITY];
    uint16_t length;
    uint16_t index;
    uint8_t active;
} AudioFrame;

static AudioFrame g_audio;

void AudioTask_Init(void)
{
    g_audio.length = 0U;
    g_audio.index = 0U;
    g_audio.active = 0U;
}

uint8_t AudioTask_RequestText(const char *text)
{
    uint8_t text_length;
    uint16_t i;
    uint8_t checksum;

    if ((text == 0) || (g_audio.active != 0U))
    {
        return 0U;
    }

    /* GBK-safe truncation; also avoids uint8_t overflow of strlen() */
    text_length = (uint8_t)GbkText_SafeLength(text, AUDIO_MAX_TEXT_LENGTH);

    g_audio.frame[0] = 0xFDU;
    g_audio.frame[1] = 0x00U;
    g_audio.frame[2] = (uint8_t)(text_length + 3U);
    g_audio.frame[3] = 0x01U;
    g_audio.frame[4] = 0x21U; /* Music=2，保持与原高级模式一致。 */

    checksum = 0U;
    for (i = 0U; i < 5U; ++i)
    {
        checksum ^= g_audio.frame[i];
    }
    for (i = 0U; i < text_length; ++i)
    {
        g_audio.frame[5U + i] = (uint8_t)text[i];
        checksum ^= g_audio.frame[5U + i];
    }
    g_audio.frame[5U + text_length] = checksum;
    g_audio.length = (uint16_t)(text_length + SYN6288_FRAME_OVERHEAD);
    g_audio.index = 0U;
    g_audio.active = 1U;
    return 1U;
}

void AudioTask_Run(uint32_t now_ms)
{
    (void)now_ms;

    if (g_audio.active != 0U)
    {
        if (USART2_SendString(&g_audio.frame[g_audio.index], 1U) != 0U)
        {
            g_audio.index++;
            if (g_audio.index >= g_audio.length)
            {
                g_audio.active = 0U;
            }
        }
    }
}

uint8_t AudioTask_IsBusy(void)
{
    return g_audio.active;
}
