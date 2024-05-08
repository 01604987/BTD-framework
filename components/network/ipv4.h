#ifndef __IPV4__
#define __IPV4__


#include "sdkconfig.h"
#include <stddef.h>

#if defined(CONFIG_NETWORK_SOCKET_IP_INPUT_STDIN)
#include "addr_from_stdin.h"
#endif

#if defined(CONFIG_NETWORK_IPV4)
#define HOST_IP_ADDR CONFIG_NETWORK_IPV4_ADDR
#elif defined(CONFIG_NETWORK_SOCKET_IP_INPUT_STDIN)
#define HOST_IP_ADDR ""
#endif

#define PORT CONFIG_NETWORK_PORT


void init_network();
int connect_to_sock();
void send_buf(const void *dataptr, size_t size);
char* recv_buf();
void shutdown_conn();
void close_sock();
int check_conn();

#endif /* __IPV4__ */
