#include "Voice.h"
#include "BoardConfig.h"
#include "AudioTask.h"
#include "usart3.h"

#if MANBO_HAS_SPEECH
static const char *Voice_Text(VoiceId id)
{
    switch (id)
    {
        case VOICE_BOOT:      return "[v12][m0][t5]" "\xD0\xA1\xB4\xF4\xC0\xB4\xC0\xB2"; /* 小呆来啦 */
        case VOICE_AUTO_STOP: return "[v12][m0][t5]" "\xD7\xDF\xC0\xDB\xC1\xCB\xA3\xAC\xD0\xAA\xD2\xBB\xBB\xE1\xB6\xF9"; /* 走累了，歇一会儿 */
        case VOICE_OK:        return "[v12][m0][t5]" "\xBA\xC3\xB5\xC4"; /* 好的 */
        default:              return 0;
    }
}
#endif

uint8_t Voice_Play(VoiceId id)
{
#if MANBO_HAS_SPEECH
    const char *text = Voice_Text(id);
    return (text != 0) ? AudioTask_RequestText(text) : 0U;
#elif MANBO_HAS_VOICE_REPLY
    uint8_t frame[VOICE_FRAME_LEN];
    if (USART3_TxFree() < VOICE_FRAME_LEN)
    {
        return 0U;      /* never send half a frame */
    }
    Voice_BuildFrame(id, frame);
    return (USART3_SendBytes(frame, VOICE_FRAME_LEN) == VOICE_FRAME_LEN) ? 1U : 0U;
#else
    (void)id;
    return 0U;
#endif
}
