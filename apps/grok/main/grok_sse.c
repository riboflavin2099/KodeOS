/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_sse.c
 * @brief SSE framing plus the two JSON shapes the chat client understands.
 */
#include "grok_sse.h"

#include <string.h>

/** @brief How deep a JSON value may nest before the walk gives up. */
#define GROK_SSE_NEST_MAX  8

/**
 * @brief One decoded code point into @p out.
 * @return false when it did not fit. @p clipped is set.
 */
static bool put_cp(char *out, size_t cap, size_t *n, unsigned cp, bool *clipped)
{
    unsigned char tmp[4];
    size_t        need = 0U;

    if ( (NULL == out) || (0U == cap) )
    {
        return true;
    }
    if ( 0x80U > cp )
    {
        tmp[0] = (unsigned char)cp;
        need = 1U;
    }
    else if ( 0x800U > cp )
    {
        tmp[0] = (unsigned char)(0xC0U | (cp >> 6));
        tmp[1] = (unsigned char)(0x80U | (cp & 0x3FU));
        need = 2U;
    }
    else if ( 0x10000U > cp )
    {
        tmp[0] = (unsigned char)(0xE0U | (cp >> 12));
        tmp[1] = (unsigned char)(0x80U | ((cp >> 6) & 0x3FU));
        tmp[2] = (unsigned char)(0x80U | (cp & 0x3FU));
        need = 3U;
    }
    else
    {
        tmp[0] = (unsigned char)(0xF0U | (cp >> 18));
        tmp[1] = (unsigned char)(0x80U | ((cp >> 12) & 0x3FU));
        tmp[2] = (unsigned char)(0x80U | ((cp >> 6) & 0x3FU));
        tmp[3] = (unsigned char)(0x80U | (cp & 0x3FU));
        need = 4U;
    }
    if ( (*n + need) >= cap )
    {
        if ( NULL != clipped )
        {
            *clipped = true;
        }
        return false;
    }
    (void)memcpy(&out[*n], tmp, need);
    *n += need;
    return true;
}

/** @brief Hex digit, or -1. */
static int hex_val(char c)
{
    if ( ('0' <= c) && ('9' >= c) )
    {
        return (int)c - '0';
    }
    if ( ('a' <= c) && ('f' >= c) )
    {
        return (int)c - 'a' + 10;
    }
    if ( ('A' <= c) && ('F' >= c) )
    {
        return (int)c - 'A' + 10;
    }
    return -1;
}

/**
 * @brief Read the JSON string that starts at @p s[*i] == '"'.
 *
 * @param out NULL discards the characters.
 * @param cap Bytes @p out holds.
 * @return false when the string never closes.
 */
