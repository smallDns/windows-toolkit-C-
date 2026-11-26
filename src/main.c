#include "platform.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "banner.h"
#include "color.h"
#include "cpu.h"
#include "diagnostics.h"
#include "disk.h"
#include "formatter.h"
#include "hardware.h"
#include "meminfo.h"
#include "network.h"
#include "processes.h"
#include "report.h"
#include "sysinfo.h"
#include "util.h"

#define VERSION "1.0.0"

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

typedef void (*Callback)(void);

static void clear_screen(void) { fflush(stdout); system("cls"); }

/* Equivalente a input(): le uma linha; encerra o programa em EOF (Ctrl+Z / pipe fechado). */
static void read_line(char *buf, size_t n)
{
    if (!fgets(buf, (int)n, stdin)) { putchar('\n'); exit(0); }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static void pause_screen(void)
{
    char tmp[64];
    printf("\nPressione ENTER para continuar...");
    fflush(stdout);
    read_line(tmp, sizeof tmp);
}

static void show(const char *title, Callback cb)
{
    clear_screen();
    heading(title);
    cb();
    pause_screen();
}

/* Aceita "1".."9", "01".."09", "0" e "00" (como o dicionario do main.py). */
static int parse_choice(const char *s)
{
    size_t n = strlen(s);
    if (n == 0 || n > 2) return -1;
    for (size_t i = 0; i < n; i++)
        if (!isdigit((unsigned char)s[i])) return -1;
    return atoi(s);
}

static void init_console(void)
{
    SetConsoleOutputCP(CP_UTF8); /* banner com caracteres de caixa/bloco */
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); /* cores ANSI no CMD */
}

int main(void)
{
    static const struct { const char *title; Callback cb; } actions[] = {
        {NULL, NULL},
        {"System Information",     sysinfo_show},
        {"CPU Monitor",            cpu_show},
        {"RAM Monitor",            mem_show},
        {"Disk Information",       disk_show},
        {"Network Information",    net_show},
        {"Process Manager",        proc_show},
        {"Hardware Information",   hw_show},
        {"System Diagnostics",     diag_show},
        {"Generate System Report", report_generate},
    };
    char line[64];

    init_console();

    for (;;) {
        clear_screen();
        banner(VERSION);
        printf("  " AZUL "[01]" RESET " " BRANCO "System Information" RESET "\n");
        printf("  " AZUL "[02]" RESET " " BRANCO "CPU Monitor" RESET "\n");
        printf("  " AZUL "[03]" RESET " " BRANCO "RAM Monitor" RESET "\n");
        printf("  " AZUL "[04]" RESET " " BRANCO "Disk Information" RESET "\n");
        printf("  " AZUL "[05]" RESET " " BRANCO "Network Information" RESET "\n");
        printf("  " LARANJA "[06]" RESET " " BRANCO "Process Manager" RESET "\n");
        printf("  " LARANJA "[07]" RESET " " BRANCO "Hardware Information" RESET "\n");
        printf("  " AMARELO "[08]" RESET " " BRANCO "System Diagnostics" RESET "\n");
        printf("  " VERDE "[09]" RESET " " BRANCO "Generate System Report" RESET "\n");
        printf("  " VERMELHO "[00]" RESET " " VERMELHO "Exit" RESET "\n");
        printf("\n  Select an option: ");
        fflush(stdout);

        read_line(line, sizeof line);
        str_trim(line);
        int choice = parse_choice(line);

        if (choice == 0) {
            printf("\nSaindo do Windows Toolkit...\n");
            break;
        }
        if (choice >= 1 && choice <= 9) {
            show(actions[choice].title, actions[choice].cb);
        } else {
            printf("\nOpcao invalida.\n");
            pause_screen();
        }
    }
    return 0;
}
