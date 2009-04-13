/*
 *  ejsTimer.c -- Timer class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/****************************** Forward Declarations **************************/

static void timerCallback(EjsTimer *tp, MprEvent *e);

/*********************************** Methods **********************************/
/*
 *  Create a new timer
 *
 *  function Timer(period: Number, callback: Function, drift: Boolean = true)
 */
static EjsVar *constructor(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 2 || argc == 3);
    mprAssert(ejsIsNumber(argv[0]));
    mprAssert(ejsIsFunction(argv[1]));

    tp->ejs = ejs;
    tp->period = ejsGetInt(argv[0]);
    tp->callback = (EjsFunction*) argv[1];
    tp->drift = (argc == 3) ? ejsGetInt(argv[2]) : 1;

    tp->event = mprCreateTimerEvent(ejs, (MprEventProc) timerCallback, tp->period, MPR_NORMAL_PRIORITY, tp, MPR_EVENT_CONTINUOUS);
    if (tp->event == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    return 0;
}



/*
 *  Get the timer drift setting
 *
 *  function get drift(): Boolean
 */
static EjsVar *getDrift(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    return (EjsVar*) ejsCreateBoolean(ejs, tp->drift);
}


/*
 *  Set the timer drift setting
 *
 *  function set drift(period: Boolean): Void
 */
static EjsVar *setDrift(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsBoolean(argv[0]));

    tp->drift = ejsGetBoolean(argv[0]);
    return 0;
}


/*
 *  Get the timer period
 *
 *  function get period(): Number
 */
static EjsVar *getPeriod(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    return (EjsVar*) ejsCreateNumber(ejs, tp->period);
}


/*
 *  Set the timer period and restart the timer
 *
 *  function set period(period: Number): Void
 */
static EjsVar *setPeriod(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    tp->period = ejsGetInt(argv[0]);
    mprRescheduleEvent(tp->event, tp->period);
    return 0;
}


/*
 *  Restart a timer
 *
 *  function restart(); Void
 */
static EjsVar *restart(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    mprRestartContinuousEvent(tp->event);
    return 0;
}


/*
 *  Stop a timer
 *
 *  function stop(): Void
 */
static EjsVar *stop(Ejs *ejs, EjsTimer *tp, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    mprRemoveEvent(tp->event);
#if UNUSED
    mprStopContinuousEvent(tp->event);
#endif
    return 0;
}


/*********************************** Support **********************************/

EjsObject *ejsCreateTimerEvent(Ejs *ejs, EjsTimer *tp)
{
    EjsObject       *event;

    event = ejsCreateObject(ejs, ejsGetType(ejs, ES_ejs_events_TimerEvent), 0);
    if (event == 0) {
        return 0;
    }
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_data, (EjsVar*) tp);
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_timestamp, (EjsVar*) ejsCreateDate(ejs, 0));
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_bubbles, (EjsVar*) ejs->falseValue);
    ejsSetProperty(ejs, (EjsVar*) event, ES_ejs_events_Event_priority, (EjsVar*) ejsCreateNumber(ejs, MPR_NORMAL_PRIORITY));
    return event;
}



static void timerCallback(EjsTimer *tp, MprEvent *e)
{
    Ejs         *ejs;
    EjsObject   *event;
    EjsVar      *arg;

    mprAssert(tp);

    ejs = tp->ejs;

    event = ejsCreateTimerEvent(ejs, tp);
    if (event == 0) {
        return;
    }

    arg = (EjsVar*) event;
    ejsRunFunction(tp->ejs, tp->callback, 0, 1, &arg);
}


/*********************************** Factory **********************************/

void ejsCreateTimerType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.events", "Timer"), ejs->objectType, sizeof(EjsTimer), 
        ES_ejs_events_Timer, ES_ejs_events_Timer_NUM_CLASS_PROP, ES_ejs_events_Timer_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureTimerType(Ejs *ejs)
{
    EjsType     *type;

    type = ejsGetType(ejs, ES_ejs_events_Timer);

    ejsBindMethod(ejs, type, ES_ejs_events_Timer_Timer, (EjsNativeFunction) constructor);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_restart, (EjsNativeFunction) restart);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_stop, (EjsNativeFunction) stop);

    ejsBindMethod(ejs, type, ES_ejs_events_Timer_period, (EjsNativeFunction) getPeriod);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_set_period, (EjsNativeFunction) setPeriod);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_drift, (EjsNativeFunction) getDrift);
    ejsBindMethod(ejs, type, ES_ejs_events_Timer_set_drift, (EjsNativeFunction) setDrift);
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */
