#ifndef TK_CPU_H
#define TK_CPU_H
typedef struct {
    char   model[256];
    int    physical_cores; /* 0 = indisponivel */
    int    logical_cores;
    double usage_percent;
    double frequency_mhz;  /* <= 0 = indisponivel */
} CpuInfo;
void cpu_get_data(CpuInfo *d);
void cpu_show(void);
void cpu_model(char *out, unsigned long n);
double cpu_usage_percent(unsigned long interval_ms);
#endif
