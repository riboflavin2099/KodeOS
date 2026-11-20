/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_ao.c
 * @brief Buttons in, one worker for the card, the radio, the request and the glass.
 *
 * The worker blocks. This object does not. Holding the bottom button is
 * kodeOS home and is not given a meaning here.
 */
#include "app_ao.h"

#include "esp_heap_caps.h"
#include "grok_audio.h"
#include "grok_client.h"
#include "grok_config.h"
#include "grok_net.h"
#include "grok_prompts.h"
#include "grok_transcript.h"
#include "grok_ui.h"
#include "kode.h"
#include "kode_log.h"
#include "kode_worker.h"

/** @brief This file's log tag. */
#define TAG "grok.ao"

/** @brief The worker's task name. */
#define APP_WORKER_NAME  "grok_worker"

/** @brief The worker's stack, in PSRAM. TLS wants more than the 8 KB floor. */
#define APP_WORKER_STACK_BYTES  20480U

/** @brief Jobs that may wait besides the one running. This AO sends one. */
#define APP_WORKER_DEPTH  1U

/** @brief How long boot waits for an address the radio is already joining. */
#define APP_JOIN_POLLS  40U

/** @brief Pause between those polls, ms. */
#define APP_JOIN_STEP_MS  200U

/** @brief The path on the card. Data, not the firmware. */
#define APP_CONFIG_PATH  "sd/grok/config.txt"

/**
 * @brief The app's own signals. They are posted, never published.
 */
enum AppAoSignals {
    APP_JOB_DONE_SIG = MAX_SIG  /**< The worker returned. */
};

/** @brief What the worker was asked to do. */
enum AppJob {
    APP_JOB_BOOT = 1,     /**< Read the card, wait for an address, draw. */
    APP_JOB_CHAT,         /**< One completion from the transcript.        */
    APP_JOB_LISTEN,       /**< Open the microphone.                       */
    APP_JOB_LISTEN_END,   /**< Close it. Upload is not done here.         */
    APP_JOB_DRAW          /**< Draw again.                                */
};

/** @brief Where the buttons are being handled. */
enum AppPhase {
    APP_PHASE_BUSY = 0,   /**< A job is out. Buttons do not start another. */
    APP_PHASE_IDLE,       /**< A press may start a job.                     */
    APP_PHASE_LISTEN      /**< The microphone is open. Release ends it.    */
};

/**
 * @brief The Active Object's own state, private to this file.
 */
typedef struct {
    QActive        super;       /**< The QP/C base. Stays first.            */
    kode_worker_t  worker;      /**< The card, the radio and HTTP run here. */
    uint16_t       job_seq;     /**< The job this object is waiting for.    */
    uint8_t        phase;       /**< AppPhase.                              */
    uint8_t        job;         /**< AppJob, or 0 when none is out.         */
    bool           bot_down;    /**< Bottom is down, and was not a hold.    */
    bool           bot_held;    /**< Bottom reached the hold. Home owns it. */
    bool           stop_listen; /**< Top was released before listen returned. */
    int            scroll;      /**< Lines above the tail.                  */
    size_t         prompt;      /**< Which canned prompt is armed.          */
} AppAo;

/** @brief Its event queue. */
static QEvtPtr s_queue[KQP_AO_QUEUE_LEN];

/** @brief Its stack, in words. Internal RAM: an Active Object may write flash. */
static StackType_t s_stack[KQP_AO_STACK_BYTES / sizeof(StackType_t)];

/** @brief The one instance. */
static AppAo s_ao;

/** @brief Which job the worker should run. Set before submit, read by the job. */
static uint8_t s_job_kind;

/** @brief The card file, once parsed. The key lives here and is not logged. */
static grok_config_t s_cfg;

/** @brief The lines on the glass. */
static grok_transcript_t s_tr;

/** @brief True after a non-empty config file was parsed. */
static bool s_have_file;

/** @brief The last address check. */
static grok_net_status_t s_net;

/** @brief True while a completion is the job in flight. The status line reads it. */
static bool s_asking;

/** @brief True while the microphone is open. */
static bool s_listening;

/** @brief The answer, in PSRAM, taken on the first chat. */
static char *s_reply;

QActive * const AO_App = &s_ao.super;

