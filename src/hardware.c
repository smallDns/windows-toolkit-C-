#include "platform.h"
#include <stdio.h>
#include <string.h>
#include "hardware.h"
#include "cpu.h"
#include "color.h"
#include "util.h"

#define PS_TIMEOUT_MS 12000
#define PS_UNAVAILABLE "PowerShell nao disponivel"

/* Executa `powershell -NoProfile -Command <query>` (timeout de 12 s) e captura a saida. */
static void powershell(const char *query, char *out, size_t outsz)
{
    SECURITY_ATTRIBUTES sa;
    HANDLE rd = NULL, wr = NULL, nul = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    char cmd[2048];

    memset(&pi, 0, sizeof pi);
    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&rd, &wr, &sa, 0)) { copy_str(out, outsz, PS_UNAVAILABLE); return; }
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0); /* so a ponta de escrita e herdada */
    nul = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      &sa, OPEN_EXISTING, 0, NULL);

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = nul;
    si.hStdOutput = wr;
    si.hStdError = nul; /* stderr descartado, como no original */

    /* Forca saida em UTF-8 (sem BOM), equivalente ao encoding="utf-8" do subprocess. */
    snprintf(cmd, sizeof cmd,
             "powershell -NoProfile -Command \"[Console]::OutputEncoding="
             "New-Object System.Text.UTF8Encoding $false; %s\"", query);

    BOOL started = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(wr); /* fecha a nossa copia para detectar EOF */
    if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
    if (!started) { CloseHandle(rd); copy_str(out, outsz, PS_UNAVAILABLE); return; }

    size_t len = 0;
    int timed_out = 0;
    DWORD t0 = GetTickCount();
    for (;;) {
        DWORD avail = 0;
        if (!PeekNamedPipe(rd, NULL, 0, NULL, &avail, NULL)) break; /* pipe fechado: terminou */
        if (avail > 0) {
            char tmp[1024];
            DWORD got = 0;
            if (!ReadFile(rd, tmp, avail < sizeof tmp ? avail : (DWORD)sizeof tmp, &got, NULL) || got == 0) break;
            size_t room = outsz - 1 - len;
            size_t take = got < room ? got : room; /* excedente e descartado */
            memcpy(out + len, tmp, take);
            len += take;
        } else {
            WaitForSingleObject(pi.hProcess, 20);
        }
        if (GetTickCount() - t0 > PS_TIMEOUT_MS) {
            timed_out = 1;
            TerminateProcess(pi.hProcess, 1);
            break;
        }
    }
    out[len] = '\0';

    DWORD code = 1;
    if (!timed_out) {
        WaitForSingleObject(pi.hProcess, 2000);
        GetExitCodeProcess(pi.hProcess, &code);
    }
    CloseHandle(rd);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (timed_out) { copy_str(out, outsz, PS_UNAVAILABLE); return; }
    if (code != 0) { copy_str(out, outsz, "Consulta nao disponivel"); return; }

    /* Remove BOM, '\r' e espacos nas pontas (.strip() + newlines universais). */
    char *src = out;
    if ((unsigned char)src[0] == 0xEF && (unsigned char)src[1] == 0xBB && (unsigned char)src[2] == 0xBF) src += 3;
    size_t o = 0;
    for (size_t i = 0; src[i]; i++) if (src[i] != '\r') out[o++] = src[i];
    out[o] = '\0';
    str_trim(out);
    if (!out[0]) copy_str(out, outsz, "Informacao indisponivel");
}

void hw_get_data(HwInfo *d)
{
    static const char *labels[HW_ITEMS] = {
        "CPU", "RAM instalada", "Placa-mae", "Fabricante da placa-mae", "BIOS", "GPU", "Discos fisicos"
    };
    for (int i = 0; i < HW_ITEMS; i++) { d->items[i].label = labels[i]; d->items[i].value[0] = '\0'; }

    cpu_model(d->items[0].value, HW_VALUE_MAX);

    MEMORYSTATUSEX m;
    memset(&m, 0, sizeof m);
    m.dwLength = sizeof m;
    if (GlobalMemoryStatusEx(&m))
        snprintf(d->items[1].value, HW_VALUE_MAX, "%.2f GB", (double)m.ullTotalPhys / (1024.0 * 1024.0 * 1024.0));
    else
        copy_str(d->items[1].value, HW_VALUE_MAX, "Indisponivel");

    powershell("(Get-CimInstance Win32_BaseBoard | Select-Object -First 1 -ExpandProperty Product)",
               d->items[2].value, HW_VALUE_MAX);
    powershell("(Get-CimInstance Win32_BaseBoard | Select-Object -First 1 -ExpandProperty Manufacturer)",
               d->items[3].value, HW_VALUE_MAX);
    powershell("(Get-CimInstance Win32_BIOS | Select-Object -First 1 -ExpandProperty SMBIOSBIOSVersion)",
               d->items[4].value, HW_VALUE_MAX);
    powershell("(Get-CimInstance Win32_VideoController | Select-Object -ExpandProperty Name)",
               d->items[5].value, HW_VALUE_MAX);
    powershell("(Get-PhysicalDisk | Select-Object FriendlyName, MediaType, Size | Format-Table -AutoSize | Out-String)",
               d->items[6].value, HW_VALUE_MAX);
}

void hw_show(void)
{
    static HwInfo d; /* ~28 KB: fora da pilha */
    hw_get_data(&d);

    for (int i = 0; i < HW_ITEMS; i++) {
        const char *label = d.items[i].label, *value = d.items[i].value;
        const char *cor_val = BRANCO;

        if (contains_ci(value, "indisponivel") || contains_ci(value, "nao disponivel")) cor_val = VERMELHO;
        else if (!strcmp(label, "CPU")) cor_val = AZUL_CELESTE;
        else if (!strcmp(label, "RAM instalada")) cor_val = VERDE;
        else if (!strcmp(label, "GPU")) cor_val = LARANJA;
        else if (!strcmp(label, "BIOS")) cor_val = AMARELO;

        if (!strcmp(label, "Discos fisicos"))
            printf("\n" AZUL "%-25s" BRANCO ":" RESET "\n" AZUL_CELESTE "%s" RESET "\n", label, value);
        else
            printf(AZUL "%-25s" BRANCO ":" RESET " %s%s" RESET "\n", label, cor_val, value);
    }
}
