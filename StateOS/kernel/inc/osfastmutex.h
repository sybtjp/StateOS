/******************************************************************************

    @file    StateOS: osfastmutex.h
    @author  Rajmund Szymanski
    @date    14.08.2018
    @brief   This file contains definitions for StateOS.

 ******************************************************************************

   Copyright (c) 2018 Rajmund Szymanski. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

 ******************************************************************************/

#ifndef __STATEOS_MUT_H
#define __STATEOS_MUT_H

#include "oskernel.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *
 * Name              : fast mutex (non-recursive and non-priority-inheritance)
 *
 * Note              : use only to synchronize tasks with the same priority
 *
 ******************************************************************************/

typedef struct __mut mut_t, * const mut_id;

struct __mut
{
	tsk_t  * queue; // next process in the DELAYED queue
	void   * res;   // allocated fast mutex object's resource
	tsk_t  * owner; // owner task
};

/******************************************************************************
 *
 * Name              : _MUT_INIT
 *
 * Description       : create and initialize a fast mutex object
 *
 * Parameters        : none
 *
 * Return            : fast mutex object
 *
 * Note              : for internal use
 *
 ******************************************************************************/

#define               _MUT_INIT() { 0, 0, 0 }

/******************************************************************************
 *
 * Name              : OS_MUT
 *
 * Description       : define and initialize a fast mutex object
 *
 * Parameters
 *   mut             : name of a pointer to fast mutex object
 *
 ******************************************************************************/

#define             OS_MUT( mut )                     \
                       mut_t mut##__mut = _MUT_INIT(); \
                       mut_id mut = & mut##__mut

/******************************************************************************
 *
 * Name              : static_MUT
 *
 * Description       : define and initialize a static fast mutex object
 *
 * Parameters
 *   mut             : name of a pointer to fast mutex object
 *
 ******************************************************************************/

#define         static_MUT( mut )                     \
                static mut_t mut##__mut = _MUT_INIT(); \
                static mut_id mut = & mut##__mut

/******************************************************************************
 *
 * Name              : MUT_INIT
 *
 * Description       : create and initialize a fast mutex object
 *
 * Parameters        : none
 *
 * Return            : fast mutex object
 *
 * Note              : use only in 'C' code
 *
 ******************************************************************************/

#ifndef __cplusplus
#define                MUT_INIT() \
                      _MUT_INIT()
#endif

/******************************************************************************
 *
 * Name              : MUT_CREATE
 * Alias             : MUT_NEW
 *
 * Description       : create and initialize a fast mutex object
 *
 * Parameters        : none
 *
 * Return            : pointer to fast mutex object
 *
 * Note              : use only in 'C' code
 *
 ******************************************************************************/

#ifndef __cplusplus
#define                MUT_CREATE() \
           (mut_t[]) { MUT_INIT  () }
#define                MUT_NEW \
                       MUT_CREATE
#endif

/******************************************************************************
 *
 * Name              : mut_init
 *
 * Description       : initialize a fast mutex object
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mut_init( mut_t *mut );

/******************************************************************************
 *
 * Name              : mut_create
 * Alias             : mut_new
 *
 * Description       : create and initialize a new fast mutex object
 *
 * Parameters        : none
 *
 * Return            : pointer to fast mutex object (fast mutex successfully created)
 *   0               : fast mutex not created (not enough free memory)
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

mut_t *mut_create( void );

__STATIC_INLINE
mut_t *mut_new( void ) { return mut_create(); }

/******************************************************************************
 *
 * Name              : mut_kill
 *
 * Description       : reset the fast mutex object and wake up all waiting tasks with 'E_STOPPED' event value
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mut_kill( mut_t *mut );

/******************************************************************************
 *
 * Name              : mut_delete
 *
 * Description       : reset the fast mutex object and free allocated resource
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mut_delete( mut_t *mut );

/******************************************************************************
 *
 * Name              : mut_waitFor
 *
 * Description       : try to lock the fast mutex object,
 *                     wait for given duration of time if the fast mutex object can't be locked immediately
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *   delay           : duration of time (maximum number of ticks to wait for lock the fast mutex object)
 *                     IMMEDIATE: don't wait if the fast mutex object can't be locked immediately
 *                     INFINITE:  wait indefinitely until the fast mutex object has been locked
 *
 * Return
 *   E_SUCCESS       : fast mutex object was successfully locked
 *   E_STOPPED       : fast mutex object was killed before the specified timeout expired
 *   E_TIMEOUT       : fast mutex object was not locked before the specified timeout expired
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

unsigned mut_waitFor( mut_t *mut, cnt_t delay );

/******************************************************************************
 *
 * Name              : mut_waitUntil
 *
 * Description       : try to lock the fast mutex object,
 *                     wait until given timepoint if the fast mutex object can't be locked immediately
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *   time            : timepoint value
 *
 * Return
 *   E_SUCCESS       : fast mutex object was successfully locked
 *   E_STOPPED       : fast mutex object was killed before the specified timeout expired
 *   E_TIMEOUT       : fast mutex object was not locked before the specified timeout expired
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

unsigned mut_waitUntil( mut_t *mut, cnt_t time );

/******************************************************************************
 *
 * Name              : mut_wait
 *
 * Description       : try to lock the fast mutex object,
 *                     wait indefinitely if the fast mutex object can't be locked immediately
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *
 * Return
 *   E_SUCCESS       : fast mutex object was successfully locked
 *   E_STOPPED       : fast mutex object was killed
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

__STATIC_INLINE
unsigned mut_wait( mut_t *mut ) { return mut_waitFor(mut, INFINITE); }

/******************************************************************************
 *
 * Name              : mut_take
 *
 * Description       : try to lock the fast mutex object,
 *                     don't wait if the fast mutex object can't be locked immediately
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *
 * Return
 *   E_SUCCESS       : fast mutex object was successfully locked
 *   E_TIMEOUT       : fast mutex object can't be locked immediately
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

__STATIC_INLINE
unsigned mut_take( mut_t *mut ) { return mut_waitFor(mut, IMMEDIATE); }

/******************************************************************************
 *
 * Name              : mut_give
 *
 * Description       : try to unlock the fast mutex object (only owner task can unlock fast mutex object),
 *                     don't wait if the fast mutex object can't be unlocked
 *
 * Parameters
 *   mut             : pointer to fast mutex object
 *
 * Return
 *   E_SUCCESS       : fast mutex object was successfully unlocked
 *   E_TIMEOUT       : fast mutex object can't be unlocked
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

unsigned mut_give( mut_t *mut );

#ifdef __cplusplus
}
#endif

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus

/******************************************************************************
 *
 * Class             : FastMutex
 *
 * Description       : create and initialize a fast mutex object
 *
 * Constructor parameters
 *                   : none
 *
 ******************************************************************************/

struct FastMutex : public __mut
{
	 FastMutex( void ): __mut _MUT_INIT() {}
	~FastMutex( void ) { assert(__mut::owner == nullptr); }

	void     kill     ( void )         {        mut_kill     (this);         }
	unsigned waitFor  ( cnt_t _delay ) { return mut_waitFor  (this, _delay); }
	unsigned waitUntil( cnt_t _time )  { return mut_waitUntil(this, _time);  }
	unsigned wait     ( void )         { return mut_wait     (this);         }
	unsigned take     ( void )         { return mut_take     (this);         }
	unsigned give     ( void )         { return mut_give     (this);         }
};

#endif//__cplusplus

/* -------------------------------------------------------------------------- */

#endif//__STATEOS_MUT_H
