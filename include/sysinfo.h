#ifndef TK_SYSINFO_H
#define TK_SYSINFO_H
typedef struct {
    char hostname[256];
    char username[256];
    char system[16];
    char windows_release[16];
    char version[64];
    char architecture[16];
    char runtime[64];
    char boot_time[32];
} SysInfo;
void sysinfo_get_data(SysInfo *d);
void sysinfo_show(void);
#endif
