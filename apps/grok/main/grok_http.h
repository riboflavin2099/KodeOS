/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_http.h
 * @brief One streaming POST. The ESP-IDF client is in grok_http.c.
 *
 * The SDK's kode_http_post() cannot set an Authorization header and it
 * buffers the whole answer. kdrv_net's POST is capped at 1024 bytes and its
 * stream reader is GET only. Chat needs a bearer, a larger body and an SSE
 * read, so this file calls esp_http_client itself, from a worker.
 */
#ifndef GROK_HTTP_H_
#define GROK_HTTP_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Called with each piece of a 200 body. Return false to stop reading.
 * @param data The bytes. Not NUL terminated.
 * @param n    How many.
 * @param ctx  What grok_http_post() was given.
 * @return false to close the body early.
 */
typedef bool (*grok_http_on_bytes)(const char *data, size_t n, void *ctx);

/**
 * @brief POST @p body and stream a 200 answer.
 *
 * https uses the Mozilla bundle (esp_crt_bundle_attach). Redirects are not
 * followed, so a bearer is not sent on to another host. @p api_key NULL or
 * empty sends no Authorization header. The key is not written to the log.
 *
 * A status other than 200 puts the first bytes of the body into @p detail
 * and does not call @p on_bytes.
 *
 * @param url        The full URL.
 * @param api_key    The raw key, or NULL. Not logged.
 * @param body       The JSON. 
 * @param body_len   Bytes of @p body.
 * @param on_bytes   The reader. NULL discards a 200 body.
 * @param ctx        Passed to @p on_bytes.
 * @param status     Out, may be NULL: the HTTP status, or 0 when none came.
 * @param detail     Out, may be NULL: a short reason, NUL terminated.
 * @param detail_cap Bytes @p detail holds.
 * @return 0 when a status line arrived. -1 when the connection failed.
 */
int grok_http_post(const char *url, const char *api_key, const char *body,
                   size_t body_len, grok_http_on_bytes on_bytes, void *ctx,
                   int *status, char *detail, size_t detail_cap);

#ifdef __cplusplus
}
#endif

#endif /* GROK_HTTP_H_ */
