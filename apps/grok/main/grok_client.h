/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_client.h
 * @brief One chat turn: build the JSON, POST it, collect the answer text.
 */
#ifndef GROK_CLIENT_H_
#define GROK_CLIENT_H_

#include <stddef.h>

#include "grok_config.h"
#include "grok_transcript.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Bytes of answer text kept, NUL included. The glass wraps this. */
#define GROK_REPLY_MAX  2048U

/** @brief How the turn ended. The reply buffer still holds a sentence. */
typedef enum {
    GROK_ASK_OK = 0,    /**< Answer text is in the buffer.          */
    GROK_ASK_NOTE,      /**< No request was sent. The buffer says why. */
    GROK_ASK_HTTP       /**< The server or the link failed.         */
} grok_ask_t;

/**
 * @brief Ask with the transcript's turns. The last user line is the question.
 *
 * Notes are not sent. Runs on a worker: it blocks on the network. The key is
 * not logged. A base of api.x.ai with no key does not call the network.
 *
 * @param cfg   The config. The key may be empty for a relay.
 * @param tr    The transcript, including the new user line.
 * @param reply Out: the answer or a one-line reason. NUL terminated.
 * @param cap   Bytes @p reply holds. Below 2 is refused.
 * @return GROK_ASK_OK when answer text arrived.
 */
grok_ask_t grok_client_ask(const grok_config_t *cfg, const grok_transcript_t *tr,
                           char *reply, size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* GROK_CLIENT_H_ */
