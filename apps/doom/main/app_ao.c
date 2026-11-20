/* SPDX-License-Identifier: GPL-3.0-or-later */
/**
 * @file app_ao.c
 * @brief Starts the frame task and the worker that reads the card.
 */
#include "app_ao.h"

#include "doom_file.h"
#include "doom_frame.h"
#include "doom_menu.h"
#include "doom_save.h"
#include "kode_log.h"
#include "kode_worker.h"

#define TAG "doom.ao"

#define APP_WORKER_NAME         "doom_worker"
#define APP_WORKER_STACK_BYTES  8192U
#define APP_WORKER_DEPTH        1U

enum AppAoSignals {
    APP_JOB_DONE_SIG = MAX_SIG,
    APP_PICK_SIG,
    APP_RESCAN_SIG
};

typedef struct {
    QActive super;
    kode_worker_t worker;
    uint16_t job_seq;
    bool busy;
    bool want_scan;
    bool want_save;
} AppAo;

static QEvtPtr s_queue[KQP_AO_QUEUE_LEN];
static StackType_t s_stack[KQP_AO_STACK_BYTES / sizeof(StackType_t)];
static AppAo s_ao;
static QEvt s_pick_evt;
static QEvt s_rescan_evt;
static bool s_evts_ready = false;

QActive * const AO_App = &s_ao.super;

static QState AppAo_initial(AppAo * const me, void const * const par);
static QState AppAo_running(AppAo * const me, QEvt const * const e);

static kode_status_t save_job(void *arg, uint32_t *value)
{
    char name[64];
    kode_status_t st = KODE_OK;

    (void)arg;
    if ( NULL != value )
    {
        *value = 0U;
    }
    if ( doom_menu_copy_pick(name, sizeof(name)) )
    {
        st = doom_save_store(name);
        if ( (KODE_OK == st) && (NULL != value) )
        {
            *value = 1U;
        }
    }
    return st;
}

static void pump(AppAo * const me)
{
    if ( me->busy )
    {
        return;
    }
    if ( me->want_scan )
    {
        me->want_scan = false;
        if ( KODE_OK == kode_worker_submit(&me->worker, &doom_file_boot_job, NULL,
                                           (enum_t)APP_JOB_DONE_SIG, &me->job_seq) )
        {
            me->busy = true;
        }
    }
    else if ( me->want_save )
    {
        me->want_save = false;
        if ( KODE_OK == kode_worker_submit(&me->worker, &save_job, NULL,
                                           (enum_t)APP_JOB_DONE_SIG, &me->job_seq) )
        {
            me->busy = true;
        }
    }
}

void AppAo_ctor(void)
{
    QActive_ctor(&s_ao.super, Q_STATE_CAST(&AppAo_initial));
}

void AppAo_start(void)
{
    if ( !s_evts_ready )
    {
        QEvt_ctor(&s_pick_evt, (enum_t)APP_PICK_SIG);
        QEvt_ctor(&s_rescan_evt, (enum_t)APP_RESCAN_SIG);
        s_evts_ready = true;
    }
    QActive_setAttr(AO_App, TASK_NAME_ATTR, "doom_ao");
    QACTIVE_START(AO_App, KODE_PRIO_USER_LOW, s_queue, Q_DIM(s_queue),
                  s_stack, sizeof(s_stack), (void *)0);
}

void AppAo_post_pick(void)
{
    QACTIVE_POST(AO_App, &s_pick_evt, 0U);
}

void AppAo_post_rescan(void)
{
    QACTIVE_POST(AO_App, &s_rescan_evt, 0U);
}

static void queue_job(AppAo * const me, bool scan)
{
    if ( scan )
    {
        me->want_scan = true;
    }
    else
    {
        me->want_save = true;
    }
    pump(me);
}

static QState AppAo_initial(AppAo * const me, void const * const par)
{
    kode_status_t rc;

    (void)par;
    me->busy = false;
    me->want_scan = false;
    me->want_save = false;
    rc = kode_worker_start(&me->worker, &me->super, APP_WORKER_NAME,
                           APP_WORKER_STACK_BYTES, (uint8_t)KODE_PRIO_USER_LOW,
                           (uint8_t)APP_WORKER_DEPTH);
    if ( KODE_OK != rc )
    {
        KLOG_E(TAG, "the worker did not start (status %d)", (int)rc);
    }
    else
    {
        queue_job(me, true);
    }
    if ( !doom_frame_start() )
    {
        KLOG_E(TAG, "no frame task");
    }
    KLOG_I(TAG, "up");
    return Q_TRAN(&AppAo_running);
}

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
                KLOG_I(TAG, "job done: status %u value %lu",
                       (unsigned)done->status, (unsigned long)done->value);
            }
            me->busy = false;
            pump(me);
            status = Q_HANDLED();
            break;
        }
        case APP_PICK_SIG:
        {
            queue_job(me, false);
            status = Q_HANDLED();
            break;
        }
        case APP_RESCAN_SIG:
        {
            queue_job(me, true);
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
