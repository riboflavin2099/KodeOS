/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_client.c
 * @brief Turn the transcript into one streamed chat request.
 */
#include "grok_client.h"

#include <string.h>

#include "esp_heap_caps.h"

#include "grok_http.h"
#include "grok_request.h"
#include "grok_sse.h"
#include "kode_log.h"

/** @brief This file's log tag. */
#define TAG "grok.client"

/** @brief Bytes of JSON this client will send. In PSRAM. */
#define GROK_REQ_MAX  8192U

/** @brief What the SSE callback appends into. */
typedef struct {
    grok_sse_t *parser;          /**< The parser, in PSRAM.     */
    char       *reply;           /**< The answer being built.   */
    size_t      cap;             /**< Bytes reply holds.        */
    size_t      len;             /**< Bytes written, no NUL.    */
    int         saw_text;        /**< A content delta arrived.  */
    int         saw_done;        /**< The server closed the turn. */
    int         saw_error;       /**< An error event arrived.   */
    char        err[160];        /**< The error text.           */
} ask_ctx_t;

/** @brief The JSON body. Taken once, from PSRAM. */
static char *s_body;

/** @brief The parser. Taken once, from PSRAM. It is about 4 KB. */
static grok_sse_t *s_parser;

/**
 * @brief Take the two PSRAM blocks the first time.
 * @return false when PSRAM has no room.
 */