static bool read_string(const char *s, size_t n, size_t *i, char *out, size_t cap,
                        bool *clipped)
{
    size_t used = 0U;

    if ( (NULL == s) || (NULL == i) || (*i >= n) || ('"' != s[*i]) )
    {
        return false;
    }
    if ( NULL != clipped )
    {
        *clipped = false;
    }
    (*i)++;
    while ( *i < n )
    {
        unsigned char c = (unsigned char)s[*i];

        if ( '"' == c )
        {
            (*i)++;
            if ( (NULL != out) && (0U < cap) )
            {
                if ( used >= cap )
                {
                    used = cap - 1U;
                }
                out[used] = '\0';
            }
            return true;
        }
        if ( '\\' == c )
        {
            unsigned cp = 0U;

            (*i)++;
            if ( *i >= n )
            {
                return false;
            }
            c = (unsigned char)s[*i];
            (*i)++;
            if ( '"' == c )
            {
                cp = '"';
            }
            else if ( '\\' == c )
            {
                cp = '\\';
            }
            else if ( '/' == c )
            {
                cp = '/';
            }
            else if ( 'b' == c )
            {
                cp = '\b';
            }
            else if ( 'f' == c )
            {
                cp = '\f';
            }
            else if ( 'n' == c )
            {
                cp = '\n';
            }
            else if ( 'r' == c )
            {
                cp = '\r';
            }
            else if ( 't' == c )
            {
                cp = '\t';
            }
            else if ( 'u' == c )
            {
                int h0;
                int h1;
                int h2;
                int h3;

                if ( (*i + 4U) > n )
                {
                    return false;
                }
                h0 = hex_val(s[*i]);
                h1 = hex_val(s[*i + 1U]);
                h2 = hex_val(s[*i + 2U]);
                h3 = hex_val(s[*i + 3U]);
                *i += 4U;
                if ( (0 > h0) || (0 > h1) || (0 > h2) || (0 > h3) )
                {
                    return false;
                }
                cp = ((unsigned)h0 << 12) | ((unsigned)h1 << 8)
                   | ((unsigned)h2 << 4) | (unsigned)h3;
                /* A high surrogate wants a low one next. A lone one is dropped. */
                if ( (0xD800U <= cp) && (0xDC00U > cp) && ((*i + 6U) <= n)
                     && ('\\' == s[*i]) && ('u' == s[*i + 1U]) )
                {
                    int l0 = hex_val(s[*i + 2U]);
                    int l1 = hex_val(s[*i + 3U]);
                    int l2 = hex_val(s[*i + 4U]);
                    int l3 = hex_val(s[*i + 5U]);
                    unsigned low;

                    if ( (0 > l0) || (0 > l1) || (0 > l2) || (0 > l3) )
                    {
                        return false;
                    }
                    low = ((unsigned)l0 << 12) | ((unsigned)l1 << 8)
                        | ((unsigned)l2 << 4) | (unsigned)l3;
                    if ( (0xDC00U <= low) && (0xE000U > low) )
                    {
                        *i += 6U;
                        cp = 0x10000U + (((cp - 0xD800U) << 10) | (low - 0xDC00U));
                    }
                    else
                    {
                        continue;
                    }
                }
                else if ( (0xD800U <= cp) && (0xE000U > cp) )
                {
                    continue;
                }
            }
            else
            {
                return false;
            }
            (void)put_cp(out, cap, &used, cp, clipped);
            continue;
        }
        (*i)++;
        (void)put_cp(out, cap, &used, c, clipped);
    }
    return false;
}

/** @brief Index of the next byte that is not space, tab, CR or LF. */
static size_t skip_ws(const char *s, size_t n, size_t i)
{
    while ( (i < n) && ((' ' == s[i]) || ('\t' == s[i]) || ('\n' == s[i])
                        || ('\r' == s[i])) )
    {
        i++;
    }
    return i;
}

/** @brief Index just past the container that starts at @p i. */
static size_t skip_container(const char *s, size_t n, size_t i)
{
    char open;
    char close;
    int  depth = 0;

    if ( i >= n )
    {
        return n;
    }
    open = s[i];
    close = ('{' == open) ? '}' : ']';
    while ( i < n )
    {
        if ( '"' == s[i] )
        {
            if ( !read_string(s, n, &i, NULL, 0U, NULL) )
            {
                return n;
            }
            continue;
        }
        if ( open == s[i] )
        {
            depth++;
        }
        else if ( close == s[i] )
        {
            depth--;
            i++;
            if ( 0 == depth )
            {
                return i;
            }
            continue;
        }
        i++;
    }
    return n;
}

/** @brief Index just past one JSON value. */
static size_t skip_value(const char *s, size_t n, size_t i)
{
    i = skip_ws(s, n, i);
    if ( i >= n )
    {
        return n;
    }
    if ( ('"' == s[i]) )
    {
        if ( !read_string(s, n, &i, NULL, 0U, NULL) )
        {
            return n;
        }
        return i;
    }
    if ( ('{' == s[i]) || ('[' == s[i]) )
    {
        return skip_container(s, n, i);
    }
    while ( (i < n) && (',' != s[i]) && ('}' != s[i]) && (']' != s[i])
            && (' ' != s[i]) && ('\n' != s[i]) && ('\r' != s[i]) && ('\t' != s[i]) )
    {
        i++;
    }
    return i;
}

