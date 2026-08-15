#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int format_serial(char *buffer, size_t capacity, const char *format, ...)
{
    va_list args;
    int written;

    if (format == NULL)
    {
        return -1;
    }
    va_start(args, format);
    written = vsnprintf(buffer, capacity, format, args);
    va_end(args);
    return written;
}

int main(void)
{
    char serial_buffer[100];
    char oled_buffer[30];
    char long_text[160];
    int written;

    memset(long_text, 'A', sizeof(long_text) - 1U);
    long_text[sizeof(long_text) - 1U] = '\0';

    written = format_serial(serial_buffer, sizeof(serial_buffer), "%s", long_text);
    assert(written == (int)(sizeof(long_text) - 1U));
    assert(serial_buffer[sizeof(serial_buffer) - 1U] == '\0');
    assert(strlen(serial_buffer) == sizeof(serial_buffer) - 1U);

    written = format_serial(oled_buffer, sizeof(oled_buffer), "%s", long_text);
    assert(written == (int)(sizeof(long_text) - 1U));
    assert(oled_buffer[sizeof(oled_buffer) - 1U] == '\0');
    assert(strlen(oled_buffer) == sizeof(oled_buffer) - 1U);

    assert(format_serial(serial_buffer, sizeof(serial_buffer), NULL) == -1);
    puts("format_contract_tests: passed");
    return 0;
}
