/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file test_grok.c
 * @brief Host checks for the request builder, the SSE parser and the config.
 *        No SDK, no network, no API key.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "grok_config.h"
#include "grok_prompts.h"
#include "grok_request.h"
#include "grok_sse.h"
#include "grok_transcript.h"

static int s_failed = 0;

/**
 * @brief Count one failure.
 * @param cond True when the check passed.
 * @param what What was checked.
 */
static void expect(int cond, const char *what)
{
    if ( !cond )
    {
        fprintf(stderr, "FAIL %s\n", what);
        s_failed++;
    }
}

/** @brief What a parse collected. */
typedef struct {
    char   text[1024];
    char   reason[GROK_SSE_DELTA_MAX];
    char   error[GROK_SSE_DELTA_MAX];
    int    done;
    size_t text_len;
    size_t events;
} collect_t;

/**
 * @brief Record one SSE event.
 * @return true to keep reading.
 */
static bool on_event(const grok_sse_event_t *ev, void *ctx)
{
    collect_t *c = (collect_t *)ctx;

    c->events++;
    if ( GROK_SSE_TEXT == ev->kind )
    {
        size_t n = strlen(ev->text);

        if ( (c->text_len + n) < sizeof(c->text) )
        {
            (void)memcpy(&c->text[c->text_len], ev->text, n + 1U);
            c->text_len += n;
        }
    }
    else if ( GROK_SSE_REASON == ev->kind )
    {
        (void)snprintf(c->reason, sizeof(c->reason), "%s", ev->text);
    }
    else if ( GROK_SSE_ERROR == ev->kind )
    {
        (void)snprintf(c->error, sizeof(c->error), "%s", ev->text);
    }
    else if ( GROK_SSE_DONE == ev->kind )
    {
        c->done++;
    }
    return true;
}

/**
 * @brief Read a sample and feed it in steps of @p step bytes.
 * @param path The file, relative to the working directory.
 * @param step Bytes per feed. 0 feeds the whole file at once.
 * @param out  Cleared, then filled.
 * @return false when the file cannot be read.
 */
static int feed_file(const char *path, size_t step, collect_t *out)
{
    FILE       *f;
    char       *buf;
    long        len;
    grok_sse_t  parser;
    size_t      off = 0U;

    (void)memset(out, 0, sizeof(*out));
    f = fopen(path, "rb");
    if ( NULL == f )
    {
        fprintf(stderr, "FAIL open %s\n", path);
        s_failed++;
        return 0;
    }
    if ( 0 != fseek(f, 0, SEEK_END) )
    {
        fclose(f);
        return 0;
    }
    len = ftell(f);
    if ( 0 > len )
    {
        fclose(f);
        return 0;
    }
    if ( 0 != fseek(f, 0, SEEK_SET) )
    {
        fclose(f);
        return 0;
    }
    buf = (char *)malloc((size_t)len + 1U);
    if ( NULL == buf )
    {
        fclose(f);
        return 0;
    }
    if ( (size_t)len != fread(buf, 1, (size_t)len, f) )
    {
        free(buf);
        fclose(f);
        return 0;
    }
    fclose(f);
    buf[len] = '\0';
    grok_sse_init(&parser);
    if ( 0U == step )
    {
        step = (size_t)len;
    }
    while ( (size_t)len > off )
    {
        size_t n = step;

        if ( ((size_t)len - off) < n )
        {
            n = (size_t)len - off;
        }
        if ( !grok_sse_feed(&parser, &buf[off], n, &on_event, out) )
        {
            free(buf);
            return 0;
        }
        off += n;
    }
    if ( !grok_sse_flush(&parser, &on_event, out) )
    {
        free(buf);
        return 0;
    }
    free(buf);
    return 1;
}

/**
 * @brief The request body and the history fitter.
 */
