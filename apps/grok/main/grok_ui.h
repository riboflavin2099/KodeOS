/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_ui.h
 * @brief The chat glass: a status line, the armed prompt, and the transcript.
 *
 * kode_screen_* can wait, so grok_ui_show() runs on the worker, not in an
 * Active Object step. The keyboard is a sentence. The prompts are the input.
 */
#ifndef GROK_UI_H_
#define GROK_UI_H_

#include <stdbool.h>
#include <stddef.h>

#include "grok_config.h"
#include "grok_net.h"
#include "grok_transcript.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write the one status line the glass shows under the title.
 * @param dst       The line. NUL terminated when @p cap is above 0.
 * @param cap       Bytes @p dst holds.
 * @param cfg       The config. May be NULL.
 * @param net       Whether the radio has an address.
 * @param have_file true when sd/grok/config.txt was read.
 * @param listening true while the microphone is open.
 * @param asking    true while a request is out.
 */
void grok_ui_status(char *dst, size_t cap, const grok_config_t *cfg,
                    grok_net_status_t net, bool have_file, bool listening,
                    bool asking);

/**
 * @brief Replace the glass with the current transcript.
 * @param status       The status line.
 * @param tr           The lines.
 * @param scroll       Lines above the tail. 0 follows the newest.
 * @param prompt_index Which canned prompt is armed.
 */
void grok_ui_show(const char *status, const grok_transcript_t *tr, int scroll,
                  size_t prompt_index);

#ifdef __cplusplus
}
#endif

#endif /* GROK_UI_H_ */
