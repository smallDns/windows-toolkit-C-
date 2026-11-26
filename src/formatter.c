#include <stdio.h>
#include "formatter.h"

void format_bytes(double value, char *out, size_t n)
{
    static const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    for (int i = 0; i < 6; i++) {
        if (value < 1024.0 || i == 5) {
            snprintf(out, n, "%.2f %s", value, units[i]);
            return;
        }
        value /= 1024.0;
    }
}

void percent_bar(double percent, char *out, size_t n)
{
    enum { WIDTH = 24 };
    char bar[WIDTH + 1];
    int filled = (int)(WIDTH * percent / 100.0);
    if (filled < 0) filled = 0;
    if (filled > WIDTH) filled = WIDTH;
    for (int i = 0; i < WIDTH; i++) bar[i] = (i < filled) ? '#' : '-';
    bar[WIDTH] = '\0';
    snprintf(out, n, "[%s] %.1f%%", bar, percent);
}

void heading(const char *text)
{
    for (int i = 0; i < 60; i++) putchar('=');
    printf("\n  %s\n", text);
    for (int i = 0; i < 60; i++) putchar('=');
    putchar('\n');
}
