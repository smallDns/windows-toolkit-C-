#ifndef TK_FORMATTER_H
#define TK_FORMATTER_H
#include <stddef.h>
void format_bytes(double value, char *out, size_t n);  /* "12.34 GB" */
void percent_bar(double percent, char *out, size_t n); /* "[####----] 45.3%" */
void heading(const char *text);
#endif
