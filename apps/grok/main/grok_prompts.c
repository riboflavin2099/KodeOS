/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file grok_prompts.c
 * @brief Three prompts. Left and right on the pad move between them.
 */
#include "grok_prompts.h"

/** @brief The prompts, in pad order. */
static const grok_prompt_t s_prompts[] = {
    { "Hello", "Say hello in one sentence." },
    { "Kode Dot", "What is a Kode Dot, in two short sentences?" },
    { "Joke", "Tell me a very short joke." }
};

size_t grok_prompt_count(void)
{
    return sizeof(s_prompts) / sizeof(s_prompts[0]);
}

const grok_prompt_t *grok_prompt_at(size_t index)
{
    size_t n = grok_prompt_count();

    return &s_prompts[index % n];
}

const char *grok_keyboard_hint(void)
{
    return "No keyboard yet. Left and right pick a prompt.";
}
