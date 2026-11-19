/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_http.c
 * @brief esp_http_client POST with a bearer and a streamed body.
 */
#include "grok_http.h"

#include <stdio.h>
#include <string.h>

#if !defined(ESP_PLATFORM)

int grok_http_post(const char *url, const char *api_key, const char *body,
                   size_t body_len, grok_http_on_bytes on_bytes, void *ctx,
                   int *status, char *detail, size_t detail_cap)
{
    (void)url;
    (void)api_key;
    (void)body;
    (void)body_len;
    (void)on_bytes;
    (void)ctx;
    if ( NULL != status )
    {
        *status = 0;
    }
    if ( (NULL != detail) && (0U < detail_cap) )
    {
        size_t i = 0U;
        const char *text = "no network in this build";

        while ( ('\0' != text[i]) && ((detail_cap - 1U) > i) )
        {
            detail[i] = text[i];
            i++;
        }
        detail[i] = '\0';
    }
    return -1;
}

#else /* ESP_PLATFORM */

#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "grok.http"

/** @brief How long one read may wait for the next byte, ms. */
#define GROK_HTTP_TIMEOUT_MS  120000

/** @brief Empty reads that are not the end of the body before giving up. */
#define GROK_HTTP_EMPTY_MAX   4

/**
 * @brief Copy a short reason and always terminate it.
 * @param detail The buffer. NULL is ignored.
 * @param cap    Its size.
 * @param text   The reason.
 */
static void detail_set(char *detail, size_t cap, const char *text)
{
    size_t n = 0U;

    if ( (NULL == detail) || (0U == cap) )
    {
        return;
    }
    if ( NULL == text )
    {
        detail[0] = '\0';
        return;
    }
    while ( ('\0' != text[n]) && ((cap - 1U) > n) )
    {
        detail[n] = text[n];
        n++;
    }
    detail[n] = '\0';
}

/**
 * @brief True when the key can sit in one header value.
 * @param key The key. NULL is safe.
 */
static bool key_header_ok(const char *key)
{
    const unsigned char *p;

    if ( (NULL == key) || ('\0' == key[0]) )
    {
        return true;
    }
    for ( p = (const unsigned char *)key; '\0' != *p; p++ )
    {
        if ( (0x20U >= *p) || (0x7EU < *p) )
        {
            return false;
        }
    }
    return true;
}

