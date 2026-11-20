/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_sse.h
 * @brief Incremental parser for one xAI chat stream.
 *
 * Two shapes, both from docs.x.ai on 2026-11-19:
 *   Chat Completions: a `data:` line whose JSON has choices[].delta.content,
 *   then `data: [DONE]`.
 *   Responses: an event response.output_text.delta whose JSON "delta" is a
 *   string.
 * reasoning_content is reported on its own and is not the answer text.
 * One SSE event over GROK_SSE_PAYLOAD_MAX bytes is an error and is skipped.
 */
#ifndef GROK_SSE_H_
#define GROK_SSE_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Limits ---- */

/** @brief Bytes of one assembled SSE event, NUL not stored in the payload. */
#define GROK_SSE_PAYLOAD_MAX  2048U

/** @brief Bytes of one text delta this parser hands back, NUL included. */
#define GROK_SSE_DELTA_MAX     480U

/** @brief Bytes of an SSE event name, NUL included. */
#define GROK_SSE_EVENT_MAX      64U

/* ---- What one event was ---- */

/** @brief The kind of one completed SSE event. */
typedef enum {
    GROK_SSE_TEXT = 1, /**< Answer text. text holds the unescaped delta. */
    GROK_SSE_REASON,   /**< Reasoning text. Not the answer.               */
    GROK_SSE_DONE,     /**< The server said the turn is over.             */
    GROK_SSE_ERROR     /**< A server error object, or an event past the cap. */
} grok_sse_kind_t;

/** @brief One event. text is NUL terminated. Empty when the kind has none. */
typedef struct {
    grok_sse_kind_t kind;                    /**< Which event.            */
    bool            clipped;                 /**< The delta was cut to fit. */
    char            text[GROK_SSE_DELTA_MAX];/**< The unescaped delta.    */
} grok_sse_event_t;

/**
 * @brief Called once per event. Return false to stop feeding.
 * @param ev  The event. Valid only for the call.
 * @param ctx What grok_sse_feed() was given.
 * @return false to stop.
 */
typedef bool (*grok_sse_fn)(const grok_sse_event_t *ev, void *ctx);

/** @brief Parser state. About 4 KB. Keep it off an internal-RAM stack. */
typedef struct {
    char     line[GROK_SSE_PAYLOAD_MAX];    /**< The line being assembled. */
    size_t   line_len;                      /**< Bytes in line, no NUL.    */
    char     payload[GROK_SSE_PAYLOAD_MAX]; /**< data: lines joined so far. */
    size_t   payload_len;                   /**< Bytes in payload, no NUL. */
    char     event_name[GROK_SSE_EVENT_MAX];/**< The current event: name.  */
    bool     overflow;                      /**< This event is past the cap. */
    bool     skip_line;                     /**< Drop the rest of a long line. */
} grok_sse_t;

/* ---- Calls ---- */

/**
 * @brief Clear the parser.
 * @param p The parser. NULL is ignored.
 */
void grok_sse_init(grok_sse_t *p);

/**
 * @brief Feed the next bytes of the body.
 *
 * A `data:` line is stored. A blank line ends the event and may call @p fn
 * once or twice (reasoning, then text). Return is false when @p fn returned
 * false or an argument is NULL.
 *
 * @param p    The parser.
 * @param data The bytes. NULL is refused when @p n is above 0.
 * @param n    How many.
 * @param fn   The callback. NULL is refused.
 * @param ctx  Passed to @p fn.
 * @return false to say "stop reading".
 */
bool grok_sse_feed(grok_sse_t *p, const char *data, size_t n, grok_sse_fn fn,
                   void *ctx);

/**
 * @brief End the body. A payload with no trailing blank line is still parsed.
 * @param p  The parser.
 * @param fn The callback. NULL is refused.
 * @param ctx Passed to @p fn.
 * @return false when @p fn returned false or an argument is NULL.
 */
bool grok_sse_flush(grok_sse_t *p, grok_sse_fn fn, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* GROK_SSE_H_ */
