#ifndef TK_COLOR_H
#define TK_COLOR_H
/* Equivalente a utils/color.py (sequencias ANSI). Sao literais de string,
 * entao podem ser concatenados direto em formatos do printf:
 *   printf(AZUL "texto" RESET "\n");                                   */
#define AMARELO      "\033[38;5;226m"
#define AZULE        "\033[38;5;21m"
#define LARANJA      "\033[38;5;208m"
#define ROSA         "\033[38;5;201m"
#define ROXO         "\033[38;5;93m"
#define AZUL         "\033[38;5;39m"
#define VERDE        "\033[38;5;118m"
#define VERMELHO     "\033[38;5;196m"
#define BRANCO       "\033[38;5;255m"
#define RESET        "\033[0m"
#define LARANJA_RGB  "\033[38;2;255;165;0m"
#define AZUL_CELESTE "\033[38;2;0;191;255m"
#endif
