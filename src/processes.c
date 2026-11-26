#include "platform.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "processes.h"
#include "color.h"
#include "util.h"

static int cmp_mem_desc(const void *pa, const void *pb)
{
    const ProcInfo *a = (const ProcInfo *)pa, *b = (const ProcInfo *)pb;
    if (a->memory != b->memory) return a->memory < b->memory ? 1 : -1;
    return a->order < b->order ? -1 : (a->order > b->order ? 1 : 0); /* desempate estavel */
}

static unsigned long long working_set(DWORD pid)
{
    unsigned long long ws = 0;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return 0;
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(h, &pmc, sizeof pmc)) ws = pmc.WorkingSetSize;
    CloseHandle(h);
    return ws;
}

static void owner_of(DWORD pid, char *out, size_t n)
{
    copy_str(out, n, "?");
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return;

    HANDLE tok = NULL;
    if (OpenProcessToken(h, TOKEN_QUERY, &tok)) {
        DWORD len = 0;
        GetTokenInformation(tok, TokenUser, NULL, 0, &len);
        TOKEN_USER *tu = len ? (TOKEN_USER *)malloc(len) : NULL;
        if (tu && GetTokenInformation(tok, TokenUser, tu, len, &len)) {
            wchar_t user[256], dom[256];
            DWORD ul = 256, dl = 256;
            SID_NAME_USE use;
            if (LookupAccountSidW(NULL, tu->User.Sid, user, &ul, dom, &dl, &use)) {
                wchar_t full[520]; /* dominio(<=255) + '\\' + usuario(<=255) */
                full[0] = L'\0';
                if (dl > 0) { wcscat(full, dom); wcscat(full, L"\\"); }
                wcscat(full, user);
                wide_to_utf8(full, out, n);
            }
        }
        free(tu);
        CloseHandle(tok);
    }
    CloseHandle(h);
}

int proc_get_data(ProcInfo **out, int limit)
{
    *out = NULL;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    int cap = 256, n = 0;
    ProcInfo *a = (ProcInfo *)malloc((size_t)cap * sizeof *a);
    if (!a) { CloseHandle(snap); return 0; }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof pe;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (n == cap) {
                ProcInfo *tmp = (ProcInfo *)realloc(a, (size_t)cap * 2 * sizeof *a);
                if (!tmp) break;
                a = tmp;
                cap *= 2;
            }
            ProcInfo *p = &a[n];
            memset(p, 0, sizeof *p);
            p->pid = pe.th32ProcessID;
            wide_to_utf8(pe.szExeFile, p->name, sizeof p->name);
            if (!p->name[0]) copy_str(p->name, sizeof p->name, "?");
            copy_str(p->username, sizeof p->username, "?");
            p->memory = working_set(pe.th32ProcessID); /* sem acesso => 0 (como no original) */
            p->order = (unsigned)n;
            n++;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    qsort(a, (size_t)n, sizeof *a, cmp_mem_desc);
    if (limit > 0 && n > limit) n = limit;

    /* O nome de usuario so e necessario para os processos que serao exibidos. */
    for (int i = 0; i < n; i++) owner_of((DWORD)a[i].pid, a[i].username, sizeof a[i].username);

    *out = a;
    return n;
}

void proc_show(void)
{
    ProcInfo *list = NULL;
    int n = proc_get_data(&list, 25);

    printf(AZUL "%7s  %10s  %s" RESET "\n", "PID", "RAM (MB)", "Processo");
    printf(AZUL);
    for (int i = 0; i < 48; i++) putchar('-');
    printf(RESET "\n");

    for (int i = 0; i < n; i++) {
        double ram_mb = (double)list[i].memory / 1024.0 / 1024.0;
        const char *cor = ram_mb >= 500.0 ? VERMELHO : (ram_mb >= 150.0 ? AMARELO : VERDE);
        char name[512];
        copy_str(name, sizeof name, list[i].name);
        utf8_truncate(name, 32);
        printf(AZUL_CELESTE "%7lu" RESET "  %s%10.1f" RESET "  " BRANCO "%s" RESET "\n",
               list[i].pid, cor, ram_mb, name);
    }

    printf("\n" LARANJA "Lista ordenada por consumo de RAM. Consulta somente; nao encerra processos." RESET "\n");
    free(list);
}
