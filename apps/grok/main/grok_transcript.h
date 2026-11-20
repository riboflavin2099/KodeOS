/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_transcript.h
 * @brief The lines on the glass. Oldest drops off when the ring is full.
 */
#ifndef GROK_TRANSCRIPT_H_
#define GROK_TRANSCRIPT_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- The ring ---- */

/** @brief How many lines are kept. */
#define GROK_TR_LINES  8U

/** @brief Bytes of one line, NUL included. The glass shows fewer than this. */
#define GROK_TR_CHARS  120U

/** @brief Who the line is from. */
typedef enum {
    GROK_TR_USER = 1, /**< The person.     */
    GROK_TR_ASSIST,   /**< The model.      */
    GROK_TR_NOTE      /**< A status, not a turn the model is shown. */
} grok_tr_role_t;

/** @brief One line. */
typedef struct {
    grok_tr_role_t role;               /**< Who said it.          */
    char           text[GROK_TR_CHARS];/**< Cut to fit. NUL ended. */
} grok_tr_line_t;

/** @brief The ring. lines[0] is the oldest. */
typedef struct {
    grok_tr_line_t lines[GROK_TR_LINES]; /**< Oldest at index 0. */
    size_t         count;                /**< 0 to GROK_TR_LINES. */
} grok_transcript_t;

/* ---- Calls ---- */

/**
 * @brief Empty the ring.
 * @param t The transcript. NULL is ignored.
 */
void grok_transcript_init(grok_transcript_t *t);

/**
 * @brief Add a line. The oldest is dropped when the ring is full.
 * @param t    The transcript.
 * @param role Who said it.
 * @param text The text. NULL adds an empty line.
 * @return false when @p t is NULL.
 */
bool grok_transcript_add(grok_transcript_t *t, grok_tr_role_t role, const char *text);

/**
 * @brief Add @p text as whole lines, cutting each one to the line width.
 *
 * A short text is one line. A longer text is several lines, in order, each
 * the same role. The ring still drops the oldest when it fills.
 *
 * @param t    The transcript.
 * @param role Who said it.
 * @param text The text. NULL or empty adds one empty line.
 * @return false when @p t is NULL.
 */
bool grok_transcript_add_text(grok_transcript_t *t, grok_tr_role_t role,
                              const char *text);

/**
 * @brief Add @p text to the last line when that line has @p role. Otherwise add one.
 * @param t    The transcript.
 * @param role The role the tail must already have.
 * @param text The extra text.
 * @return false when @p t is NULL.
 */
bool grok_transcript_append(grok_transcript_t *t, grok_tr_role_t role, const char *text);

/**
 * @brief Point @p out at the lines to draw, oldest first.
 *
 * @p scroll 0 is the tail. A larger scroll moves the window toward older lines.
 *
 * @param t       The transcript.
 * @param max     How many pointers @p out holds.
 * @param scroll  Lines above the tail. Negative is treated as 0.
 * @param out     Filled with pointers into @p t. NULL is refused.
 * @param clamped Out, may be NULL: the scroll that was actually used.
 * @return How many pointers were written.
 */
size_t grok_transcript_window(const grok_transcript_t *t, size_t max, int scroll,
                              const grok_tr_line_t **out, int *clamped);

#ifdef __cplusplus
}
#endif

#endif /* GROK_TRANSCRIPT_H_ */