static bool blocks(void)
{
    if ( NULL == s_body )
    {
        s_body = heap_caps_malloc(GROK_REQ_MAX, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if ( NULL == s_parser )
    {
        s_parser = heap_caps_calloc(1, sizeof(*s_parser),
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    return (NULL != s_body) && (NULL != s_parser);
}

/**
 * @brief Copy a sentence into the reply and terminate it.
 * @param reply The buffer.
 * @param cap   Its size.
 * @param text  The sentence.
 */
static void say(char *reply, size_t cap, const char *text)
{
    size_t n = 0U;

    if ( (NULL == reply) || (0U == cap) )
    {
        return;
    }
    if ( NULL == text )
    {
        reply[0] = '\0';
        return;
    }
    while ( ('\0' != text[n]) && ((cap - 1U) > n) )
    {
        reply[n] = text[n];
        n++;
    }
    reply[n] = '\0';
}

/**
 * @brief One SSE event: keep answer text, stop on done or an error.
 * @return false to stop the HTTP read.
 */
static bool on_sse(const grok_sse_event_t *ev, void *vctx)
{
    ask_ctx_t *ctx = (ask_ctx_t *)vctx;
    size_t     n;
    size_t     i;

    if ( GROK_SSE_TEXT == ev->kind )
    {
        n = strlen(ev->text);
        for ( i = 0U; (n > i) && ((ctx->cap - 1U) > ctx->len); i++ )
        {
            ctx->reply[ctx->len] = ev->text[i];
            ctx->len++;
        }
        ctx->reply[ctx->len] = '\0';
        ctx->saw_text = 1;
        return true;
    }
    if ( GROK_SSE_DONE == ev->kind )
    {
        ctx->saw_done = 1;
        return false;
    }
    if ( GROK_SSE_ERROR == ev->kind )
    {
        say(ctx->err, sizeof(ctx->err), ev->text);
        ctx->saw_error = 1;
        return false;
    }
    /* Reasoning is not the answer on this glass. */
    return true;
}

/**
 * @brief Feed one HTTP chunk to the parser.
 * @return false when the parser wants the body closed.
 */
static bool on_bytes(const char *data, size_t n, void *vctx)
{
    ask_ctx_t *ctx = (ask_ctx_t *)vctx;

    return grok_sse_feed(ctx->parser, data, n, &on_sse, ctx);
}

/**
 * @brief True when the base is xAI's own host, which will refuse an empty key.
 * @param base The base URL.
 */
static bool is_xai(const char *base)
{
    return (NULL != base) && (NULL != strstr(base, "://api.x.ai"));
}

grok_ask_t grok_client_ask(const grok_config_t *cfg, const grok_transcript_t *tr,
                           char *reply, size_t cap)
{
    grok_msg_t prior[GROK_TR_LINES];
    ask_ctx_t  ctx;
    char       url[GROK_CFG_URL_MAX + GROK_CFG_PATH_MAX];
    size_t     np = 0U;
    size_t     i;
    const char *user;
    size_t     body_len;
    int        status = 0;
    char       detail[160];
    int        http_rc;

    if ( (NULL == reply) || (2U > cap) )
    {
        return GROK_ASK_NOTE;
    }
    reply[0] = '\0';
    if ( (NULL == cfg) || (NULL == tr) )
    {
        say(reply, cap, "Nothing to ask.");
        return GROK_ASK_NOTE;
    }
    for ( i = 0U; tr->count > i; i++ )
    {
        if ( GROK_TR_NOTE == tr->lines[i].role )
        {
            continue;
        }
        if ( GROK_TR_LINES <= np )
        {
            break;
        }
        prior[np].role = (GROK_TR_USER == tr->lines[i].role) ? "user" : "assistant";
        prior[np].content = tr->lines[i].text;
        np++;
    }
    if ( (0U == np) || (0 != strcmp(prior[np - 1U].role, "user")) )
    {
        say(reply, cap, "Nothing to ask.");
        return GROK_ASK_NOTE;
    }
    user = prior[np - 1U].content;
    if ( is_xai(cfg->base_url) && !cfg->has_key )
    {
        say(reply, cap, "No API key. Add api_key to sd/grok/config.txt.");
        return GROK_ASK_NOTE;
    }
    if ( !grok_config_url(cfg, url, sizeof(url)) )
    {
        say(reply, cap, "The server address does not fit.");
        return GROK_ASK_NOTE;
    }
    if ( !blocks() )
    {
        say(reply, cap, "No memory for the request.");
        return GROK_ASK_NOTE;
    }
    body_len = grok_request_fit(s_body, GROK_REQ_MAX, cfg->model, cfg->system,
                                prior, np - 1U, user);
    if ( 0U == body_len )
    {
        say(reply, cap, "The question does not fit.");
        return GROK_ASK_NOTE;
    }
    KLOG_I(TAG, "POST %s model %s", url, cfg->model);

    (void)memset(&ctx, 0, sizeof(ctx));
    ctx.parser = s_parser;
    ctx.reply = reply;
    ctx.cap = cap;
    grok_sse_init(s_parser);
    http_rc = grok_http_post(url, cfg->has_key ? cfg->api_key : NULL,
                             s_body, body_len, &on_bytes, &ctx,
                             &status, detail, sizeof(detail));
    if ( 0 == http_rc )
    {
        (void)grok_sse_flush(s_parser, &on_sse, &ctx);
    }
    /* The body held the question, not the key. Drop it anyway. */
    (void)memset(s_body, 0, body_len);

    if ( 0 != http_rc )
    {
        if ( '\0' == detail[0] )
        {
            say(reply, cap, "Could not reach the server.");
        }
        else
        {
            say(reply, cap, detail);
        }
        return GROK_ASK_HTTP;
    }
    if ( 200 != status )
    {
        if ( 401 == status )
        {
            say(reply, cap, "The API key was refused.");
        }
        else if ( 429 == status )
        {
            say(reply, cap, "Rate limit. Wait and try again.");
        }
        else if ( '\0' != detail[0] )
        {
            say(reply, cap, detail);
        }
        else
        {
            say(reply, cap, "The server refused the request.");
        }
        return GROK_ASK_HTTP;
    }
    if ( ctx.saw_error && !ctx.saw_text )
    {
        say(reply, cap, ('\0' != ctx.err[0]) ? ctx.err : "The server reported an error.");
        return GROK_ASK_HTTP;
    }
    if ( !ctx.saw_text )
    {
        say(reply, cap, "The reply was empty.");
        return GROK_ASK_HTTP;
    }
    return GROK_ASK_OK;
}
