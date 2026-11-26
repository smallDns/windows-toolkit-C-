#include <string.h>
#include "json.h"

static void indent(JsonWriter *w)
{
    fputc('\n', w->f);
    for (int i = 0; i < w->depth * 2; i++) fputc(' ', w->f);
}

/* Prepara a escrita de um valor (virgula/quebra de linha quando for item de array). */
static void before_value(JsonWriter *w)
{
    if (w->after_key) { w->after_key = 0; return; }
    if (w->depth > 0) {
        if (w->count[w->depth]++ > 0) fputc(',', w->f);
        indent(w);
    }
}

static int utf8_seq_len(const unsigned char *s, size_t rem)
{
    unsigned char c = s[0];
    if (c < 0x80) return 1;
    if (c >= 0xC2 && c <= 0xDF)
        return (rem >= 2 && (s[1] & 0xC0) == 0x80) ? 2 : 0;
    if (c >= 0xE0 && c <= 0xEF) {
        if (rem < 3 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
        if (c == 0xE0 && s[1] < 0xA0) return 0;
        if (c == 0xED && s[1] >= 0xA0) return 0;
        return 3;
    }
    if (c >= 0xF0 && c <= 0xF4) {
        if (rem < 4 || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 ||
            (s[3] & 0xC0) != 0x80) return 0;
        if (c == 0xF0 && s[1] < 0x90) return 0;
        if (c == 0xF4 && s[1] >= 0x90) return 0;
        return 4;
    }
    return 0;
}

static void write_string(JsonWriter *w, const char *str)
{
    const unsigned char *p = (const unsigned char *)str;
    size_t rem = strlen(str);
    fputc('"', w->f);
    while (rem > 0) {
        unsigned char c = *p;
        int len = 1;
        switch (c) {
        case '"':  fputs("\\\"", w->f); break;
        case '\\': fputs("\\\\", w->f); break;
        case '\n': fputs("\\n", w->f);  break;
        case '\r': fputs("\\r", w->f);  break;
        case '\t': fputs("\\t", w->f);  break;
        case '\b': fputs("\\b", w->f);  break;
        case '\f': fputs("\\f", w->f);  break;
        default:
            if (c < 0x20) {
                fprintf(w->f, "\\u%04x", c);
            } else if (c < 0x80) {
                fputc(c, w->f);
            } else {
                len = utf8_seq_len(p, rem);
                if (len == 0) { fputs("\xEF\xBF\xBD", w->f); len = 1; } /* U+FFFD */
                else fwrite(p, 1, (size_t)len, w->f);
            }
        }
        p += len;
        rem -= (size_t)len;
    }
    fputc('"', w->f);
}

void json_init(JsonWriter *w, FILE *f)
{
    memset(w, 0, sizeof *w);
    w->f = f;
}

void json_begin_object(JsonWriter *w)
{
    before_value(w);
    fputc('{', w->f);
    if (w->depth < 31) w->depth++;
    w->count[w->depth] = 0;
}

void json_end_object(JsonWriter *w)
{
    int had = w->count[w->depth];
    w->depth--;
    if (had) indent(w);
    fputc('}', w->f);
}

void json_begin_array(JsonWriter *w)
{
    before_value(w);
    fputc('[', w->f);
    if (w->depth < 31) w->depth++;
    w->count[w->depth] = 0;
}

void json_end_array(JsonWriter *w)
{
    int had = w->count[w->depth];
    w->depth--;
    if (had) indent(w);
    fputc(']', w->f);
}

void json_key(JsonWriter *w, const char *key)
{
    if (w->count[w->depth]++ > 0) fputc(',', w->f);
    indent(w);
    write_string(w, key);
    fputs(": ", w->f);
    w->after_key = 1;
}

void json_string(JsonWriter *w, const char *s) { before_value(w); write_string(w, s); }
void json_int(JsonWriter *w, long long v)      { before_value(w); fprintf(w->f, "%lld", v); }
void json_uint(JsonWriter *w, unsigned long long v) { before_value(w); fprintf(w->f, "%llu", v); }
void json_double(JsonWriter *w, double v, int d) { before_value(w); fprintf(w->f, "%.*f", d, v); }
void json_bool(JsonWriter *w, int v)           { before_value(w); fputs(v ? "true" : "false", w->f); }
void json_null(JsonWriter *w)                  { before_value(w); fputs("null", w->f); }
