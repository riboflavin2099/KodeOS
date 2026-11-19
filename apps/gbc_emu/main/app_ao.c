/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_ao.c
 * @brief Starts the frame task and asks the worker to load a ROM.
 */
#include "app_ao.h"

#include "gbc_core.h"
#include "gbc_frame.h"
#include "gbc_rom.h"
#include "gbc_save.h"
#include "kode_worker.h"

/** @brief This file's log tag. */
#define TAG "gbc.ao"

/** @brief The worker's task name. */
#define APP_WORKER_NAME  "gbc_worker"

/** @brief The worker's stack, in PSRAM. The catalogue's floor is 8192. */
#define APP_WORKER_STACK_BYTES  8192U

/** @brief Jobs that may wait besides the one running. */
#define APP_WORKER_DEPTH  1U

/**
 * @brief The app's own signals. They start at MAX_SIG and are posted, never published.
 */
enum AppAoSignals {
    APP_JOB_DONE_SIG = MAX_SIG, /**< The ROM load returned. */
    APP_SAVE_SIG,               /**< The frame task wants a battery write. */
    APP_SAVE_DONE_SIG           /**< The save job returned. */
};

/**
 * @brief The Active Object's own state, private to this file.
 */
typedef struct {
    QActive super;            /**< The QP/C base. Stays first. */
    kode_worker_t worker;     /**< File reads run here, off the Active Object's step. */
    uint16_t job_seq;         /**< The sequence number of the load this object waits for. */
    uint16_t save_seq;        /**< The sequence number of the save it waits for. */
    bool     save_busy;       /**< A save job has been submitted and not answered. */
} AppAo;

/** @brief Posted from the frame task. Static, so the post does not allocate. */
static QEvt const s_save_req = QEVT_INITIALIZER(APP_SAVE_SIG);

/**
 * @brief Write cart RAM and the clock. Runs on the worker.
 */
static kode_status_t save_job(void *arg, uint32_t *value)
{
    uint32_t ram_len = 0U;
    uint8_t *ram = NULL;
    const char *name = NULL;
    int64_t clock = 0;
    kode_status_t rc = KODE_OK;

    (void)arg;
    if ( NULL == value )
    {
        return KODE_DENIED;
    }
    *value = 0U;
    if ( !gbc_core_save_pending() )
    {
        return KODE_OK;
    }
    name = gbc_rom_name();
    ram = gbc_core_cart_ram(&ram_len);
    clock = gbc_core_clock();
    if ( NULL == name )
    {
        rc = KODE_DENIED;
    }
    else
    {
        rc = gbc_save_store(name, ram, ram_len, clock);
        if ( KODE_OK == rc )
        {
            gbc_core_save_clear();
            *value = ram_len;
        }
    }
    return rc;
}

/** @brief Its event queue. */
static QEvtPtr s_queue[KQP_AO_QUEUE_LEN];

/** @brief Its stack, in words. Internal RAM: an Active Object may write flash. */
static StackType_t s_stack[KQP_AO_STACK_BYTES / sizeof(StackType_t)];

/** @brief The one instance. */
static AppAo s_ao;

QActive * const AO_App = &s_ao.super;

static QState AppAo_initial(AppAo * const me, void const * const par);
static QState AppAo_running(AppAo * const me, QEvt const * const e);

void AppAo_ctor(void)
{
    QActive_ctor(&s_ao.super, Q_STATE_CAST(&AppAo_initial));
}

void AppAo_request_save(void)
{
    bool const posted = (bool)QACTIVE_POST_X(AO_App, &s_save_req, 1U, (void *)0);

    (void)posted;
}

void AppAo_start(void)
{
    QActive_setAttr(AO_App, TASK_NAME_ATTR, "gbc_ao");
    QACTIVE_START(AO_App, KODE_PRIO_USER_LOW, s_queue, Q_DIM(s_queue),
                  s_stack, sizeof(s_stack), (void *)0);
}

/**
 * @brief Subscribe, start the worker and the frame task, then ask for a ROM.
 * @param me  The Active Object.
 * @param par Unused.
 * @return The transition to AppAo_running.
 */
static QState AppAo_initial(AppAo * const me, void const * const par)
{
    kode_status_t rc;

    (void)par;
    rc = kode_worker_start(&me->worker, &me->super, APP_WORKER_NAME,
                           APP_WORKER_STACK_BYTES, (uint8_t)KODE_PRIO_USER_LOW,
                           (uint8_t)APP_WORKER_DEPTH);
    if ( KODE_OK != rc )
    {
        KLOG_E(TAG, "the worker did not start (status %d)", (int)rc);
    }
    else
    {
        rc = kode_worker_submit(&me->worker, &gbc_rom_boot_job, NULL,
                                (enum_t)APP_JOB_DONE_SIG, &me->job_seq);
        if ( KODE_OK != rc )
        {
            KLOG_W(TAG, "the worker took no job (status %d)", (int)rc);
        }
    }
    if ( !gbc_frame_start() )
    {
        KLOG_E(TAG, "no frame task");
    }
    KLOG_I(TAG, "up");
    return Q_TRAN(&AppAo_running);
}

/**
 * @brief The one state: note the load and ignore a late answer.
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
            KqpWorkerDoneEvt const * const done = Q_EVT_CAST(KqpWorkerDoneEvt);

            if ( me->job_seq == done->seq )
            {
                KLOG_I(TAG, "ROM job done: status %u, bytes %lu",
                       (unsigned)done->status, (unsigned long)done->value);
            }
            status = Q_HANDLED();
            break;
        }
        case APP_SAVE_SIG:
        {
            kode_status_t rc = KODE_OK;

            if ( !me->save_busy )
            {
                rc = kode_worker_submit(&me->worker, &save_job, NULL,
                                        (enum_t)APP_SAVE_DONE_SIG, &me->save_seq);
                if ( KODE_OK == rc )
                {
                    me->save_busy = true;
                }
            }
            status = Q_HANDLED();
            break;
        }
        case APP_SAVE_DONE_SIG:
        {
            KqpWorkerDoneEvt const * const done = Q_EVT_CAST(KqpWorkerDoneEvt);

            if ( me->save_seq == done->seq )
            {
                me->save_busy = false;
                if ( KODE_OK != done->status )
                {
                    KLOG_W(TAG, "save job status %u", (unsigned)done->status);
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