/**
 * @brief The first string value of @p key inside the JSON from @p begin to @p end.
 * @return true when that string was found.
 */
static bool find_string(const char *s, size_t n, size_t begin, size_t end,
                        const char *key, char *out, size_t cap, bool *clipped,
                        int depth)
{
    size_t i = begin;

    if ( GROK_SSE_NEST_MAX < depth )
    {
        return false;
    }
    while ( i < end )
    {
        i = skip_ws(s, n, i);
        if ( i >= end )
        {
            return false;
        }
        if ( '{' == s[i] )
        {
            i++;
            while ( i < end )
            {
                char   keybuf[48];
                bool   key_cut = false;
                size_t vend;

                i = skip_ws(s, n, i);
                if ( (i < end) && ('}' == s[i]) )
                {
                    break;
                }
                if ( (i >= end) || ('"' != s[i]) )
                {
                    return false;
                }
                if ( !read_string(s, n, &i, keybuf, sizeof(keybuf), &key_cut) )
                {
                    return false;
                }
                i = skip_ws(s, n, i);
                if ( (i >= end) || (':' != s[i]) )
                {
                    return false;
                }
                i++;
                i = skip_ws(s, n, i);
                if ( i >= end )
                {
                    return false;
                }
                if ( !key_cut && (0 == strcmp(keybuf, key)) && ('"' == s[i]) )
                {
                    return read_string(s, n, &i, out, cap, clipped);
                }
                if ( ('{' == s[i]) || ('[' == s[i]) )
                {
                    vend = skip_container(s, n, i);
                    if ( find_string(s, n, i, vend, key, out, cap, clipped, depth + 1) )
                    {
                        return true;
                    }
                    i = vend;
                }
                else
                {
                    i = skip_value(s, n, i);
                }
                i = skip_ws(s, n, i);
                if ( (i < end) && (',' == s[i]) )
                {
                    i++;
                    continue;
                }
                break;
            }
            return false;
        }
        if ( '[' == s[i] )
        {
            i++;
            while ( i < end )
            {
                size_t vend;

                i = skip_ws(s, n, i);
                if ( (i < end) && (']' == s[i]) )
                {
                    break;
                }
                if ( i >= end )
                {
                    return false;
                }
                if ( ('{' == s[i]) || ('[' == s[i]) )
                {
                    vend = skip_container(s, n, i);
                    if ( find_string(s, n, i, vend, key, out, cap, clipped, depth + 1) )
                    {
                        return true;
                    }
                    i = vend;
                }
                else
                {
                    i = skip_value(s, n, i);
                }
                i = skip_ws(s, n, i);
                if ( (i < end) && (',' == s[i]) )
                {
                    i++;
                    continue;
                }
                break;
            }
            return false;
        }
        return false;
    }
    return false;
}

/**
 * @brief Hand one event to the callback.
 * @return The callback's answer. false means stop.
 */
static bool emit(grok_sse_fn fn, void *ctx, grok_sse_kind_t kind, const char *text,
                 bool clipped)
{
    grok_sse_event_t ev;

    (void)memset(&ev, 0, sizeof(ev));
    ev.kind = kind;
    ev.clipped = clipped;
    if ( NULL != text )
    {
        size_t n = strlen(text);

        if ( n >= sizeof(ev.text) )
        {
            n = sizeof(ev.text) - 1U;
            ev.clipped = true;
        }
        (void)memcpy(ev.text, text, n);
        ev.text[n] = '\0';
    }
    return fn(&ev, ctx);
}

/**
 * @brief Parse the payload that a blank line just closed.
 * @return false when the callback said stop.
 */
