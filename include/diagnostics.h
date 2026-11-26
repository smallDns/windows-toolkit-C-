#ifndef TK_DIAGNOSTICS_H
#define TK_DIAGNOSTICS_H
#define DIAG_MAX 8
typedef struct { char name[64]; char detail[128]; int ok; } DiagResult;
int  diag_run_checks(DiagResult *out, int max); /* retorna quantidade */
void diag_show(void);
#endif