static QState AppAo_initial(AppAo * const me, void const * const par);
static QState AppAo_running(AppAo * const me, QEvt const * const e);
static bool   app_submit(AppAo * const me, uint8_t kind);

/**
 * @brief Draw the current status and transcript. Blocks. Worker only.
 */
static void app_paint(void)
{
    char status[120];

    grok_ui_status(status, sizeof(status), &s_cfg, s_net, s_have_file,
                   s_listening, s_asking);
    grok_ui_show(status, &s_tr, s_ao.scroll, s_ao.prompt);
}

/**
 * @brief Parse the card into s_cfg. A missing file leaves the defaults.
 */
static void app_load_file(void)
{
    const char *text = kode_file_read(APP_CONFIG_PATH);
    bool        cut = false;

    if ( (NULL == text) || ('\0' == text[0]) )
    {
        return;
    }
    if ( !grok_config_parse(&s_cfg, text, &cut) )
    {
        return;
    }
    s_have_file = true;
    if ( cut )
    {
        KLOG_W(TAG, "a config value was cut to fit");
    }
    /* The key is not part of this line. */
    KLOG_I(TAG, "config model %s, key %s", s_cfg.model,
           s_cfg.has_key ? "set" : "missing");
}

/**
 * @brief The worker's only job function. It switches on s_job_kind.
 * @param arg   Unused. The kind is s_job_kind, set before the submit.
 * @param value Receives the kind, so the done event can be checked.
 * @return KODE_OK. The glass carries the failure, not this status.
 */
