#ifndef TK_DISK_H
#define TK_DISK_H
#define DISK_MAX 26
typedef struct {
    char device[8];
    char mountpoint[8];
    char filesystem[32];
    unsigned long long total, used, free;
    double percent;
} DiskInfo;
int  disk_get_data(DiskInfo *out, int max); /* retorna quantidade */
void disk_show(void);
#endif
