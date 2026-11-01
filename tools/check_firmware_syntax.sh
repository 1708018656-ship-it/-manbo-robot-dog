#!/usr/bin/env bash
# Syntax/warning check of every C file in the Keil project for Cortex-M3,
# using clang (no ARM toolchain needed). Fails on any error.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
STUB="$(mktemp -d)"
trap 'rm -rf "$STUB"' EXIT
cat > "$STUB/stdio.h" <<'H'
#pragma once
#include <stddef.h>
#include <stdarg.h>
typedef struct __FILE FILE; extern FILE *stdout;
int printf(const char*,...); int sprintf(char*,const char*,...); int snprintf(char*,size_t,const char*,...);
int vsprintf(char*,const char*,va_list); int vsnprintf(char*,size_t,const char*,va_list); int fputc(int,FILE*); int fgetc(FILE*);
H
printf '#pragma once\n#include <stddef.h>\nint abs(int); int rand(void); void srand(unsigned); int atoi(const char*);\n' > "$STUB/stdlib.h"
printf '#pragma once\n#include <stddef.h>\nsize_t strlen(const char*); void*memcpy(void*,const void*,size_t); void*memset(void*,int,size_t); int strcmp(const char*,const char*); int strncmp(const char*,const char*,size_t); char*strncpy(char*,const char*,size_t);\n' > "$STUB/string.h"
printf '#pragma once\ndouble sqrt(double); double pow(double,double); double sin(double); double cos(double); double fabs(double); double atan2(double,double); double round(double); float sinf(float); float cosf(float);\n' > "$STUB/math.h"

FILES=$(grep -oP '(?<=<FilePath>)[^<]+\.c(?=</FilePath>)' Project.uvprojx | tr '\\' '/' | grep -v -E 'Library/|core_cm3')
status=0
for f in $FILES; do
  clang --target=thumbv7m-none-eabi -mcpu=cortex-m3 -ffreestanding -fsyntax-only -std=c99 \
    -Wall -Wextra -Wno-unused-parameter -Wno-missing-braces -Wno-invalid-source-encoding \
    -Werror=implicit-function-declaration \
    -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -isystem "$STUB" \
    -IStart -ILibrary -IUser -ISystem -IHardware "$f" || status=1
done
[ $status -eq 0 ] && echo "firmware syntax check: OK ($(echo "$FILES" | wc -l) files)"
exit $status
