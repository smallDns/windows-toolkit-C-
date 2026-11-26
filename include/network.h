#ifndef TK_NETWORK_H
#define TK_NETWORK_H
typedef struct { char family[5]; char address[64]; } NetAddr;
typedef struct {
    char    name[256];
    NetAddr *addrs;
    int     naddrs;
    int     is_up;
} NetAdapter;
typedef struct {
    NetAdapter *adapters;
    int nadapters;
    unsigned long long bytes_sent, bytes_received, packets_sent, packets_received;
} NetData;
void net_get_data(NetData *d);
void net_free(NetData *d);
void net_show(void);
#endif