static bool dispatch(grok_sse_t *p, grok_sse_fn fn, void *ctx)
{
    char   text[GROK_SSE_DELTA_MAX];
    char   kind_name[GROK_SSE_EVENT_MAX];
    bool   clipped = false;
    bool   ok = true;
    size_t n;

    if ( p->overflow )
    {
        p->overflow = false;
        p->payload_len = 0U;
        p->event_name[0] = '\0';
        return emit(fn, ctx, GROK_SSE_ERROR, "event too long", false);
    }
    if ( 0U == p->payload_len )
    {
        p->event_name[0] = '\0';
        return true;
    }
    p->payload[p->payload_len] = '\0';
    n = p->payload_len;
    {
        size_t i = skip_ws(p->payload, n, 0U);

        if ( (i + 6U) <= n )
        {
            /* [DONE] is the chat-completions end marker, not JSON. */
            if ( 0 == strncmp(&p->payload[i], "[DONE]", 6U) )
            {
                size_t j = skip_ws(p->payload, n, i + 6U);

                if ( j == n )
                {
                    p->payload_len = 0U;
                    p->event_name[0] = '\0';
                    return emit(fn, ctx, GROK_SSE_DONE, "", false);
                }
            }
        }
    }
    kind_name[0] = '\0';
    (void)find_string(p->payload, n, 0U, n, "type", kind_name, sizeof(kind_name),
                      NULL, 1);
    if ( ('\0' == kind_name[0]) && ('\0' != p->event_name[0]) )
    {
        (void)memcpy(kind_name, p->event_name, sizeof(kind_name));
    }
    if ( (0 == strcmp(kind_name, "response.output_text.delta"))
         || (0 == strcmp(p->event_name, "response.output_text.delta")) )
    {
        clipped = false;
        if ( find_string(p->payload, n, 0U, n, "delta", text, sizeof(text),
                         &clipped, 1)
             && ('\0' != text[0]) )
        {
            ok = emit(fn, ctx, GROK_SSE_TEXT, text, clipped);
        }
    }
    else if ( (0 == strcmp(kind_name, "response.completed"))
              || (0 == strcmp(kind_name, "response.output_text.done")) )
    {
        ok = emit(fn, ctx, GROK_SSE_DONE, "", false);
    }
    else if ( 0 == strcmp(kind_name, "response.failed") )
    {
        clipped = false;
        if ( !find_string(p->payload, n, 0U, n, "message", text, sizeof(text),
                          &clipped, 1) )
        {
            (void)memcpy(text, "response failed", 16U);
        }
        ok = emit(fn, ctx, GROK_SSE_ERROR, text, clipped);
    }
    else
    {
        bool saw = false;

        clipped = false;
        if ( find_string(p->payload, n, 0U, n, "reasoning_content", text,
                         sizeof(text), &clipped, 1)
             && ('\0' != text[0]) )
        {
            ok = emit(fn, ctx, GROK_SSE_REASON, text, clipped);
            saw = true;
        }
        if ( ok )
        {
            clipped = false;
            if ( find_string(p->payload, n, 0U, n, "content", text, sizeof(text),
                             &clipped, 1)
                 && ('\0' != text[0]) )
            {
                ok = emit(fn, ctx, GROK_SSE_TEXT, text, clipped);
                saw = true;
            }
        }
        if ( ok && !saw
             && find_string(p->payload, n, 0U, n, "error", text, sizeof(text),
                            &clipped, 1) )
        {
            /* "error" as a string is unusual. The object form is below. */
            ok = emit(fn, ctx, GROK_SSE_ERROR, text, clipped);
            saw = true;
        }
        if ( ok && !saw
             && (NULL != strstr(p->payload, "\"error\""))
             && find_string(p->payload, n, 0U, n, "message", text, sizeof(text),
                            &clipped, 1)
             && ('\0' != text[0]) )
        {
            /* An error object: its message is the text to show. A chunk that
             * only carries a role has neither, and that is not an error. */
            ok = emit(fn, ctx, GROK_SSE_ERROR, text, clipped);
        }
    }
    p->payload_len = 0U;
    p->event_name[0] = '\0';
    return ok;
}

