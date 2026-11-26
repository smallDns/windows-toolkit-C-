#ifndef TK_PROCESSES_H
#define TK_PROCESSES_H
typedef struct {
    unsigned long pid;
    char name[512];
    char username[256];
    unsigned long long memory; /* working set (RSS) em bytes */
    unsigned order;
} ProcInfo;
/* Retorna ate `limit` processos ordenados por RAM (desc). Libere *out com free(). */
int  proc_get_data(ProcInfo **out, int limit);
void proc_show(void);
#endif
