#include "platform.h"
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>
#include <string.h>
#include "diagnostics.h"
#include "cpu.h"
#include "color.h"
#include "util.h"

#define GIB (1024.0 * 1024.0 * 1024.0)

static void add(DiagResult *out, int *n, int max, const char *name, const char *detail, int ok)
{
    if (*n >= max) return;
    copy_str(out[*n].name, sizeof out[*n].name, name);
    copy_str(out[*n].detail, sizeof out[*n].detail, detail);
    out[*n].ok = ok;
    (*n)++;
}

/* Ping ICMP real via IcmpSendEcho (sem processo externo). -1 = nao foi possivel executar. */
static int ping_host(const char *ip, DWORD timeout_ms)
{
    IPAddr addr = 0;
    if (InetPtonA(AF_INET, ip, &addr) != 1) return -1;

    HANDLE h = IcmpCreateFile();
    if (h == INVALID_HANDLE_VALUE) return -1;

    char payload[32] = "windows-toolkit";
    union { /* uniao garante o alinhamento correto do ICMP_ECHO_REPLY */
        ICMP_ECHO_REPLY r;
        char raw[sizeof(ICMP_ECHO_REPLY) + sizeof payload + 8 + 64];
    } reply;
    DWORD got = IcmpSendEcho(h, addr, payload, (WORD)sizeof payload, NULL, &reply, (DWORD)sizeof reply, timeout_ms);
    int ok = (got > 0 && reply.r.Status == IP_SUCCESS);
    IcmpCloseHandle(h);
    return ok;
}

int diag_run_checks(DiagResult *out, int max)
{
    int n = 0;
    char buf[128];

    add(out, &n, max, "Sistema", "Windows", 1);

    ULARGE_INTEGER avail, total, freeb;
    if (GetDiskFreeSpaceExA(NULL, &avail, &total, &freeb)) { /* NULL = unidade atual */
        snprintf(buf, sizeof buf, "%.2f GB", (double)avail.QuadPart / GIB);
        add(out, &n, max, "Espaco livre na unidade atual", buf, (double)avail.QuadPart > 2.0 * GIB);
    } else {
        add(out, &n, max, "Espaco livre na unidade atual", "Indisponivel", 0);
    }

    MEMORYSTATUSEX m;
    memset(&m, 0, sizeof m);
    m.dwLength = sizeof m;
    if (GlobalMemoryStatusEx(&m)) {
        snprintf(buf, sizeof buf, "%.2f GB", (double)m.ullAvailPhys / GIB);
        add(out, &n, max, "RAM disponivel", buf, m.ullAvailPhys > 512ULL * 1024 * 1024);
    } else {
        add(out, &n, max, "RAM disponivel", "Indisponivel", 0);
    }

    snprintf(buf, sizeof buf, "%.1f%%", cpu_usage_percent(500));
    add(out, &n, max, "CPU usage", buf, 1);

    WSADATA wsa;
    int dns_ok = 0;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) {
        dns_ok = (gethostbyname("www.microsoft.com") != NULL);
        WSACleanup();
    }
    if (dns_ok) add(out, &n, max, "DNS/Internet (resolucao)", "OK: www.microsoft.com", 1);
    else        add(out, &n, max, "DNS/Internet (resolucao)", "Falha na resolucao DNS", 0);

    int p = ping_host("8.8.8.8", 1500);
    if (p < 0)       add(out, &n, max, "Ping para 8.8.8.8", "Nao foi possivel executar", 0);
    else if (p == 1) add(out, &n, max, "Ping para 8.8.8.8", "Resposta", 1);
    else             add(out, &n, max, "Ping para 8.8.8.8", "Sem resposta", 0);

    return n;
}

void diag_show(void)
{
    DiagResult r[DIAG_MAX];
    int n = diag_run_checks(r, DIAG_MAX);

    for (int i = 0; i < n; i++) {
        const char *cor_status = r[i].ok ? VERDE : VERMELHO;
        const char *cor_detail = r[i].ok ? AZUL_CELESTE : AMARELO;
        printf("%s%-9s" RESET " " AZUL "%-32s" RESET " %s%s" RESET "\n",
               cor_status, r[i].ok ? "[OK]" : "[AVISO]", r[i].name, cor_detail, r[i].detail);
    }
    printf("\n" LARANJA "Diagnosticos simples. Resultados podem variar conforme firewall/rede." RESET "\n");
}
