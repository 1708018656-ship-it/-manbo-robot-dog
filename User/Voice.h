#ifndef VOICE_H
#define VOICE_H

/*
 * Voice replies triggered by the firmware (as opposed to the replies the
 * SU-03T speaks by itself when it recognises a phrase).
 *
 *   Full build : the phrase text is spoken by the SYN6288 (AudioTask).
 *   LITE build : a 5-byte frame  AA 55 <id> 55 AA  is sent to the SU-03T on
 *                USART3; the module plays the phrase configured for that
 *                frame on its platform ("串口输入" -> 播报). See docs/su03t.md.
 */

#include <stdint.h>

typedef enum
{
    VOICE_BOOT = 0x01,       /* 小呆来啦            power-up greeting        */
    VOICE_AUTO_STOP = 0x02,  /* 走累了，歇一会儿    walk timeout reached     */
    VOICE_OK = 0x03          /* 好的                generic acknowledgement  */
} VoiceId;

#define VOICE_FRAME_HEAD1 0xAAU
#define VOICE_FRAME_HEAD2 0x55U
#define VOICE_FRAME_TAIL1 0x55U
#define VOICE_FRAME_TAIL2 0xAAU
#define VOICE_FRAME_LEN   5U

/* Build the LITE frame for an id (pure, host-tested). */
static inline void Voice_BuildFrame(VoiceId id, uint8_t frame[VOICE_FRAME_LEN])
{
    frame[0] = VOICE_FRAME_HEAD1;
    frame[1] = VOICE_FRAME_HEAD2;
    frame[2] = (uint8_t)id;
    frame[3] = VOICE_FRAME_TAIL1;
    frame[4] = VOICE_FRAME_TAIL2;
}

/* Non-blocking; returns 1 if the reply was queued. */
uint8_t Voice_Play(VoiceId id);

#endif /* VOICE_H */