static void test_request(void)
{
    char       body[512];
    grok_msg_t msg;
    grok_msg_t hist[3];
    size_t     n;
    const char *user_hi = "Hi";

    msg.role = "user";
    msg.content = user_hi;
    n = grok_request_write(body, sizeof(body), "grok-4.3", &msg, 1U, true);
    expect(0U < n, "simple body has a length");
    expect(0 == strcmp(body,
                       "{\"model\":\"grok-4.3\",\"stream\":true,\"messages\":"
                       "[{\"role\":\"user\",\"content\":\"Hi\"}]}"),
           "simple body bytes");

    msg.content = "say \"hi\"\n";
    n = grok_request_write(body, sizeof(body), "m", &msg, 1U, true);
    expect(NULL != strstr(body, "say \\\"hi\\\"\\n"), "quotes and newline are escaped");

    msg.role = "system\"";
    n = grok_request_write(body, sizeof(body), "m", &msg, 1U, true);
    expect(0U == n, "a role with a quote is refused");
    expect('\0' == body[0], "a refused body is empty");

    msg.role = "user";
    msg.content = "Hi";
    n = grok_request_write(body, 16U, "grok-4.3", &msg, 1U, true);
    expect(0U == n, "a short buffer is refused");

    hist[0].role = "user";
    hist[0].content = "OLDEST_SHOULD_DROP";
    hist[1].role = "assistant";
    hist[1].content = "kept-middle";
    hist[2].role = "user";
    hist[2].content = "kept-late";
    n = grok_request_fit(body, 220U, "grok-4.3", "Be brief.", hist, 3U, "NEW");
    expect(0U < n, "fit writes a body");
    expect(NULL == strstr(body, "OLDEST_SHOULD_DROP"), "fit drops the oldest turn");
    expect(NULL != strstr(body, "NEW"), "fit keeps the new user turn");
    expect(NULL != strstr(body, "Be brief."), "fit keeps the system turn");
}

/**
 * @brief The card file.
 */
static void test_config(void)
{
    grok_config_t cfg;
    char          url[200];
    bool          cut = false;
    char          long_key[GROK_CFG_KEY_MAX + 32U];
    char          text[400];
    size_t        i;

    grok_config_defaults(&cfg);
    expect(grok_config_url(&cfg, url, sizeof(url)), "default url builds");
    expect(0 == strcmp(url, "https://api.x.ai/v1/chat/completions"), "default url");
    expect(!cfg.has_key, "default has no key");

    expect(grok_config_parse(&cfg,
                             "# comment\n\n"
                             "base_url = https://api.x.ai/v1/\n"
                             "model = grok-4.7\n"
                             "api_key = test-key-not-real\n"
                             "unknown = no\n",
                             &cut),
           "parse accepts a file");
    expect(!cut, "the sample file fits");
    expect(cfg.has_key, "a key sets has_key");
    expect(0 == strcmp(cfg.api_key, "test-key-not-real"), "the key is stored");
    expect(0 == strcmp(cfg.model, "grok-4.7"), "model overlays");
    expect(0 == strcmp(cfg.base_url, "https://api.x.ai/v1"), "trailing slash is dropped");
    expect(grok_config_url(&cfg, url, sizeof(url)), "url after overlay");
    expect(NULL == strstr(url, "test-key"), "the key is not in the url");
    expect(0 == strcmp(url, "https://api.x.ai/v1/chat/completions"), "path stays");

    grok_config_defaults(&cfg);
    expect(grok_config_parse(&cfg,
                             "base_url=http://192.168.1.20:8787/v1\n"
                             "api_key=\n",
                             NULL),
           "a relay base parses");
    expect(!cfg.has_key, "an empty key is no key");
    expect(grok_config_url(&cfg, url, sizeof(url)), "relay url builds");
    expect(0 == strcmp(url, "http://192.168.1.20:8787/v1/chat/completions"),
           "relay url");

    for ( i = 0U; (GROK_CFG_KEY_MAX + 8U) > i; i++ )
    {
        long_key[i] = 'k';
    }
    long_key[i] = '\0';
    (void)snprintf(text, sizeof(text), "api_key=%s\n", long_key);
    grok_config_defaults(&cfg);
    expect(grok_config_parse(&cfg, text, &cut), "a long key parses");
    expect(cut, "a long key is reported as cut");
    expect(cfg.has_key, "a cut key is still a key");
    expect((GROK_CFG_KEY_MAX - 1U) == strlen(cfg.api_key), "the key was cut to the field");
}

/**
 * @brief The recorded streams, fed whole and one byte at a time.
 */