static kode_status_t app_job(void * const arg, uint32_t * const value)
{
    uint32_t i;

    (void)arg;
    *value = s_job_kind;
    if ( APP_JOB_BOOT == s_job_kind )
    {
        grok_config_defaults(&s_cfg);
        s_have_file = false;
        app_load_file();
        /* Autojoin may unmount the card while the C5 link comes up. Wait,
         * then read again if the first read found nothing. */
        for ( i = 0U; APP_JOIN_POLLS > i; i++ )
        {
            if ( kode_wifi_is_connected() )
            {
                break;
            }
            kode_delay(APP_JOIN_STEP_MS);
        }
        if ( !s_have_file )
        {
            app_load_file();
        }
        s_net = grok_net_ensure(&s_cfg);
        app_paint();
    }
    else if ( APP_JOB_CHAT == s_job_kind )
    {
        grok_ask_t ask;

        s_net = grok_net_now();
        if ( GROK_NET_UP != s_net )
        {
            (void)grok_transcript_add(&s_tr, GROK_TR_NOTE, "No network.");
        }
        else
        {
            if ( NULL == s_reply )
            {
                s_reply = heap_caps_malloc(GROK_REPLY_MAX,
                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            }
            if ( NULL == s_reply )
            {
                (void)grok_transcript_add(&s_tr, GROK_TR_NOTE, "No memory for the answer.");
            }
            else
            {
                ask = grok_client_ask(&s_cfg, &s_tr, s_reply, GROK_REPLY_MAX);
                (void)grok_transcript_add_text(&s_tr,
                                               (GROK_ASK_OK == ask)
                                                   ? GROK_TR_ASSIST
                                                   : GROK_TR_NOTE,
                                               s_reply);
            }
        }
        s_asking = false;
        s_net = grok_net_now();
        app_paint();
    }
    else if ( APP_JOB_LISTEN == s_job_kind )
    {
        s_listening = grok_audio_listen(GROK_AUDIO_PTT_MS);
        if ( !s_listening )
        {
            (void)grok_transcript_add(&s_tr, GROK_TR_NOTE, "Microphone did not start.");
        }
        app_paint();
    }
    else if ( APP_JOB_LISTEN_END == s_job_kind )
    {
        uint32_t ms = 0U;

        if ( grok_audio_listen_end(&ms) )
        {
            (void)grok_transcript_add(&s_tr, GROK_TR_NOTE,
                                      "Heard you. Speech upload is not in this scaffold.");
        }
        else
        {
            (void)grok_transcript_add(&s_tr, GROK_TR_NOTE, "The recording did not stop cleanly.");
        }
        s_listening = false;
        app_paint();
    }
    else
    {
        s_net = grok_net_now();
        app_paint();
    }
    return KODE_OK;
}

/**
 * @brief Queue one job. False when the worker did not take it.
 * @param me   The Active Object.
 * @param kind AppJob.
 */
static bool app_submit(AppAo * const me, uint8_t kind)
{
    kode_status_t rc;

    s_job_kind = kind;
    me->job = kind;
    rc = kode_worker_submit(&me->worker, &app_job, NULL, (enum_t)APP_JOB_DONE_SIG,
                            &me->job_seq);
    if ( KODE_OK != rc )
    {
        me->job = 0U;
        KLOG_W(TAG, "the worker took no job (status %d)", (int)rc);
        return false;
    }
    return true;
}

/**
 * @brief Arm the canned prompt and ask. Called only from the idle phase.
 * @param me The Active Object.
 */
static void app_send_prompt(AppAo * const me)
{
    const grok_prompt_t *prompt = grok_prompt_at(me->prompt);

    (void)grok_transcript_add(&s_tr, GROK_TR_USER, prompt->text);
    me->scroll = 0;
    s_asking = true;
    if ( app_submit(me, (uint8_t)APP_JOB_CHAT) )
    {
        me->phase = (uint8_t)APP_PHASE_BUSY;
    }
    else
    {
        s_asking = false;
        (void)grok_transcript_add(&s_tr, GROK_TR_NOTE, "Could not ask.");
    }
}

void AppAo_ctor(void)
{
    QActive_ctor(&s_ao.super, Q_STATE_CAST(&AppAo_initial));
    grok_transcript_init(&s_tr);
    grok_config_defaults(&s_cfg);
}

void AppAo_start(void)
{
    QActive_setAttr(AO_App, TASK_NAME_ATTR, "grok_ao");
    QACTIVE_START(AO_App, KODE_PRIO_USER_LOW, s_queue, Q_DIM(s_queue),
                  s_stack, sizeof(s_stack), (void *)0);
}

/**
 * @brief Start the worker and ask it to read the card.
 * @param me  The Active Object.
 * @param par Unused.
 * @return The transition to AppAo_running.
 */
static QState AppAo_initial(AppAo * const me, void const * const par)
{
    kode_status_t rc;

    (void)par;
    me->phase = (uint8_t)APP_PHASE_BUSY;
    QActive_subscribe(&me->super, BUTTON_EVT_SIG);
    QActive_subscribe(&me->super, BUTTON_HOLD_SIG);
    rc = kode_worker_start(&me->worker, &me->super, APP_WORKER_NAME,
                           APP_WORKER_STACK_BYTES, (uint8_t)KODE_PRIO_USER_LOW,
                           (uint8_t)APP_WORKER_DEPTH);
    if ( KODE_OK != rc )
    {
        KLOG_E(TAG, "the worker did not start (status %d)", (int)rc);
        me->phase = (uint8_t)APP_PHASE_IDLE;
    }
    else if ( !app_submit(me, (uint8_t)APP_JOB_BOOT) )
    {
        me->phase = (uint8_t)APP_PHASE_IDLE;
    }
    KLOG_I(TAG, "up");
    return Q_TRAN(&AppAo_running);
}

/**
 * @brief A finished job. A late answer is ignored.
 * @param me    The Active Object.
 * @param value The kind the job wrote.
 */
static void app_job_done(AppAo * const me, uint32_t value)
{
    (void)value;
    if ( (uint8_t)APP_JOB_LISTEN == me->job )
    {
        me->job = 0U;
        if ( !s_listening )
        {
            me->phase = (uint8_t)APP_PHASE_IDLE;
            me->stop_listen = false;
            return;
        }
        if ( me->stop_listen )
        {
            me->stop_listen = false;
            if ( !app_submit(me, (uint8_t)APP_JOB_LISTEN_END) )
            {
                me->phase = (uint8_t)APP_PHASE_IDLE;
                s_listening = false;
            }
            return;
        }
        me->phase = (uint8_t)APP_PHASE_LISTEN;
        return;
    }
    me->job = 0U;
    me->phase = (uint8_t)APP_PHASE_IDLE;
    s_asking = false;
}

/**
 * @brief One button edge while nothing is out.
 * @param me The Active Object.
 * @param be The edge.
 */
static void app_idle_button(AppAo * const me, ButtonEvt const * const be)
{
    size_t n = grok_prompt_count();

    if ( KODE_SYS_BUTTON_BOT == (uint32_t)be->id )
    {
        if ( be->pressed )
        {
            me->bot_down = true;
            me->bot_held = false;
        }
        else
        {
            if ( me->bot_down && !me->bot_held )
            {
                app_send_prompt(me);
            }
            me->bot_down = false;
        }
        return;
    }
    if ( !be->pressed )
    {
        return;
    }
    if ( KODE_SYS_BUTTON_TOP == (uint32_t)be->id )
    {
        if ( app_submit(me, (uint8_t)APP_JOB_LISTEN) )
        {
            me->phase = (uint8_t)APP_PHASE_BUSY;
            me->stop_listen = false;
        }
        return;
    }
    if ( KODE_SYS_BUTTON_UP == (uint32_t)be->id )
    {
        if ( me->scroll < (int)GROK_TR_LINES )
        {
            me->scroll++;
        }
    }
    else if ( KODE_SYS_BUTTON_DOWN == (uint32_t)be->id )
    {
        if ( 0 < me->scroll )
        {
            me->scroll--;
        }
    }
    else if ( KODE_SYS_BUTTON_LEFT == (uint32_t)be->id )
    {
        me->prompt = (0U == me->prompt) ? (n - 1U) : (me->prompt - 1U);
    }
    else if ( KODE_SYS_BUTTON_RIGHT == (uint32_t)be->id )
    {
        me->prompt = (me->prompt + 1U) % n;
    }
    else
    {
        return;
    }
    if ( !app_submit(me, (uint8_t)APP_JOB_DRAW) )
    {
        me->phase = (uint8_t)APP_PHASE_IDLE;
    }
    else
    {
        me->phase = (uint8_t)APP_PHASE_BUSY;
    }
}

/**
 * @brief The one state.
 * @param me The Active Object.
 * @param e  The event.
 * @return Q_HANDLED() for what it handles, Q_SUPER() otherwise.
 */
static QState AppAo_running(AppAo * const me, QEvt const * const e)
{
    QState status;

    switch ( e->sig )
    {
        case Q_ENTRY_SIG:
        {
            status = Q_HANDLED();
            break;
        }
        case APP_JOB_DONE_SIG:
        {
            kode_worker_done_t const * const done = Q_EVT_CAST(kode_worker_done_t);

            if ( me->job_seq == done->seq )
            {
                app_job_done(me, done->value);
            }
            status = Q_HANDLED();
            break;
        }
        case BUTTON_HOLD_SIG:
        {
            ButtonHoldEvt const * const hold = Q_EVT_CAST(ButtonHoldEvt);

            if ( KODE_SYS_BUTTON_BOT == (uint32_t)hold->id )
            {
                /* Home. Do not also send the armed prompt. */
                me->bot_held = true;
            }
            status = Q_HANDLED();
            break;
        }
        case BUTTON_EVT_SIG:
        {
            ButtonEvt const * const be = Q_EVT_CAST(ButtonEvt);

            if ( (uint8_t)APP_PHASE_IDLE == me->phase )
            {
                app_idle_button(me, be);
            }
            else if ( (uint8_t)APP_PHASE_LISTEN == me->phase )
            {
                if ( !be->pressed && (KODE_SYS_BUTTON_TOP == (uint32_t)be->id) )
                {
                    if ( app_submit(me, (uint8_t)APP_JOB_LISTEN_END) )
                    {
                        me->phase = (uint8_t)APP_PHASE_BUSY;
                    }
                }
            }
            else if ( !be->pressed && (KODE_SYS_BUTTON_TOP == (uint32_t)be->id)
                      && ((uint8_t)APP_JOB_LISTEN == me->job) )
            {
                me->stop_listen = true;
            }
            else if ( (KODE_SYS_BUTTON_UP == (uint32_t)be->id) && be->pressed )
            {
                if ( me->scroll < (int)GROK_TR_LINES )
                {
                    me->scroll++;
                }
            }
            else if ( (KODE_SYS_BUTTON_DOWN == (uint32_t)be->id) && be->pressed )
            {
                if ( 0 < me->scroll )
                {
                    me->scroll--;
                }
            }
            status = Q_HANDLED();
            break;
        }
        default:
        {
            status = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status;
}
