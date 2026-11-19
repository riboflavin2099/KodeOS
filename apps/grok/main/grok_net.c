/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_net.c
 * @brief Wi-Fi through kode_wifi_*. The C5 link is AO_Radio's.
 */
#include "grok_net.h"

#include "kode.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "grok.net"

grok_net_status_t grok_net_now(void)
{
    if ( kode_wifi_is_connected() )
    {
        return GROK_NET_UP;
    }
    return GROK_NET_DOWN;
}

grok_net_status_t grok_net_ensure(const grok_config_t *cfg)
{
    const char *pass;

    if ( kode_wifi_is_connected() )
    {
        return GROK_NET_UP;
    }
    if ( (NULL == cfg) || ('\0' == cfg->ssid[0]) )
    {
        KLOG_I(TAG, "no address, and the card names no network");
        return GROK_NET_DOWN;
    }
    pass = ('\0' == cfg->password[0]) ? NULL : cfg->password;
    KLOG_I(TAG, "joining %s", cfg->ssid);
    if ( kode_wifi_connect(cfg->ssid, pass) )
    {
        return GROK_NET_UP;
    }
    KLOG_W(TAG, "join failed: %s", kode_error());
    return GROK_NET_DOWN;
}
