/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_net.h
 * @brief Whether the Dot has an address, and a join from the card if it does not.
 */
#ifndef GROK_NET_H_
#define GROK_NET_H_

#include "grok_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief What grok_net_ensure() found. */
typedef enum {
    GROK_NET_UP = 0,  /**< The radio has an address. */
    GROK_NET_DOWN     /**< It does not.              */
} grok_net_status_t;

/**
 * @brief Join if @p cfg names a network and the radio has no address yet.
 *
 * Blocks. For a worker, not an Active Object. A remembered network is left
 * to CONFIG_KODEOS_WIFI_AUTOJOIN; this call is the card's ssid, if set.
 * The password is not logged.
 *
 * @param cfg The config. NULL only checks the current address.
 * @return GROK_NET_UP when the board has an address afterwards.
 */
grok_net_status_t grok_net_ensure(const grok_config_t *cfg);

/**
 * @brief Whether the board has an address now. Does not wait.
 * @return GROK_NET_UP or GROK_NET_DOWN.
 */
grok_net_status_t grok_net_now(void);

#ifdef __cplusplus
}
#endif

#endif /* GROK_NET_H_ */
