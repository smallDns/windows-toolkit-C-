#include "platform.h"
#include <powrprof.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "color.h"
#include "util.h"

#define CPU_REG_KEY "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"

static int reg_read_string(const char *name, char *out, size_t n)
{
    HKEY k;
    DWORD sz = (DWORD)(n - 1), type = 0;
    int ok = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, CPU_REG_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return 0;
    if (RegQueryValueExA(k, name, NULL, &type, (BYTE *)out, &sz) == ERROR_SUCCESS && type == REG_SZ) {
        out[sz < n ? sz : n - 1] = '\0';
        out[n - 1] = '\0';
        ok = 1;
    }
    RegCloseKey(k);
    return ok;
}

static int reg_read_dword(const char *name, DWORD *v)
{
    HKEY k;
    DWORD sz = sizeof *v, type = 0;
    int ok = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, CPU_REG_KEY, 0, KEY_READ, &k) != ERROR_SUCCESS) return 0;
    if (RegQueryValueExA(k, name, NULL, &type, (BYTE *)v, &sz) == ERROR_SUCCESS && type == REG_DWORD) ok = 1;
    RegCloseKey(k);
    return ok;
}

void cpu_model(char *out, unsigned long n)
{
    if (reg_read_string("ProcessorNameString", out, n)) {
        str_trim(out);
        if (out[0]) return;
    }
    copy_str(out, n, "Indisponivel");
}

static int count_physical_cores(void)
{
    DWORD len = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || len == 0) return 0;

    BYTE *buf = (BYTE *)malloc(len);
    if (!buf) return 0;
    if (!GetLogicalProcessorInformationEx(RelationProcessorCore,
                                          (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buf, &len)) {
        free(buf);
        return 0;
    }
    int count = 0;
    for (DWORD off = 0; off < len;) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)(buf + off);
        if (info->Relationship == RelationProcessorCore) count++;
        if (info->Size == 0) break;
        off += info->Size;
    }
    free(buf);
    return count;
}

typedef struct { ULONG Number, MaxMhz, CurrentMhz, MhzLimit, MaxIdleState, CurrentIdleState; } TkProcPowerInfo;

static double frequency_mhz(int logical)
{
    double mhz = 0.0;
    if (logical > 0) {
        TkProcPowerInfo *ppi =
            (TkProcPowerInfo *)calloc((size_t)logical, sizeof *ppi);
        if (ppi) {
            if (CallNtPowerInformation(ProcessorInformation, NULL, 0, ppi,
                                       (ULONG)((size_t)logical * sizeof *ppi)) == 0)
                mhz = (double)ppi[0].CurrentMhz;
            free(ppi);
        }
    }
    if (mhz <= 0.0) { /* fallback: frequencia nominal gravada no registro */
        DWORD v = 0;
        if (reg_read_dword("~MHz", &v)) mhz = (double)v;
    }
    return mhz;
}

static unsigned long long ft_u64(FILETIME ft)
{
    return ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
}

/* Equivalente a psutil.cpu_percent(interval=...): amostra GetSystemTimes duas vezes. */
double cpu_usage_percent(unsigned long interval_ms)
{
    FILETIME i1, k1, u1, i2, k2, u2;
    if (!GetSystemTimes(&i1, &k1, &u1)) return 0.0;
    Sleep((DWORD)interval_ms);
    if (!GetSystemTimes(&i2, &k2, &u2)) return 0.0;

    unsigned long long idle = ft_u64(i2) - ft_u64(i1);
    unsigned long long total = (ft_u64(k2) - ft_u64(k1)) + (ft_u64(u2) - ft_u64(u1)); /* kernel inclui idle */
    if (total == 0) return 0.0;
    double pct = (double)(total - idle) * 100.0 / (double)total;
    if (pct < 0.0) pct = 0.0;
    if (pct > 100.0) pct = 100.0;
    return round1(pct);
}

void cpu_get_data(CpuInfo *d)
{
    memset(d, 0, sizeof *d);
    cpu_model(d->model, sizeof d->model);
    d->physical_cores = count_physical_cores();
    d->logical_cores = (int)GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    d->usage_percent = cpu_usage_percent(1000);
    d->frequency_mhz = round1(frequency_mhz(d->logical_cores));
}

void cpu_show(void)
{
    CpuInfo d;
    char phys[16], logi[16], freq[32];

    cpu_get_data(&d);
    if (d.physical_cores > 0) snprintf(phys, sizeof phys, "%d", d.physical_cores); else copy_str(phys, sizeof phys, "N/A");
    if (d.logical_cores > 0)  snprintf(logi, sizeof logi, "%d", d.logical_cores);  else copy_str(logi, sizeof logi, "N/A");
    if (d.frequency_mhz > 0.0) snprintf(freq, sizeof freq, "%.1f", d.frequency_mhz); else copy_str(freq, sizeof freq, "N/A");

    printf("    " AZUL "Modelo            " BRANCO ":" RESET " " BRANCO "%s" RESET "\n", d.model);
    printf("    " AZUL "Nucleos fisicos   " BRANCO ":" RESET " " AZUL_CELESTE "%s" RESET "\n", phys);
    printf("    " AZUL "Nucleos logicos   " BRANCO ":" RESET " " AZUL_CELESTE "%s" RESET "\n", logi);
    printf("    " AZUL "Uso atual         " BRANCO ":" RESET " %.1f%%\n", d.usage_percent);
    printf("    " AZUL "Frequencia        " BRANCO ":" RESET " " LARANJA "%s MHz" RESET "\n", freq);
}
