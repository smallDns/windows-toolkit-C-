#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "report.h"
#include "sysinfo.h"
#include "cpu.h"
#include "meminfo.h"
#include "disk.h"
#include "network.h"
#include "processes.h"
#include "hardware.h"
#include "diagnostics.h"
#include "json.h"
#include "color.h"
#include "util.h"

typedef struct {
    char generated_at[32];
    SysInfo sys;
    CpuInfo cpu;
    MemInfo mem;
    DiskInfo disks[DISK_MAX];
    int ndisks;
    NetData net;
    ProcInfo *procs;
    int nprocs;
    HwInfo hw;
    DiagResult diag[DIAG_MAX];
    int ndiag;
} ReportData;

/* Pasta "reports" ao lado do executavel (o Makefile gera o .exe na raiz do projeto). */
static void report_dir(char *out, size_t n)
{
    char exe[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe, (DWORD)sizeof exe);
    if (len == 0 || len >= sizeof exe) { copy_str(out, n, "reports"); return; }
    char *slash = strrchr(exe, '\\');
    if (slash) *slash = '\0';
    snprintf(out, n, "%s\\reports", exe);
}

static void collect(ReportData *r, const struct tm *now)
{
    strftime(r->generated_at, sizeof r->generated_at, "%Y-%m-%dT%H:%M:%S", now);
    sysinfo_get_data(&r->sys);
    cpu_get_data(&r->cpu);
    mem_get_data(&r->mem);
    r->ndisks = disk_get_data(r->disks, DISK_MAX);
    net_get_data(&r->net);
    r->nprocs = proc_get_data(&r->procs, 25);
    hw_get_data(&r->hw);
    r->ndiag = diag_run_checks(r->diag, DIAG_MAX);
}

static void release(ReportData *r)
{
    net_free(&r->net);
    free(r->procs);
    r->procs = NULL;
}

