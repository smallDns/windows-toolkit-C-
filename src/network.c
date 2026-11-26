#include "platform.h"
#include <iphlpapi.h>
#include <netioapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "network.h"
#include "color.h"
#include "util.h"

static void add_addr(NetAdapter *a, const char *family, const char *address)
{
    NetAddr *p = (NetAddr *)realloc(a->addrs, (size_t)(a->naddrs + 1) * sizeof *p);
    if (!p) return;
    a->addrs = p;
    copy_str(p[a->naddrs].family, sizeof p[a->naddrs].family, family);
    copy_str(p[a->naddrs].address, sizeof p[a->naddrs].address, address);
    a->naddrs++;
}

static void collect_adapters(NetData *d)
{
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 16 * 1024;
    IP_ADAPTER_ADDRESSES *buf = NULL;
    ULONG rc = ERROR_BUFFER_OVERFLOW;

    for (int tries = 0; tries < 4 && rc == ERROR_BUFFER_OVERFLOW; tries++) {
        free(buf);
        buf = (IP_ADAPTER_ADDRESSES *)malloc(size);
        if (!buf) return;
        rc = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, buf, &size);
    }
    if (rc != NO_ERROR) { free(buf); return; }

    for (IP_ADAPTER_ADDRESSES *a = buf; a; a = a->Next) {
        NetAdapter *arr = (NetAdapter *)realloc(d->adapters, (size_t)(d->nadapters + 1) * sizeof *arr);
        if (!arr) break;
        d->adapters = arr;
        NetAdapter *cur = &d->adapters[d->nadapters++];
        memset(cur, 0, sizeof *cur);
        wide_to_utf8(a->FriendlyName, cur->name, sizeof cur->name);
        cur->is_up = (a->OperStatus == IfOperStatusUp);

        for (IP_ADAPTER_UNICAST_ADDRESS *u = a->FirstUnicastAddress; u; u = u->Next) {
            SOCKADDR *sa = u->Address.lpSockaddr;
            char text[64] = "";
            if (!sa) continue;
            if (sa->sa_family == AF_INET) {
                InetNtopA(AF_INET, &((SOCKADDR_IN *)sa)->sin_addr, text, sizeof text);
                add_addr(cur, "IPv4", text);
            } else if (sa->sa_family == AF_INET6) {
                InetNtopA(AF_INET6, &((SOCKADDR_IN6 *)sa)->sin6_addr, text, sizeof text);
                add_addr(cur, "IPv6", text);
            }
        }
    }
    free(buf);
}

static void collect_counters(NetData *d)
{
    PMIB_IF_TABLE2 t = NULL;
    if (GetIfTable2(&t) != NO_ERROR || !t) return;
    for (ULONG i = 0; i < t->NumEntries; i++) {
        const MIB_IF_ROW2 *r = &t->Table[i];
        d->bytes_sent += r->OutOctets;
        d->bytes_received += r->InOctets;
        d->packets_sent += r->OutUcastPkts + r->OutNUcastPkts;
        d->packets_received += r->InUcastPkts + r->InNUcastPkts;
    }
    FreeMibTable(t);
}

void net_get_data(NetData *d)
{
    memset(d, 0, sizeof *d);
    collect_adapters(d);
    collect_counters(d);
}

void net_free(NetData *d)
{
    for (int i = 0; i < d->nadapters; i++) free(d->adapters[i].addrs);
    free(d->adapters);
    memset(d, 0, sizeof *d);
}

void net_show(void)
{
    NetData d;
    char s1[40], s2[40], s3[40], s4[40];

    net_get_data(&d);
    printf(AMARELO "Adaptadores:" RESET "\n");
    for (int i = 0; i < d.nadapters; i++) {
        const NetAdapter *a = &d.adapters[i];
        printf("\n " AZUL "-" RESET " " AZUL_CELESTE "%s" RESET " (%s%s" RESET ")\n",
               a->name, a->is_up ? VERDE : VERMELHO, a->is_up ? "Ativo" : "Desativado");
        for (int j = 0; j < a->naddrs; j++) {
            const char *cor_fam = strcmp(a->addrs[j].family, "IPv4") == 0 ? LARANJA : ROXO;
            printf("    %s%s" BRANCO ":" RESET " " BRANCO "%s" RESET "\n",
                   cor_fam, a->addrs[j].family, a->addrs[j].address);
        }
    }

    fmt_thousands(d.bytes_sent, s1, sizeof s1);
    fmt_thousands(d.bytes_received, s2, sizeof s2);
    fmt_thousands(d.packets_sent, s3, sizeof s3);
    fmt_thousands(d.packets_received, s4, sizeof s4);

    printf("\n" AZUL "Bytes enviados   " BRANCO ":" RESET " " VERDE "%s" RESET "\n", s1);
    printf(AZUL "Bytes recebidos  " BRANCO ":" RESET " " AZUL_CELESTE "%s" RESET "\n", s2);
    printf(AZUL "Pacotes enviados " BRANCO ":" RESET " " LARANJA "%s" RESET "\n", s3);
    printf(AZUL "Pacotes recebidos" BRANCO ":" RESET " " AMARELO "%s" RESET "\n", s4);
    net_free(&d);
}
