/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_config.c
 * @brief Parse sd/grok/config.txt. The key stays in the struct.
 */
#include "grok_config.h"

#include <string.h>

/** @brief The public chat endpoint this scaffold speaks. */
#define GROK_DEFAULT_URL   "https://api.x.ai/v1"

/** @brief Chat Completions, the small SSE shape. See FEASIBILITY.md. */
#define GROK_DEFAULT_PATH  "/chat/completions"

/** @brief Cheapest current general model on the public API, 2026-11-19. */
#define GROK_DEFAULT_MODEL "grok-4.3"

/** @brief Shown on a small glass: short answers, no preamble. */
#define GROK_DEFAULT_SYSTEM \
    "You are Grok, a helpful assistant. Answer in a few short sentences."

/**
 * @brief Copy @p src into @p dst, always NUL terminated.
 * @param dst  The field.
 * @param cap  Bytes @p dst holds, above 0.
 * @param src  The text. NULL copies nothing.
 * @return false when the text was cut to fit.
 */
static bool copy_field(char *dst, size_t cap, const char *src)
{
    size_t n = 0U;

    if ( (NULL == dst) || (0U == cap) )
    {
        return false;
    }
    if ( NULL == src )
    {
        dst[0] = '\0';
        return true;
    }
    while ( ('\0' != src[n]) && ((cap - 1U) > n) )
    {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';
    return '\0' == src[n];
}

/**
 * @brief Drop spaces and tabs at both ends, in place.
 * @param s The line. May be empty.
 */
static void trim(char *s)
{
    size_t n;
    size_t i = 0U;
    char  *start;

    if ( NULL == s )
    {
        return;
    }
    n = strlen(s);
    while ( (0U < n) && ((' ' == s[n - 1U]) || ('\t' == s[n - 1U])
                         || ('\r' == s[n - 1U])) )
    {
        n--;
        s[n] = '\0';
    }
    start = s;
    while ( (' ' == start[0]) || ('\t' == start[0]) )
    {
        start++;
    }
    if ( start != s )
    {
        while ( '\0' != start[i] )
        {
            s[i] = start[i];
            i++;
        }
        s[i] = '\0';
    }
}

/**
 * @brief Store one known key. Unknown keys are ignored.
 * @param cfg       The config.
 * @param key       The name, already trimmed.
 * @param value     The value, already trimmed.
 * @param truncated Set when a value is cut.
 * @return false when the key is unknown. That is not a failure for the file.
 */
static bool store_key(grok_config_t *cfg, const char *key, const char *value,
                      bool *truncated)
{
    bool fitted = true;

    if ( 0 == strcmp(key, "base_url") )
    {
        fitted = copy_field(cfg->base_url, sizeof(cfg->base_url), value);
    }
    else if ( 0 == strcmp(key, "path") )
    {
        fitted = copy_field(cfg->path, sizeof(cfg->path), value);
    }
    else if ( 0 == strcmp(key, "model") )
    {
        fitted = copy_field(cfg->model, sizeof(cfg->model), value);
    }
    else if ( 0 == strcmp(key, "api_key") )
    {
        fitted = copy_field(cfg->api_key, sizeof(cfg->api_key), value);
    }
    else if ( 0 == strcmp(key, "system") )
    {
        fitted = copy_field(cfg->system, sizeof(cfg->system), value);
    }
    else if ( 0 == strcmp(key, "ssid") )
    {
        fitted = copy_field(cfg->ssid, sizeof(cfg->ssid), value);
    }
    else if ( 0 == strcmp(key, "password") )
    {
        fitted = copy_field(cfg->password, sizeof(cfg->password), value);
    }
    else
    {
        return false;
    }
    if ( !fitted && (NULL != truncated) )
    {
        *truncated = true;
    }
    return true;
}

void grok_config_defaults(grok_config_t *cfg)
{
    if ( NULL == cfg )
    {
        return;
    }
    (void)memset(cfg, 0, sizeof(*cfg));
    (void)copy_field(cfg->base_url, sizeof(cfg->base_url), GROK_DEFAULT_URL);
    (void)copy_field(cfg->path, sizeof(cfg->path), GROK_DEFAULT_PATH);
    (void)copy_field(cfg->model, sizeof(cfg->model), GROK_DEFAULT_MODEL);
    (void)copy_field(cfg->system, sizeof(cfg->system), GROK_DEFAULT_SYSTEM);
    cfg->has_key = false;
}

bool grok_config_parse(grok_config_t *cfg, const char *text, bool *truncated)
{
    const char *p;
    bool        cut = false;

    if ( (NULL == cfg) || (NULL == text) )
    {
        return false;
    }
    if ( NULL != truncated )
    {
        *truncated = false;
    }
    p = text;
    while ( '\0' != *p )
    {
        char        line[GROK_CFG_SYSTEM_MAX + GROK_CFG_KEY_MAX];
        size_t      n = 0U;
        char       *eq;
        const char *key;
        const char *value;

        while ( ('\0' != p[0]) && ('\n' != p[0]) && (sizeof(line) - 1U > n) )
        {
            line[n] = p[0];
            n++;
            p++;
        }
        /* A line that did not fit is skipped to its end, and reported. */
        if ( ('\0' != p[0]) && ('\n' != p[0]) )
        {
            cut = true;
            while ( ('\0' != p[0]) && ('\n' != p[0]) )
            {
                p++;
            }
        }
        line[n] = '\0';
        if ( '\n' == p[0] )
        {
            p++;
        }
        trim(line);
        if ( ('\0' == line[0]) || ('#' == line[0]) )
        {
            continue;
        }
        eq = strchr(line, '=');
        if ( NULL == eq )
        {
            continue;
        }
        *eq = '\0';
        trim(line);
        value = eq + 1;
        /* value points into line; trim needs a mutable copy of that tail. */
        trim(eq + 1);
        key = line;
        (void)store_key(cfg, key, value, &cut);
    }
    {
        size_t ulen = strlen(cfg->base_url);

        if ( (0U < ulen) && ('/' == cfg->base_url[ulen - 1U]) )
        {
            cfg->base_url[ulen - 1U] = '\0';
        }
    }
    if ( ('\0' != cfg->path[0]) && ('/' != cfg->path[0]) )
    {
        /* A path without a slash would glue onto the host. Refuse it by
         * putting the slash back in front when there is room. */
        size_t plen = strlen(cfg->path);

        if ( plen + 1U < sizeof(cfg->path) )
        {
            (void)memmove(&cfg->path[1], cfg->path, plen + 1U);
            cfg->path[0] = '/';
        }
        else
        {
            cut = true;
            (void)copy_field(cfg->path, sizeof(cfg->path), GROK_DEFAULT_PATH);
        }
    }
    cfg->has_key = '\0' != cfg->api_key[0];
    if ( NULL != truncated )
    {
        *truncated = cut;
    }
    return true;
}

bool grok_config_url(const grok_config_t *cfg, char *dst, size_t cap)
{
    size_t ulen;
    size_t plen;
    const char *path;

    if ( (NULL == cfg) || (NULL == dst) || (0U == cap) )
    {
        return false;
    }
    dst[0] = '\0';
    if ( '\0' == cfg->base_url[0] )
    {
        return false;
    }
    path = cfg->path;
    if ( '\0' == path[0] )
    {
        path = GROK_DEFAULT_PATH;
    }
    ulen = strlen(cfg->base_url);
    plen = strlen(path);
    if ( (ulen + plen + 1U) > cap )
    {
        return false;
    }
    (void)memcpy(dst, cfg->base_url, ulen);
    (void)memcpy(&dst[ulen], path, plen + 1U);
    return true;
}
