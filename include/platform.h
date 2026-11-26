#ifndef TK_PLATFORM_H
#define TK_PLATFORM_H
/* Inclua este header ANTES de qualquer outro header do Windows. */
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 /* Windows 10 */
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wchar.h>
#include <stddef.h>

/* UTF-16 -> UTF-8 com truncamento seguro. */
static inline void wide_to_utf8(const wchar_t *w, char *out, size_t outsz)
{
    if (outsz == 0) return;
    out[0] = '\0';
    if (!w) return;
    size_t wl = wcslen(w);
    if (wl > (outsz - 1) / 3) wl = (outsz - 1) / 3; /* UTF-8: <= 3 bytes por wchar */
    if (wl == 0) return;
    int n = WideCharToMultiByte(CP_UTF8, 0, w, (int)wl, out, (int)outsz - 1, NULL, NULL);
    out[n > 0 ? n : 0] = '\0';
}
#endif
