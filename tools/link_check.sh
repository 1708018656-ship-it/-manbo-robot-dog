#!/usr/bin/env bash
# Local link check without an ARM GCC toolchain: compile every firmware file
# with clang for Cortex-M3, link with ld.lld, and fail on any undefined
# symbol that newlib/libgcc would not provide. Usage: tools/link_check.sh [""|V2|LITE]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"; cd "$ROOT"
case "${1:-}" in V2) D="-DMANBO_BOARD_V2"; M="V2=1";; LITE) D="-DMANBO_LITE"; M="LITE=1";; *) D=""; M="";; esac
OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT
STUB="$OUT/stub"; mkdir -p "$STUB"
printf '#pragma once\n#include <stddef.h>\n#include <stdarg.h>\ntypedef struct __FILE FILE; extern FILE *stdout;\nint printf(const char*,...); int sprintf(char*,const char*,...); int snprintf(char*,size_t,const char*,...); int vsprintf(char*,const char*,va_list); int vsnprintf(char*,size_t,const char*,va_list); int fputc(int,FILE*);\n' > "$STUB/stdio.h"
printf '#pragma once\n#include <stddef.h>\nint abs(int); int rand(void); void srand(unsigned); int atoi(const char*);\n' > "$STUB/stdlib.h"
printf '#pragma once\n#include <stddef.h>\nsize_t strlen(const char*); void*memcpy(void*,const void*,size_t); void*memset(void*,int,size_t); int strcmp(const char*,const char*); int strncmp(const char*,const char*,size_t); char*strncpy(char*,const char*,size_t);\n' > "$STUB/string.h"
printf '#pragma once\ndouble sqrt(double); double pow(double,double); double sin(double); double cos(double); double fabs(double); double atan2(double,double); double round(double); float sinf(float); float cosf(float);\n' > "$STUB/math.h"
for f in $(make -n $M 2>/dev/null | grep -oP '(?<=-c )\S+\.c' | grep -v -E 'core_cm3|syscalls'); do
  clang --target=thumbv7m-none-eabi -mcpu=cortex-m3 -mthumb -ffreestanding -Os -ffunction-sections -fdata-sections -fno-common -w \
    $D -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -isystem "$STUB" -IStart -ILibrary -IUser -ISystem -IHardware -c "$f" -o "$OUT/$(echo "$f" | tr '/.' '__').o"
done
LIBC='^(__aeabi_.*|mem(cpy|set|move|cmp)|str(len|cmp|ncmp|ncpy)|v?s?n?printf|rand|srand|atoi|abs|sqrtf?|powf?|sinf?|cosf?|fabsf?|atan2f?|roundf?)$'
UNDEF=$(ld.lld -T gcc/STM32F103C8Tx.ld --gc-sections "$OUT"/*.o -o "$OUT/m.elf" 2>&1 | grep -oP 'undefined symbol: \K\S+' | sort -u | grep -vE "$LIBC" || true)
if [ -n "$UNDEF" ]; then echo "UNDEFINED (not libc): $UNDEF"; exit 1; fi
ld.lld -T gcc/STM32F103C8Tx.ld --gc-sections --unresolved-symbols=ignore-all "$OUT"/*.o -o "$OUT/m.elf"
echo "link check ${1:-original}: OK, $(llvm-size "$OUT/m.elf" | tail -1 | awk '{print $1}') bytes text (excl. libc)"
