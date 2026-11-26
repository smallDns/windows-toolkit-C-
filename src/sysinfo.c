#include "platform.h"
#include <lmcons.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sysinfo.h"
#include "color.h"
#include "util.h"

typedef LONG (WINAPI *RtlGetVersionFn)(PRTL_OSVERSIONINFOW);

void sysinfo_get_data(SysInfo *d)
{
    wchar_t w[UNLEN + 1];
    DWORD n;

    memset(d, 0, sizeof *d);

    n = (DWORD)(sizeof w / sizeof w[0]);
    if (GetComputerNameExW(ComputerNameDnsHostname, w, &n)) wide_to_utf8(w, d->hostname, sizeof d->hostname);
    else copy_str(d->hostname, sizeof d->hostname, "Indisponivel");

    n = (DWORD)(sizeof w / sizeof w[0]);
    if (GetUserNameW(w, &n)) wide_to_utf8(w, d->username, sizeof d->username);
    else copy_str(d->username, sizeof d->username, "Indisponivel");

    copy_str(d->system, sizeof d->system, "Windows");

    /* RtlGetVersion nao sofre com o "version lie" de GetVersionEx. */
    RTL_OSVERSIONINFOW vi;
    memset(&vi, 0, sizeof vi);
    vi.dwOSVersionInfoSize = sizeof vi;
    RtlGetVersionFn fn = (RtlGetVersionFn)(void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
    if (fn && fn(&vi) == 0) {
        snprintf(d->windows_release, sizeof d->windows_release, "%s", vi.dwBuildNumber >= 22000 ? "11" : "10");
        snprintf(d->version, sizeof d->version, "%lu.%lu.%lu",
                 (unsigned long)vi.dwMajorVersion, (unsigned long)vi.dwMinorVersion,
                 (unsigned long)vi.dwBuildNumber);
    } else {
        copy_str(d->windows_release, sizeof d->windows_release, "Indisponivel");
        copy_str(d->version, sizeof d->version, "Indisponivel");
    }

    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64: copy_str(d->architecture, sizeof d->architecture, "AMD64"); break;
    case PROCESSOR_ARCHITECTURE_ARM64: copy_str(d->architecture, sizeof d->architecture, "ARM64"); break;
    case PROCESSOR_ARCHITECTURE_INTEL: copy_str(d->architecture, sizeof d->architecture, "x86");   break;
    case PROCESSOR_ARCHITECTURE_ARM:   copy_str(d->architecture, sizeof d->architecture, "ARM");   break;
    default:                           copy_str(d->architecture, sizeof d->architecture, "Desconhecida");
    }

    copy_str(d->runtime, sizeof d->runtime, "C (Win32 API)");

    time_t boot = time(NULL) - (time_t)(GetTickCount64() / 1000ULL);
    struct tm *t = localtime(&boot);
    if (t) strftime(d->boot_time, sizeof d->boot_time, "%Y-%m-%d %H:%M:%S", t);
    else copy_str(d->boot_time, sizeof d->boot_time, "Indisponivel");
}

void sysinfo_show(void)
{
    SysInfo d;
    sysinfo_get_data(&d);

    printf(AZUL "%-22s" BRANCO ":" RESET " " AZUL_CELESTE "%s" RESET "\n", "Hostname",        d.hostname);
    printf(AZUL "%-22s" BRANCO ":" RESET " " VERDE        "%s" RESET "\n", "Username",        d.username);
    printf(AZUL "%-22s" BRANCO ":" RESET " " BRANCO       "%s" RESET "\n", "System",          d.system);
    printf(AZUL "%-22s" BRANCO ":" RESET " " BRANCO       "%s" RESET "\n", "Windows Release", d.windows_release);
    printf(AZUL "%-22s" BRANCO ":" RESET " " BRANCO       "%s" RESET "\n", "Version",         d.version);
    printf(AZUL "%-22s" BRANCO ":" RESET " " BRANCO       "%s" RESET "\n", "Architecture",    d.architecture);
    printf(AZUL "%-22s" BRANCO ":" RESET " " LARANJA      "%s" RESET "\n", "Runtime",         d.runtime);
    printf(AZUL "%-22s" BRANCO ":" RESET " " AMARELO      "%s" RESET "\n", "Boot Time",       d.boot_time);
}
