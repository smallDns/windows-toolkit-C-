#include <stdio.h>
#include "banner.h"
#include "color.h"

void banner(const char *version)
{
    /* Arte ASCII identica a utils/banner.py (arquivo em UTF-8). */
    printf(AZUL_CELESTE
        "                                                   \n"
        "                  ░██                  ░██                                         \n"
        "                                       ░██                                         \n"
        "░██    ░██    ░██ ░██░████████   ░████████  ░███████  ░██    ░██    ░██  ░███████  \n"
        "░██    ░██    ░██ ░██░██    ░██ ░██    ░██ ░██    ░██ ░██    ░██    ░██ ░██        \n"
        " ░██  ░████  ░██  ░██░██    ░██ ░██    ░██ ░██    ░██  ░██  ░████  ░██   ░███████  \n"
        "  ░██░██ ░██░██   ░██░██    ░██ ░██   ░███ ░██    ░██   ░██░██ ░██░██          ░██ \n"
        "   ░███   ░███    ░██░██    ░██  ░█████░██  ░███████     ░███   ░███     ░███████  \n"
        RESET "\n");
    printf(AZUL "╔════════════════════════════════════════════════════════════╗" RESET "\n");
    printf(AZUL "║" RESET "  " BRANCO "Windows Toolkit" RESET " " AMARELO "v%-10s" RESET " " AZUL "|" RESET
           " " LARANJA "%-28s" RESET AZUL "║" RESET "\n", version, "C (Win32 API)");
    printf(AZUL "╚════════════════════════════════════════════════════════════╝" RESET "\n");
}