/**
 * @brief Take the line in p->line as one SSE line.
 * @return false when the callback said stop.
 */
static bool take_line(grok_sse_t *p, grok_sse_fn fn, void *ctx)
{
    const char *v;
    size_t      vlen;
    bool        blank;

    if ( p->skip_line )
    {
        p->skip_line = false;
        p->line_len = 0U;
        p->overflow = true;
        return true;
    }
    p->line[p->line_len] = '\0';
    blank = true;
    for ( v = p->line; '\0' != *v; v++ )
    {
        if ( (' ' != *v) && ('\t' != *v) && ('\r' != *v) )
        {
            blank = false;
            break;
        }
    }
    p->line_len = 0U;
    if ( blank )
    {
        return dispatch(p, fn, ctx);
    }
    if ( ':' == p->line[0] )
    {
        return true;
    }
    if ( 0 == strncmp(p->line, "event:", 6U) )
    {
        v = &p->line[6];
        if ( ' ' == *v )
        {
            v++;
        }
        (void)memset(p->event_name, 0, sizeof(p->event_name));
        vlen = strlen(v);
        if ( vlen >= sizeof(p->event_name) )
        {
            vlen = sizeof(p->event_name) - 1U;
        }
        (void)memcpy(p->event_name, v, vlen);
        return true;
    }
    if ( 0 == strncmp(p->line, "data:", 5U) )
    {
        v = &p->line[5];
        if ( ' ' == *v )
        {
            v++;
        }
        vlen = strlen(v);
        if ( p->overflow )
        {
            return true;
        }
        if ( 0U < p->payload_len )
        {
            if ( (p->payload_len + 1U + vlen) >= GROK_SSE_PAYLOAD_MAX )
            {
                p->overflow = true;
                p->payload_len = 0U;
                return true;
            }
            p->payload[p->payload_len] = '\n';
            p->payload_len++;
        }
        else if ( (vlen + 1U) >= GROK_SSE_PAYLOAD_MAX )
        {
            p->overflow = true;
            return true;
        }
        (void)memcpy(&p->payload[p->payload_len], v, vlen);
        p->payload_len += vlen;
        p->payload[p->payload_len] = '\0';
        return true;
    }
    return true;
}

void grok_sse_init(grok_sse_t *p)
{
    if ( NULL != p )
    {
        (void)memset(p, 0, sizeof(*p));
    }
}

bool grok_sse_feed(grok_sse_t *p, const char *data, size_t n, grok_sse_fn fn,
                   void *ctx)
{
    size_t i;

    if ( (NULL == p) || (NULL == fn) || ((0U < n) && (NULL == data)) )
    {
        return false;
    }
    for ( i = 0U; n > i; i++ )
    {
        char c = data[i];

        if ( '\n' == c )
        {
            if ( !take_line(p, fn, ctx) )
            {
                return false;
            }
            continue;
        }
        if ( p->skip_line )
        {
            continue;
        }
        if ( '\r' == c )
        {
            continue;
        }
        if ( (p->line_len + 1U) >= GROK_SSE_PAYLOAD_MAX )
        {
            p->skip_line = true;
            continue;
        }
        p->line[p->line_len] = c;
        p->line_len++;
    }
    return true;
}

bool grok_sse_flush(grok_sse_t *p, grok_sse_fn fn, void *ctx)
{
    if ( (NULL == p) || (NULL == fn) )
    {
        return false;
    }
    if ( (0U < p->line_len) || p->skip_line )
    {
        if ( !take_line(p, fn, ctx) )
        {
            return false;
        }
    }
    if ( (0U < p->payload_len) || p->overflow )
    {
        return dispatch(p, fn, ctx);
    }
    return true;
}
