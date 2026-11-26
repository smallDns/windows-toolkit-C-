#include "platform.h"
#include <stdio.h>
#include <string.h>
#include "disk.h"
#include "color.h"
#include "formatter.h"
#include "util.h"

int disk_get_data(DiskInfo *out, int max)
{
    int n = 0;
    UINT old_mode = SetErrorMode(SEM_FAILCRITICALERRORS); /* sem dialogo "insira um disco" */
    DWORD mask = GetLogicalDrives();

    for (int i = 0; i < 26 && n < max; i++) {
        if (!(mask & (1UL << i))) continue;

        char root[4] = {(char)('A' + i), ':', '\\', '\0'};
        UINT type = GetDriveTypeA(root);
        if (type == DRIVE_UNKNOWN || type == DRIVE_NO_ROOT_DIR) continue;

        char fs[32] = "";
        if (!GetVolumeInformationA(root, NULL, 0, NULL, NULL, NULL, fs, sizeof fs)) continue;

        ULARGE_INTEGER avail, total, freeb;
        if (!GetDiskFreeSpaceExA(root, &avail, &total, &freeb)) continue;

        DiskInfo *d = &out[n++];
        copy_str(d->device, sizeof d->device, root);
        copy_str(d->mountpoint, sizeof d->mountpoint, root);
        copy_str(d->filesystem, sizeof d->filesystem, fs);
        d->total = total.QuadPart;
        d->free = avail.QuadPart;
        d->used = d->total > d->free ? d->total - d->free : 0;
        d->percent = d->total ? round1((double)d->used * 100.0 / (double)d->total) : 0.0;
    }
    SetErrorMode(old_mode);
    return n;
}

void disk_show(void)
{
    DiskInfo rows[DISK_MAX];
    int n = disk_get_data(rows, DISK_MAX);

    if (n == 0) printf(VERMELHO "Nenhuma unidade acessivel encontrada." RESET "\n");

    for (int i = 0; i < n; i++) {
        const DiskInfo *d = &rows[i];
        char total[32], used[32], freeb[32];
        const char *cor_uso = d->percent < 70.0 ? VERDE : (d->percent < 90.0 ? AMARELO : VERMELHO);

        format_bytes((double)d->total, total, sizeof total);
        format_bytes((double)d->used, used, sizeof used);
        format_bytes((double)d->free, freeb, sizeof freeb);

        printf("\n" AZUL_CELESTE "%s" RESET "  " LARANJA "[%s]" RESET "\n", d->device, d->filesystem);
        printf("  " AZUL "Montagem " BRANCO ":" RESET " " BRANCO "%s" RESET "\n", d->mountpoint);
        printf("  " AZUL "Total    " BRANCO ":" RESET " " BRANCO "%s" RESET "\n", total);
        printf("  " AZUL "Usado    " BRANCO ":" RESET " %s%s (%.1f%%)" RESET "\n", cor_uso, used, d->percent);
        printf("  " AZUL "Livre    " BRANCO ":" RESET " " VERDE "%s" RESET "\n", freeb);
    }
}
