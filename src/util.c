#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "util.h"

void fmt_thousands(unsigned long long v, char *out, size_t n)
{
    char tmp[32];
    size_t len, o = 0;
    if (n == 0) return;
    snprintf(tmp, sizeof tmp, "%llu", v);
    len = strlen(tmp);
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && (len - i) % 3 == 0 && o + 1 < n) out[o++] = ',';
        if (o + 1 < n) out[o++] = tmp[i];
    }
    out[o] = '\0';
}

void utf8_truncate(char *s, size_t max_chars)
{
    size_t count = 0;
    for (char *p = s; *p; p++) {
        if (((unsigned char)*p & 0xC0) != 0x80) { /* inicio de um caractere */
            if (count == max_chars) { *p = '\0'; return; }
            count++;
        }
    }
}

void str_trim(char *s)
{
    size_t len = strlen(s), start = 0;
    while (len > 0 && isspace((unsigned char)s[len - 1])) len--;
    while (start < len && isspace((unsigned char)s[start])) start++;
    memmove(s, s + start, len - start);
    s[len - start] = '\0';
}

int contains_ci(const char *hay, const char *needle)
{
    size_t nl = strlen(needle);
    if (nl == 0) return 1;
    for (; *hay; hay++) {
        size_t i = 0;
        while (i < nl && hay[i] &&
               tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i]))
            i++;
        if (i == nl) return 1;
    }
    return 0;
}

void copy_str(char *dst, size_t n, const char *src)
{
    if (n == 0) return;
    snprintf(dst, n, "%s", src ? src : "");
}

double round1(double v) { return floor(v * 10.0 + 0.5) / 10.0; }
