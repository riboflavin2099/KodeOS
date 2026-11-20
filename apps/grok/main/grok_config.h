/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_config.h
 * @brief The text file on the card: where to ask, which model, and the key.
 *
 * No SDK calls. The firmware reads the file; this parses the bytes.
 */
#ifndef GROK_CONFIG_H_
#define GROK_CONFIG_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Room for one field ---- */

/** @brief Bytes of a base URL, NUL included. */
#define GROK_CFG_URL_MAX     160U

/** @brief Bytes of a path, NUL included. It starts with '/'. */
#define GROK_CFG_PATH_MAX     48U

/** @brief Bytes of a model name, NUL included. */
#define GROK_CFG_MODEL_MAX    48U

/** @brief Bytes of an API key, NUL included. Never log this field. */
#define GROK_CFG_KEY_MAX     180U

/** @brief Bytes of the system prompt, NUL included. */
#define GROK_CFG_SYSTEM_MAX  240U

/** @brief Bytes of a Wi-Fi name, NUL included. Matches KODE_WIFI_SSID_MAX. */
#define GROK_CFG_SSID_MAX     33U

/** @brief Bytes of a Wi-Fi password, NUL included. Never log this field. */
#define GROK_CFG_PASS_MAX     65U

/* ---- The file ---- */

/**
 * @brief What sd/grok/config.txt said, after defaults filled the gaps.
 *
 * A field the file does not mention keeps the default. A value longer than
 * its field is cut, and grok_config_parse() says so. api_key and password
 * are secrets: the rest of the app may test has_key and must not print them.
 */
typedef struct {
    char base_url[GROK_CFG_URL_MAX]; /**< No trailing slash. https:// or http://. */
    char path[GROK_CFG_PATH_MAX];    /**< Begins with '/'.                         */
    char model[GROK_CFG_MODEL_MAX];  /**< Sent as the "model" field.               */
    char api_key[GROK_CFG_KEY_MAX];  /**< Empty when the file has none.            */
    char system[GROK_CFG_SYSTEM_MAX];/**< Sent as the system turn. Empty skips it. */
    char ssid[GROK_CFG_SSID_MAX];    /**< Empty: do not join from this file.       */
    char password[GROK_CFG_PASS_MAX];/**< The password for ssid. Empty is open.    */
    bool has_key;                    /**< api_key is not empty.                    */
} grok_config_t;

/* ---- Calls ---- */

/**
 * @brief Fill @p cfg with the defaults this app ships.
 *
 * The base is https://api.x.ai/v1, the path is /chat/completions, the model
 * is grok-4.3, and the key is empty.
 *
 * @param cfg Filled. A NULL cfg is ignored.
 */
void grok_config_defaults(grok_config_t *cfg);

/**
 * @brief Overlay @p text on @p cfg. Defaults stay where the text is silent.
 *
 * Lines are key=value. A '#' line is a comment. Blank lines are skipped.
 * Unknown keys are skipped. Spaces around the key and the value are dropped.
 * One trailing slash on base_url is removed.
 *
 * @param cfg       Already filled, usually by grok_config_defaults().
 * @param text      The whole file, NUL terminated. NULL is refused.
 * @param truncated Out, may be NULL: set when a value did not fit its field.
 * @return false when @p cfg or @p text is NULL.
 */
bool grok_config_parse(grok_config_t *cfg, const char *text, bool *truncated);

/**
 * @brief Write base_url and path as one URL.
 *
 * @param cfg The config.
 * @param dst Where the URL goes. NUL terminated when @p cap is above 0.
 * @param cap Bytes @p dst holds.
 * @return false when an argument is wrong or the URL does not fit.
 */
bool grok_config_url(const grok_config_t *cfg, char *dst, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* GROK_CONFIG_H_ */
