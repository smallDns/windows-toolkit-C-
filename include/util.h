#ifndef TK_UTIL_H
#define TK_UTIL_H
#include <stddef.h>

void fmt_thousands(unsigned long long v, char *out, size_t n); /* 1234567 -> "1,234,567" */
void utf8_truncate(char *s, size_t max_chars);                 /* equivale a s[:max_chars] */
void str_trim(char *s);                                        /* equivale a str.strip()   */
int  contains_ci(const char *hay, const char *needle);         /* needle in hay.lower()    */
void copy_str(char *dst, size_t n, const char *src);           /* strcpy seguro            */
double round1(double v);                                       /* round(v, 1)              */
#endif