static void test_sse(void)
{
    collect_t whole;
    collect_t bytes;

    expect(feed_file("host/samples/chat_ok.txt", 0U, &whole), "chat sample reads");
    expect(0 == strcmp(whole.reason, "Think"), "reasoning is separate");
    expect(0 == strcmp(whole.text, "Ah, there."), "content deltas join");
    expect(1 == whole.done, "chat stream ends");
    expect('\0' == whole.error[0], "chat stream is not an error");

    expect(feed_file("host/samples/chat_ok.txt", 1U, &bytes), "chat sample feeds by byte");
    expect(0 == strcmp(bytes.text, whole.text), "one-byte feed matches");
    expect(bytes.done == whole.done, "one-byte feed ends");
    expect(0 == strcmp(bytes.reason, "Think"), "one-byte feed keeps reasoning");

    expect(feed_file("host/samples/chat_escape.txt", 3U, &whole), "escape sample reads");
    expect(0 == strcmp(whole.text, "say \"hi\"\nnext \xc3\xa9"),
           "escapes and u00e9 decode");

    expect(feed_file("host/samples/chat_reason.txt", 0U, &whole), "reason sample reads");
    expect(0 == strcmp(whole.reason, "content"), "reasoning_content is not the answer");
    expect(0 == strcmp(whole.text, "Answer"), "content is the answer");

    expect(feed_file("host/samples/responses_ok.txt", 1U, &whole), "responses sample reads");
    expect(0 == strcmp(whole.text, "Hello there"), "responses deltas join");
    expect(1 == whole.done, "response.completed ends the turn");

    expect(feed_file("host/samples/error_key.txt", 0U, &whole), "error sample reads");
    expect(0 == strcmp(whole.error, "Incorrect API key"), "error message is kept");
    expect(0 == whole.done, "an error is not done");
    expect('\0' == whole.text[0], "an error has no answer text");
}

/**
 * @brief The ring the glass draws from.
 */
static void test_transcript(void)
{
    grok_transcript_t      tr;
    const grok_tr_line_t  *win[4];
    int                    used = -1;
    size_t                 n;
    char                   label[8];
    int                    i;

    grok_transcript_init(&tr);
    for ( i = 0; 9 > i; i++ )
    {
        (void)snprintf(label, sizeof(label), "L%d", i);
        expect(grok_transcript_add(&tr, GROK_TR_USER, label), "add a line");
    }
    expect(GROK_TR_LINES == tr.count, "the ring stays full");
    expect(0 == strcmp(tr.lines[0].text, "L1"), "the oldest line dropped");
    expect(grok_transcript_append(&tr, GROK_TR_USER, "!"), "append to the tail");
    expect(0 == strcmp(tr.lines[tr.count - 1U].text, "L8!"), "append joined");
    expect(grok_transcript_add(&tr, GROK_TR_ASSIST, "yes"), "an assistant line");
    expect(grok_transcript_append(&tr, GROK_TR_ASSIST, " yes"), "append to that role");
    expect(0 == strcmp(tr.lines[tr.count - 1U].text, "yes yes"), "assistant append joined");

    n = grok_transcript_window(&tr, 2U, 0, win, &used);
    expect(2U == n, "window of two");
    expect(0 == used, "tail scroll is zero");
    expect(0 == strcmp(win[1]->text, "yes yes"), "the tail is last");
    n = grok_transcript_window(&tr, 2U, 100, win, &used);
    expect(2U == n, "a large scroll still returns a window");
    expect(0 == strcmp(win[0]->text, "L2"), "scroll clamps at the oldest");

    {
        char   long_text[140];
        size_t k;

        grok_transcript_init(&tr);
        for ( k = 0U; 130U > k; k++ )
        {
            long_text[k] = 'a';
        }
        long_text[130] = '\0';
        expect(grok_transcript_add_text(&tr, GROK_TR_ASSIST, long_text),
               "a long answer is stored");
        expect(2U == tr.count, "a long answer takes two lines");
        expect((GROK_TR_CHARS - 1U) == strlen(tr.lines[0].text),
               "the first line is full");
    }
}

int main(void)
{
    test_request();
    test_config();
    test_sse();
    test_transcript();
    expect(3U == grok_prompt_count(), "three canned prompts");
    expect(NULL != grok_prompt_at(0), "a prompt exists");
    expect(grok_prompt_at(0) == grok_prompt_at(grok_prompt_count()),
           "the prompt index wraps");
    expect(NULL != grok_keyboard_hint(), "the keyboard hint is a sentence");

    if ( 0 == s_failed )
    {
        printf("OK grok\n");
        return 0;
    }
    fprintf(stderr, "%d failed\n", s_failed);
    return 1;
}
