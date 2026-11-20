/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_prompts.h
 * @brief The canned prompts. A keyboard is not in this scaffold.
 */
#ifndef GROK_PROMPTS_H_
#define GROK_PROMPTS_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief One prompt: a short label for the glass, and the text that is sent. */
typedef struct {
    const char *label; /**< A few words. */
    const char *text;  /**< The user turn. */
} grok_prompt_t;

/**
 * @brief How many canned prompts there are.
 * @return The count. At least one.
 */
size_t grok_prompt_count(void);

/**
 * @brief The prompt at @p index, wrapped into range.
 * @param index Any index. It is reduced modulo the count.
 * @return The prompt. Never NULL.
 */
const grok_prompt_t *grok_prompt_at(size_t index);

/**
 * @brief What the glass says where a keyboard would be.
 * @return A fixed sentence. Never NULL.
 */
const char *grok_keyboard_hint(void);

#ifdef __cplusplus
}
#endif

#endif /* GROK_PROMPTS_H_ */
