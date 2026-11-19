/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_request.h
 * @brief Build a Chat Completions JSON body. No SDK, no network.
 */
#ifndef GROK_REQUEST_H_
#define GROK_REQUEST_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- One turn ---- */

/** @brief Turns grok_request_fit() will put in one body, system and user included. */
#define GROK_REQUEST_TURNS_MAX  8U

/**
 * @brief One message. role is "system", "user" or "assistant".
 *
 * The pointers are borrowed for the call that reads them. They are not kept.
 */
typedef struct {
    const char *role;    /**< "system", "user" or "assistant". */
    const char *content; /**< UTF-8 text. NULL is an empty string. */
} grok_msg_t;

/* ---- Calls ---- */

/**
 * @brief Write one JSON body for POST /v1/chat/completions.
 *
 * Compact JSON. stream is the boolean the server uses to send SSE.
 * A role that is not one of the three names fails the whole body, so a
 * caller cannot put a quote into the role and break out of the string.
 *
 * @param dst    The body, NUL terminated when @p cap is above 0.
 * @param cap    Bytes @p dst holds.
 * @param model  The model name. NULL or empty fails.
 * @param msgs   The turns, in order. NULL only when @p nmsgs is 0.
 * @param nmsgs  How many turns.
 * @param stream true sends "stream":true.
 * @return The bytes written, not counting the NUL. 0 when it does not fit
 *         or an argument is wrong. On 0, @p dst is "" when @p cap is above 0.
 */
size_t grok_request_write(char *dst, size_t cap, const char *model,
                          const grok_msg_t *msgs, size_t nmsgs, bool stream);

/**
 * @brief The same body, keeping the newest history that still fits.
 *
 * Order is the system turn (skipped when @p system is NULL or empty), then
 * the newest @p keep turns of @p history, then @p user. @p history is oldest
 * first. The new user turn is never dropped. 0 means even that did not fit.
 *
 * @param dst     As grok_request_write().
 * @param cap     As grok_request_write().
 * @param model   As grok_request_write().
 * @param system  The system text, or NULL.
 * @param history Older turns, oldest first. NULL when @p nhist is 0.
 * @param nhist   How many older turns.
 * @param user    The new user text. NULL fails.
 * @return The bytes written, not counting the NUL. 0 on failure.
 */
size_t grok_request_fit(char *dst, size_t cap, const char *model,
                        const char *system, const grok_msg_t *history,
                        size_t nhist, const char *user);

#ifdef __cplusplus
}
#endif

#endif /* GROK_REQUEST_H_ */
