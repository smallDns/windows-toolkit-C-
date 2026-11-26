#include "platform.h"
#include <stdio.h>
#include <string.h>
#include "meminfo.h"
#include "color.h"
#include "formatter.h"
#include "util.h"

void mem_get_data(MemInfo *d)
{
    MEMORYSTATUSEX m;
    memset(d, 0, sizeof *d);
    memset(&m, 0, sizeof m);
    m.dwLength = sizeof m;
    if (!GlobalMemoryStatusEx(&m)) return;

    d->total = m.ullTotalPhys;
    d->available = m.ullAvailPhys;
    d->used = d->total - d->available;
    d->percent = d->total ? round1((double)d->used * 100.0 / (double)d->total) : 0.0;

    /* Mesma logica do psutil no Windows: swap = limite de commit - RAM fisica. */
    d->swap_total = m.ullTotalPageFile > m.ullTotalPhys ? m.ullTotalPageFile - m.ullTotalPhys : 0;
    unsigned long long swap_avail = m.ullAvailPageFile > m.ullAvailPhys ? m.ullAvailPageFile - m.ullAvailPhys : 0;
    if (swap_avail > d->swap_total) swap_avail = d->swap_total;
    d->swap_used = d->swap_total - swap_avail;
    d->swap_percent = d->swap_total ? round1((double)d->swap_used * 100.0 / (double)d->swap_total) : 0.0;
}

static const char *cor_por_uso(double p)
{
    return p < 60.0 ? VERDE : (p < 85.0 ? AMARELO : VERMELHO);
}

void mem_show(void)
{
    MemInfo d;
    char total[32], used[32], avail[32], bar[64], stotal[32], sused[32];

    mem_get_data(&d);
    const char *cor_ram = cor_por_uso(d.percent);
    const char *cor_swap = cor_por_uso(d.swap_percent);

    format_bytes((double)d.total, total, sizeof total);
    format_bytes((double)d.used, used, sizeof used);
    format_bytes((double)d.available, avail, sizeof avail);
    format_bytes((double)d.swap_total, stotal, sizeof stotal);
    format_bytes((double)d.swap_used, sused, sizeof sused);
    percent_bar(d.percent, bar, sizeof bar);

    printf(AZUL "RAM total       " BRANCO ":" RESET " " BRANCO "%s" RESET "\n", total);
    printf(AZUL "RAM utilizada   " BRANCO ":" RESET " %s%s (%.1f%%)" RESET "\n", cor_ram, used, d.percent);
    printf(AZUL "RAM disponivel  " BRANCO ":" RESET " " VERDE "%s" RESET "\n", avail);
    printf(AZUL "Uso             " BRANCO ":" RESET " %s%s" RESET "\n", cor_ram, bar);
    printf("\n" LARANJA "Memoria virtual " BRANCO ":" RESET " %s%s" RESET " " BRANCO "/" RESET
           " " BRANCO "%s" RESET " %s(%.1f%%)" RESET "\n",
           cor_swap, sused, stotal, cor_swap, d.swap_percent);
}