/* ---------------------------------------------------------------- JSON */
static void write_json(FILE *f, const ReportData *r)
{
    JsonWriter w;
    json_init(&w, f);
    json_begin_object(&w);

    json_key(&w, "generated_at"); json_string(&w, r->generated_at);

    json_key(&w, "system"); json_begin_object(&w);
      json_key(&w, "hostname");        json_string(&w, r->sys.hostname);
      json_key(&w, "username");        json_string(&w, r->sys.username);
      json_key(&w, "system");          json_string(&w, r->sys.system);
      json_key(&w, "windows_release"); json_string(&w, r->sys.windows_release);
      json_key(&w, "version");         json_string(&w, r->sys.version);
      json_key(&w, "architecture");    json_string(&w, r->sys.architecture);
      json_key(&w, "runtime");         json_string(&w, r->sys.runtime);
      json_key(&w, "boot_time");       json_string(&w, r->sys.boot_time);
    json_end_object(&w);

    json_key(&w, "cpu"); json_begin_object(&w);
      json_key(&w, "model"); json_string(&w, r->cpu.model);
      json_key(&w, "physical_cores");
      if (r->cpu.physical_cores > 0) json_int(&w, r->cpu.physical_cores); else json_string(&w, "N/A");
      json_key(&w, "logical_cores");
      if (r->cpu.logical_cores > 0) json_int(&w, r->cpu.logical_cores); else json_string(&w, "N/A");
      json_key(&w, "usage_percent"); json_double(&w, r->cpu.usage_percent, 1);
      json_key(&w, "frequency_mhz");
      if (r->cpu.frequency_mhz > 0.0) json_double(&w, r->cpu.frequency_mhz, 1); else json_null(&w);
    json_end_object(&w);

    json_key(&w, "memory"); json_begin_object(&w);
      json_key(&w, "total");        json_uint(&w, r->mem.total);
      json_key(&w, "used");         json_uint(&w, r->mem.used);
      json_key(&w, "available");    json_uint(&w, r->mem.available);
      json_key(&w, "percent");      json_double(&w, r->mem.percent, 1);
      json_key(&w, "swap_total");   json_uint(&w, r->mem.swap_total);
      json_key(&w, "swap_used");    json_uint(&w, r->mem.swap_used);
      json_key(&w, "swap_percent"); json_double(&w, r->mem.swap_percent, 1);
    json_end_object(&w);

    json_key(&w, "disks"); json_begin_array(&w);
    for (int i = 0; i < r->ndisks; i++) {
        const DiskInfo *d = &r->disks[i];
        json_begin_object(&w);
          json_key(&w, "device");     json_string(&w, d->device);
          json_key(&w, "mountpoint"); json_string(&w, d->mountpoint);
          json_key(&w, "filesystem"); json_string(&w, d->filesystem);
          json_key(&w, "total");      json_uint(&w, d->total);
          json_key(&w, "used");       json_uint(&w, d->used);
          json_key(&w, "free");       json_uint(&w, d->free);
          json_key(&w, "percent");    json_double(&w, d->percent, 1);
        json_end_object(&w);
    }
    json_end_array(&w);

    json_key(&w, "network"); json_begin_object(&w);
      json_key(&w, "adapters"); json_begin_array(&w);
      for (int i = 0; i < r->net.nadapters; i++) {
          const NetAdapter *a = &r->net.adapters[i];
          json_begin_object(&w);
            json_key(&w, "name"); json_string(&w, a->name);
            json_key(&w, "addresses"); json_begin_array(&w);
            for (int j = 0; j < a->naddrs; j++) {
                json_begin_object(&w);
                  json_key(&w, "family");  json_string(&w, a->addrs[j].family);
                  json_key(&w, "address"); json_string(&w, a->addrs[j].address);
                json_end_object(&w);
            }
            json_end_array(&w);
            json_key(&w, "is_up"); json_bool(&w, a->is_up);
          json_end_object(&w);
      }
      json_end_array(&w);
      json_key(&w, "bytes_sent");       json_uint(&w, r->net.bytes_sent);
      json_key(&w, "bytes_received");   json_uint(&w, r->net.bytes_received);
      json_key(&w, "packets_sent");     json_uint(&w, r->net.packets_sent);
      json_key(&w, "packets_received"); json_uint(&w, r->net.packets_received);
    json_end_object(&w);

    json_key(&w, "processes"); json_begin_array(&w);
    for (int i = 0; i < r->nprocs; i++) {
        json_begin_object(&w);
          json_key(&w, "pid");      json_uint(&w, r->procs[i].pid);
          json_key(&w, "name");     json_string(&w, r->procs[i].name);
          json_key(&w, "username"); json_string(&w, r->procs[i].username);
          json_key(&w, "memory");   json_uint(&w, r->procs[i].memory);
        json_end_object(&w);
    }
    json_end_array(&w);

    json_key(&w, "hardware"); json_begin_object(&w);
    for (int i = 0; i < HW_ITEMS; i++) {
        json_key(&w, r->hw.items[i].label);
        json_string(&w, r->hw.items[i].value);
    }
    json_end_object(&w);

    json_key(&w, "diagnostics"); json_begin_array(&w);
    for (int i = 0; i < r->ndiag; i++) {
        json_begin_object(&w);
          json_key(&w, "name");   json_string(&w, r->diag[i].name);
          json_key(&w, "detail"); json_string(&w, r->diag[i].detail);
          json_key(&w, "ok");     json_bool(&w, r->diag[i].ok);
        json_end_object(&w);
    }
    json_end_array(&w);

    json_end_object(&w);
    fputc('\n', f);
}