int grok_http_post(const char *url, const char *api_key, const char *body,
                   size_t body_len, grok_http_on_bytes on_bytes, void *ctx,
                   int *status, char *detail, size_t detail_cap)
{
    esp_http_client_config_t cfg;
    esp_http_client_handle_t h;
    char                     auth[220];
    int                      empty = 0;
    int                      st = 0;
    bool                     send_key;

    if ( NULL != status )
    {
        *status = 0;
    }
    detail_set(detail, detail_cap, "");
    if ( (NULL == url) || ('\0' == url[0]) || (NULL == body) || (0U == body_len) )
    {
        detail_set(detail, detail_cap, "bad request");
        return -1;
    }
    if ( !key_header_ok(api_key) )
    {
        detail_set(detail, detail_cap, "the API key has a character a header cannot carry");
        return -1;
    }
    send_key = (NULL != api_key) && ('\0' != api_key[0]);
    auth[0] = '\0';
    if ( send_key )
    {
        int nw = snprintf(auth, sizeof(auth), "Bearer %s", api_key);

        if ( (0 > nw) || ((size_t)nw >= sizeof(auth)) )
        {
            (void)memset(auth, 0, sizeof(auth));
            detail_set(detail, detail_cap, "the API key does not fit the header");
            return -1;
        }
    }

    (void)memset(&cfg, 0, sizeof(cfg));
    cfg.url = url;
    cfg.method = HTTP_METHOD_POST;
    cfg.timeout_ms = GROK_HTTP_TIMEOUT_MS;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.disable_auto_redirect = true;
    cfg.buffer_size = 1024;
    cfg.buffer_size_tx = 1024;

    h = esp_http_client_init(&cfg);
    if ( NULL == h )
    {
        (void)memset(auth, 0, sizeof(auth));
        detail_set(detail, detail_cap, "no HTTP client");
        return -1;
    }
    (void)esp_http_client_set_header(h, "Content-Type", "application/json");
    (void)esp_http_client_set_header(h, "Accept", "text/event-stream");
    if ( send_key )
    {
        (void)esp_http_client_set_header(h, "Authorization", auth);
    }
    /* The key is in auth and in the client. Do not log either. */
    (void)memset(auth, 0, sizeof(auth));

    if ( ESP_OK != esp_http_client_open(h, (int)body_len) )
    {
        KLOG_W(TAG, "could not open %s", url);
        esp_http_client_cleanup(h);
        detail_set(detail, detail_cap, "could not connect");
        return -1;
    }
    if ( esp_http_client_write(h, body, (int)body_len) != (int)body_len )
    {
        KLOG_W(TAG, "the body was not written");
        (void)esp_http_client_close(h);
        esp_http_client_cleanup(h);
        detail_set(detail, detail_cap, "the request was not sent");
        return -1;
    }
    if ( 0 > esp_http_client_fetch_headers(h) )
    {
        KLOG_W(TAG, "no status line");
        (void)esp_http_client_close(h);
        esp_http_client_cleanup(h);
        detail_set(detail, detail_cap, "no answer");
        return -1;
    }
    st = esp_http_client_get_status_code(h);
    if ( NULL != status )
    {
        *status = st;
    }
    KLOG_I(TAG, "HTTP %d", st);

    if ( 200 != st )
    {
        char   scratch[160];
        size_t got = 0U;

        /* A short error body is enough to show. The rest is drained so the
         * connection can close. */
        for ( ;; )
        {
            int n = esp_http_client_read(h, scratch, (int)sizeof(scratch) - 1);

            if ( 0 > n )
            {
                break;
            }
            if ( 0 == n )
            {
                if ( esp_http_client_is_complete_data_received(h) )
                {
                    break;
                }
                if ( GROK_HTTP_EMPTY_MAX < ++empty )
                {
                    break;
                }
                continue;
            }
            empty = 0;
            if ( (NULL != detail) && (got + 1U < detail_cap) )
            {
                size_t room = detail_cap - 1U - got;
                size_t take = (size_t)n;

                if ( take > room )
                {
                    take = room;
                }
                (void)memcpy(&detail[got], scratch, take);
                got += take;
                detail[got] = '\0';
            }
        }
        if ( (NULL != detail) && ('\0' == detail[0]) )
        {
            detail_set(detail, detail_cap, "the server refused the request");
        }
        (void)esp_http_client_close(h);
        esp_http_client_cleanup(h);
        return 0;
    }

    for ( ;; )
    {
        char buf[512];
        int  n = esp_http_client_read(h, buf, (int)sizeof(buf));

        if ( 0 > n )
        {
            detail_set(detail, detail_cap, "the reply stopped");
            (void)esp_http_client_close(h);
            esp_http_client_cleanup(h);
            return -1;
        }
        if ( 0 == n )
        {
            /* A zero read is not the end. kdrv_net learned that the hard way. */
            if ( esp_http_client_is_complete_data_received(h) )
            {
                break;
            }
            if ( GROK_HTTP_EMPTY_MAX < ++empty )
            {
                detail_set(detail, detail_cap, "the reply stalled");
                (void)esp_http_client_close(h);
                esp_http_client_cleanup(h);
                return -1;
            }
            continue;
        }
        empty = 0;
        if ( (NULL != on_bytes) && !on_bytes(buf, (size_t)n, ctx) )
        {
            break;
        }
    }
    (void)esp_http_client_close(h);
    esp_http_client_cleanup(h);
    return 0;
}

#endif /* ESP_PLATFORM */
