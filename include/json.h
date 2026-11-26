#ifndef TK_JSON_H
#define TK_JSON_H
#include <stdio.h>

/* Escritor JSON minimo, com a mesma formatacao de
 * json.dumps(obj, indent=2, ensure_ascii=False) do Python. */
typedef struct {
    FILE *f;
    int depth;
    int after_key;
    int count[32];
} JsonWriter;

void json_init(JsonWriter *w, FILE *f);
void json_begin_object(JsonWriter *w);
void json_end_object(JsonWriter *w);
void json_begin_array(JsonWriter *w);
void json_end_array(JsonWriter *w);
void json_key(JsonWriter *w, const char *key);
void json_string(JsonWriter *w, const char *s);
void json_int(JsonWriter *w, long long v);
void json_uint(JsonWriter *w, unsigned long long v);
void json_double(JsonWriter *w, double v, int decimals);
void json_bool(JsonWriter *w, int v);
void json_null(JsonWriter *w);
#endif