/* ----------------------------------------------------------------- TXT */
static void write_txt(FILE *f, const ReportData *r)
{
    char t1[40], t2[40], t3[40], t4[40];

    fprintf(f, "WINDOWS TOOLKIT - SYSTEM REPORT\nGenerated: %s\n", r->generated_at);
    for (int i = 0; i < 55; i++) fputc('=', f);
    fputc('\n', f);

    fprintf(f, "\n[SYSTEM]\n");
    fprintf(f, "hostname: %s\nusername: %s\nsystem: %s\nwindows_release: %s\n", r->sys.hostname,
            r->sys.username, r->sys.system, r->sys.windows_release);
    fprintf(f, "version: %s\narchitecture: %s\nruntime: %s\nboot_time: %s\n", r->sys.version,
            r->sys.architecture, r->sys.runtime, r->sys.boot_time);

    fprintf(f, "\n[CPU]\n");
    fprintf(f, "model: %s\n", r->cpu.model);
    if (r->cpu.physical_cores > 0) fprintf(f, "physical_cores: %d\n", r->cpu.physical_cores);
    else fprintf(f, "physical_cores: N/A\n");
    if (r->cpu.logical_cores > 0) fprintf(f, "logical_cores: %d\n", r->cpu.logical_cores);
    else fprintf(f, "logical_cores: N/A\n");
    fprintf(f, "usage_percent: %.1f\n", r->cpu.usage_percent);
    if (r->cpu.frequency_mhz > 0.0) fprintf(f, "frequency_mhz: %.1f\n", r->cpu.frequency_mhz);
    else fprintf(f, "frequency_mhz: None\n");

    fprintf(f, "\n[MEMORY]\n");
    fprintf(f, "total: %llu\nused: %llu\navailable: %llu\npercent: %.1f\n", r->mem.total, r->mem.used,
            r->mem.available, r->mem.percent);
    fprintf(f, "swap_total: %llu\nswap_used: %llu\nswap_percent: %.1f\n", r->mem.swap_total,
            r->mem.swap_used, r->mem.swap_percent);

    fprintf(f, "\n[DISKS]\n");
    for (int i = 0; i < r->ndisks; i++) {
        const DiskInfo *d = &r->disks[i];
        fprintf(f, "%s [%s] total=%llu used=%llu free=%llu percent=%.1f\n", d->device, d->filesystem,
                d->total, d->used, d->free, d->percent);
    }

    fprintf(f, "\n[NETWORK]\n");
    for (int i = 0; i < r->net.nadapters; i++) {
        const NetAdapter *a = &r->net.adapters[i];
        fprintf(f, "%s (%s)\n", a->name, a->is_up ? "Ativo" : "Desativado");
        for (int j = 0; j < a->naddrs; j++) fprintf(f, "  %s: %s\n", a->addrs[j].family, a->addrs[j].address);
    }
    fmt_thousands(r->net.bytes_sent, t1, sizeof t1);
    fmt_thousands(r->net.bytes_received, t2, sizeof t2);
    fmt_thousands(r->net.packets_sent, t3, sizeof t3);
    fmt_thousands(r->net.packets_received, t4, sizeof t4);
    fprintf(f, "bytes_sent: %s\nbytes_received: %s\npackets_sent: %s\npackets_received: %s\n", t1, t2, t3, t4);

    fprintf(f, "\n[PROCESSES]\n");
    for (int i = 0; i < r->nprocs; i++)
        fprintf(f, "pid=%lu name=%s username=%s memory=%llu\n", r->procs[i].pid, r->procs[i].name,
                r->procs[i].username, r->procs[i].memory);

    fprintf(f, "\n[HARDWARE]\n");
    for (int i = 0; i < HW_ITEMS; i++) fprintf(f, "%s: %s\n", r->hw.items[i].label, r->hw.items[i].value);

    fprintf(f, "\n[DIAGNOSTICS]\n");
    for (int i = 0; i < r->ndiag; i++)
        fprintf(f, "%s %s: %s\n", r->diag[i].ok ? "[OK]" : "[AVISO]", r->diag[i].name, r->diag[i].detail);
}

/* -------------------------------------------------------------- driver */
void report_generate(void)
{
    printf(AMARELO "Coletando dados do sistema e gerando relatorios..." RESET "\n");

    ReportData *r = (ReportData *)calloc(1, sizeof *r); /* struct grande: heap, nao pilha */
    if (!r) { printf(VERMELHO "Memoria insuficiente." RESET "\n"); return; }

    time_t now_t = time(NULL);
    struct tm now = *localtime(&now_t);
    collect(r, &now);

    char dir[MAX_PATH + 16], stamp[32], json_path[MAX_PATH + 80], txt_path[MAX_PATH + 80];
    report_dir(dir, sizeof dir);
    CreateDirectoryA(dir, NULL); /* ja existir nao e erro */
    strftime(stamp, sizeof stamp, "%Y%m%d_%H%M%S", &now);
    snprintf(json_path, sizeof json_path, "%s\\system_report_%s.json", dir, stamp);
    snprintf(txt_path, sizeof txt_path, "%s\\system_report_%s.txt", dir, stamp);

    FILE *fj = fopen(json_path, "w");
    FILE *ft = fopen(txt_path, "w");
    if (!fj || !ft) {
        printf(VERMELHO "Nao foi possivel criar os arquivos em: %s" RESET "\n", dir);
        if (fj) fclose(fj);
        if (ft) fclose(ft);
        release(r);
        free(r);
        return;
    }
    write_json(fj, r);
    write_txt(ft, r);
    fclose(fj);
    fclose(ft);

    printf("\n" VERDE "Relatorios gerados com sucesso:" RESET "\n");
    printf("  " AZUL "TXT " BRANCO ":" RESET " " AZUL_CELESTE "%s" RESET "\n", txt_path);
    printf("  " AZUL "JSON" BRANCO ":" RESET " " LARANJA "%s" RESET "\n", json_path);

    release(r);
    free(r);
}
