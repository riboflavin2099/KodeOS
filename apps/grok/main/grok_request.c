/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_request.c
 * @brief JSON for one chat turn. Strings are escaped. Roles are not free text.
 */
#include "grok_request.h"

#include <string.h>

/**
 * @brief Append raw bytes. False when they do not fit, including the NUL.
 * @param dst The buffer.
 * @param cap Its size.
 * @param n   Bytes already written, not counting a NUL.
 * @param src The bytes.
 * @param len How many.
 * @return The new length, or 0 when they do not fit. 0 is also a real length,
 *         so the caller treats a false return as failure via the bool.
 */
static bool append_raw(char *dst, size_t cap, size_t *n, const char *src,
                       size_t len)
{
    if ( (NULL == dst) || (NULL == n) || ((0U < len) && (NULL == src)) )
    {
        return false;
    }
    if ( (*n + len + 1U) > cap )
    {
        return false;
    }
    if ( 0U < len )
    {
        (void)memcpy(&dst[*n], src, len);
    }
    *n += len;
    dst[*n] = '\0';
    return true;
}

/** @brief Append a C string. */
static bool append_str(char *dst, size_t cap, size_t *n, const char *src)
{
    if ( NULL == src )
    {
        return false;
    }
    return append_raw(dst, cap, n, src, strlen(src));
}

/**
 * @brief Append one JSON string, quotes included, UTF-8 passed through.
 * @param text NULL is treated as empty.
 * @return false when it does not fit.
 */
static bool append_json_string(char *dst, size_t cap, size_t *n, const char *text)
{
    static const char hex[] = "0123456789abcdef";
    const unsigned char *p;

    if ( !append_raw(dst, cap, n, "\"", 1U) )
    {
        return false;
    }
    if ( NULL == text )
    {
        text = "";
    }
    p = (const unsigned char *)text;
    while ( '\0' != *p )
    {
        unsigned char c = *p;
        char          esc[8];
        size_t        elen = 0U;

        if ( ('\\' == c) || ('"' == c) )
        {
            esc[0] = '\\';
            esc[1] = (char)c;
            elen = 2U;
        }
        else if ( '\n' == c )
        {
            esc[0] = '\\';
            esc[1] = 'n';
            elen = 2U;
        }
        else if ( '\r' == c )
        {
            esc[0] = '\\';
            esc[1] = 'r';
            elen = 2U;
        }
        else if ( '\t' == c )
        {
            esc[0] = '\\';
            esc[1] = 't';
            elen = 2U;
        }
        else if ( 0x20U > c )
        {
            esc[0] = '\\';
            esc[1] = 'u';
            esc[2] = '0';
            esc[3] = '0';
            esc[4] = hex[c >> 4];
            esc[5] = hex[c & 0x0FU];
            elen = 6U;
        }
        else
        {
            esc[0] = (char)c;
            elen = 1U;
        }
        if ( !append_raw(dst, cap, n, esc, elen) )
        {
            return false;
        }
        p++;
    }
    return append_raw(dst, cap, n, "\"", 1U);
}

/**
 * @brief Whether @p role is one of the three names the body allows.
 * @param role The role text.
 * @return true for system, user or assistant.
 */
static bool role_ok(const char *role)
{
    if ( NULL == role )
    {
        return false;
    }
    return (0 == strcmp(role, "system"))
        || (0 == strcmp(role, "user"))
        || (0 == strcmp(role, "assistant"));
}

size_t grok_request_write(char *dst, size_t cap, const char *model,
                          const grok_msg_t *msgs, size_t nmsgs, bool stream)
{
    size_t n = 0U;
    size_t i;

    if ( (NULL == dst) || (0U == cap) || (NULL == model) || ('\0' == model[0]) )
    {
        if ( (NULL != dst) && (0U < cap) )
        {
            dst[0] = '\0';
        }
        return 0U;
    }
    if ( (0U < nmsgs) && (NULL == msgs) )
    {
        dst[0] = '\0';
        return 0U;
    }
    dst[0] = '\0';
    if ( !append_str(dst, cap, &n, "{\"model\":")
         || !append_json_string(dst, cap, &n, model)
         || !append_str(dst, cap, &n, ",\"stream\":")
         || !append_str(dst, cap, &n, stream ? "true" : "false")
         || !append_str(dst, cap, &n, ",\"messages\":[") )
    {
        dst[0] = '\0';
        return 0U;
    }
    for ( i = 0U; nmsgs > i; i++ )
    {
        if ( !role_ok(msgs[i].role) )
        {
            dst[0] = '\0';
            return 0U;
        }
        if ( (0U < i) && !append_raw(dst, cap, &n, ",", 1U) )
        {
            dst[0] = '\0';
            return 0U;
        }
        if ( !append_str(dst, cap, &n, "{\"role\":")
             || !append_json_string(dst, cap, &n, msgs[i].role)
             || !append_str(dst, cap, &n, ",\"content\":")
             || !append_json_string(dst, cap, &n, msgs[i].content)
             || !append_raw(dst, cap, &n, "}", 1U) )
        {
            dst[0] = '\0';
            return 0U;
        }
    }
    if ( !append_str(dst, cap, &n, "]}") )
    {
        dst[0] = '\0';
        return 0U;
    }
    return n;
}

size_t grok_request_fit(char *dst, size_t cap, const char *model,
                        const char *system, const grok_msg_t *history,
                        size_t nhist, const char *user)
{
    grok_msg_t built[GROK_REQUEST_TURNS_MAX];
    size_t     room;
    size_t     keep;
    bool       with_system;

    if ( (NULL == user) || ((0U < nhist) && (NULL == history)) )
    {
        if ( (NULL != dst) && (0U < cap) )
        {
            dst[0] = '\0';
        }
        return 0U;
    }
    with_system = (NULL != system) && ('\0' != system[0]);
    /* One slot is the new user turn. One more is the system turn when set. */
    room = GROK_REQUEST_TURNS_MAX - 1U;
    if ( with_system )
    {
        room--;
    }
    if ( nhist > room )
    {
        history = &history[nhist - room];
        nhist = room;
    }
    /* Drop the oldest history turn until the body fits. */
    for ( keep = nhist; ; keep-- )
    {
        size_t nbuilt = 0U;
        size_t i;
        size_t wrote;
        size_t start = nhist - keep;

        if ( with_system )
        {
            built[nbuilt].role = "system";
            built[nbuilt].content = system;
            nbuilt++;
        }
        for ( i = 0U; keep > i; i++ )
        {
            built[nbuilt] = history[start + i];
            nbuilt++;
        }
        built[nbuilt].role = "user";
        built[nbuilt].content = user;
        nbuilt++;
        wrote = grok_request_write(dst, cap, model, built, nbuilt, true);
        if ( 0U < wrote )
        {
            return wrote;
        }
        if ( 0U == keep )
        {
            break;
        }
    }
    if ( (NULL != dst) && (0U < cap) )
    {
        dst[0] = '\0';
    }
    return 0U;
}
