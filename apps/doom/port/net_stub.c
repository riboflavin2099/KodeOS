#include "config.h"

#include "doomtype.h"
#include "net_defs.h"

static boolean null_init(void)
{
    return false;
}

static void null_send(net_addr_t *addr, net_packet_t *packet)
{
    (void)addr;
    (void)packet;
}

static boolean null_recv(net_addr_t **addr, net_packet_t **packet)
{
    (void)addr;
    (void)packet;
    return false;
}

static void null_string(net_addr_t *addr, char *buffer, int buffer_len)
{
    (void)addr;
    if ( (buffer != NULL) && (buffer_len > 0) )
    {
        buffer[0] = '\0';
    }
}

static void null_free(net_addr_t *addr)
{
    (void)addr;
}

static net_addr_t *null_resolve(const char *address)
{
    (void)address;
    return NULL;
}

net_module_t net_sdl_module = {
    null_init,
    null_init,
    null_send,
    null_recv,
    null_string,
    null_free,
    null_resolve,
};

void NET_WaitForLaunch(void)
{
}
