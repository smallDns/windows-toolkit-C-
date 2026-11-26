#ifndef TK_MEMINFO_H
#define TK_MEMINFO_H
typedef struct {
    unsigned long long total, used, available;
    double percent;
    unsigned long long swap_total, swap_used;
    double swap_percent;
} MemInfo;
void mem_get_data(MemInfo *d);
void mem_show(void);
#endif
