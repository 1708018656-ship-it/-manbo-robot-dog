#ifndef GBK_TEXT_H
#define GBK_TEXT_H

/*
 * GBK-safe text length helper for the SYN6288 speech module.
 *
 * SYN6288 expects GBK-encoded text. Chinese characters are two bytes in GBK
 * (lead byte >= 0x81). Cutting a string at an arbitrary byte boundary can
 * leave a dangling lead byte, which the module speaks as noise or which
 * swallows the checksum byte. This helper returns the longest prefix length
 * that is <= max_len and never splits a double-byte character.
 *
 * Header-only so it can be shared by the blocking driver (syn6288.c), the
 * non-blocking AudioTask, and host unit tests without touching the Keil
 * project file list.
 */

#include <stddef.h>

/* SYN6288 datasheet: at most 200 bytes of text per synthesis frame. */
#define SYN6288_MAX_TEXT_BYTES 200U
/* 5-byte header (FD, len_hi, len_lo, cmd, param) + text + 1-byte XOR checksum */
#define SYN6288_FRAME_OVERHEAD 6U
#define SYN6288_FRAME_CAPACITY (SYN6288_MAX_TEXT_BYTES + SYN6288_FRAME_OVERHEAD)

static inline size_t GbkText_SafeLength(const char *text, size_t max_len)
{
    size_t i = 0U;

    if (text == NULL)
    {
        return 0U;
    }

    while (text[i] != '\0')
    {
        size_t char_len = ((unsigned char)text[i] >= 0x81U) ? 2U : 1U;

        if ((char_len == 2U) && (text[i + 1U] == '\0'))
        {
            /* Malformed trailing lead byte: drop it. */
            break;
        }
        if ((i + char_len) > max_len)
        {
            break;
        }
        i += char_len;
    }
    return i;
}

#endif /* GBK_TEXT_H */
