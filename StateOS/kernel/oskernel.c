/******************************************************************************

    @file    StateOS: oskernel.c
    @author  Rajmund Szymanski
    @date    06.05.2016
    @brief   This file provides set of variables and functions for StateOS.

 ******************************************************************************

    StateOS - Copyright (C) 2013 Rajmund Szymanski.

    This file is part of StateOS distribution.

    StateOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation; either version 3 of the License,
    or (at your option) any later version.

    StateOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.

 ******************************************************************************/

#include <os.h>

/* -------------------------------------------------------------------------- */
// SYSTEM KERNEL SERVICES
/* -------------------------------------------------------------------------- */

__attribute__ ((weak))
void idle_hook( void )
{
#if OS_ROBIN || OS_TIMER == 0
	__WFI();
#endif
}

/* -------------------------------------------------------------------------- */

#ifndef MAIN_SP
static  char     MAIN_STACK[ASIZE(OS_STACK_SIZE)] __osalign;
#define MAIN_SP (MAIN_STACK+ASIZE(OS_STACK_SIZE))
#endif
static  char     IDLE_STACK[ASIZE(OS_STACK_SIZE)] __osalign;
#define IDLE_SP (IDLE_STACK+ASIZE(OS_STACK_SIZE))

static
tsk_t MAIN = { { .id=ID_READY, .prev=&IDLE, .next=&IDLE }, .top=MAIN_SP, .basic=OS_MAIN_PRIO, .prio=OS_MAIN_PRIO }; // main task
tsk_t IDLE = { { .id=ID_IDLE,  .prev=&MAIN, .next=&MAIN }, .top=IDLE_SP, .state=idle_hook }; // idle task and tasks queue
sys_t System = { .cur=&MAIN };

/* -------------------------------------------------------------------------- */

static inline
void priv_rdy_insert( obj_id obj, unsigned id, obj_id nxt )
{
	obj_id prv = nxt->prev;

	obj->id   = id;
	obj->prev = prv;
	obj->next = nxt;
	nxt->prev = obj;
	prv->next = obj;
}

/* -------------------------------------------------------------------------- */

static inline
void priv_rdy_remove( obj_id obj )
{
	obj_id nxt = obj->next;
	obj_id prv = obj->prev;

	nxt->prev = prv;
	prv->next = nxt;
	obj->id   = ID_STOPPED;
}

/* -------------------------------------------------------------------------- */

static inline
void priv_tsk_insert( tsk_id tsk )
{
	tsk_id nxt = &IDLE;

	if    (tsk->prio)
	do     nxt = nxt->obj.next;
	while (tsk->prio <= nxt->prio);

	priv_rdy_insert(&tsk->obj, ID_READY, &nxt->obj);
}

/* -------------------------------------------------------------------------- */

void core_tsk_break( void )
{
	tsk_id cur = System.cur;

	for (;;)
	{
		port_ctx_switch();
		port_clr_lock();
		port_set_stack(cur->top);
		cur->state();
	}
}

/* -------------------------------------------------------------------------- */

void core_tsk_insert( tsk_id tsk )
{
	priv_tsk_insert(tsk);
#if OS_ROBIN
	tsk_id nxt = IDLE.obj.next;
	if (nxt->prio > System.cur->prio) // necessary because of the tsk_prio function
	port_ctx_switch();
#endif
}

/* -------------------------------------------------------------------------- */

void core_tsk_remove( tsk_id tsk )
{
	priv_rdy_remove(&tsk->obj);
}

/* -------------------------------------------------------------------------- */

void core_tsk_append( tsk_id tsk, os_id obj )
{
	tsk_id prv;
	tsk_id nxt = obj;
	tsk->guard = obj;

	do prv = nxt, nxt = nxt->obj.queue;
	while (nxt && tsk->prio <= nxt->prio);

	if (nxt)
	nxt->back  = tsk;
	tsk->back  = prv;
	tsk->obj.queue = nxt;
	prv->obj.queue = tsk;
}

/* -------------------------------------------------------------------------- */

void core_tsk_unlink( tsk_id tsk, unsigned event )
{
	tsk_id prv = tsk->back;
	tsk_id nxt = tsk->obj.queue;
	tsk->event = event;

	if (nxt)
	nxt->back = prv;
	prv->obj.queue = nxt;
	tsk->obj.queue = 0; // necessary because of tsk_sleep[Until|For] functions
}

/* -------------------------------------------------------------------------- */

static inline
unsigned priv_tsk_wait( tsk_id tsk, obj_id obj )
{
	core_tsk_append((tsk_id)tsk, obj);
	core_tsk_remove((tsk_id)tsk);
	core_tmr_insert((tmr_id)tsk, ID_DELAYED);

	core_ctx_switch();

	return tsk->event;
}

/* -------------------------------------------------------------------------- */

unsigned core_tsk_waitUntil( os_id obj, unsigned time )
{
	tsk_id cur = System.cur;

	cur->start = Counter;
	cur->delay = time - cur->start;

	if (cur->delay == IMMEDIATE)
	return E_TIMEOUT;

	return priv_tsk_wait(cur, obj);
}

/* -------------------------------------------------------------------------- */

unsigned core_tsk_waitFor( os_id obj, unsigned delay )
{
	tsk_id cur = System.cur;

	cur->start = Counter;
	cur->delay = delay;

	if (cur->delay == IMMEDIATE)
	return E_TIMEOUT;

	return priv_tsk_wait(cur, obj);
}

/* -------------------------------------------------------------------------- */

tsk_id core_tsk_wakeup( tsk_id tsk, unsigned event )
{
	if (tsk)
	{
		core_tsk_unlink((tsk_id)tsk, event);
		core_tmr_remove((tmr_id)tsk);
		core_tsk_insert((tsk_id)tsk);
	}

	return tsk;
}

/* -------------------------------------------------------------------------- */

tsk_id core_one_wakeup( os_id obj, unsigned event )
{
	obj_id lst = obj;

	return core_tsk_wakeup(lst->queue, event);
}

/* -------------------------------------------------------------------------- */

void core_all_wakeup( os_id obj, unsigned event )
{
	obj_id lst = obj;

	while (core_tsk_wakeup(lst->queue, event));
}

/* -------------------------------------------------------------------------- */

void core_tsk_prio( tsk_id tsk, unsigned prio )
{
	for (mtx_id mtx = tsk->list; mtx; mtx = mtx->list)
	{
		if (mtx->queue)
		if (prio < mtx->queue->prio)
			prio = mtx->queue->prio;
	}

	if (tsk->prio != prio)
	{
		tsk->prio = prio;

		if (tsk->obj.id == ID_READY)
		{
			core_tsk_remove(tsk);
			core_tsk_insert(tsk);
		}
		else
		if (tsk->obj.id == ID_DELAYED)
		{
			core_tsk_unlink(tsk, 0);
			core_tsk_append(tsk, tsk->guard);
		}
	}
}

/* -------------------------------------------------------------------------- */

static inline
os_id priv_tsk_prepare( tsk_id cur )
{
	if (cur->sp) return cur->sp;
	
	// prepare task stack if necessary

	ctx_id ctx = (ctx_id)cur->top - 1;

	ctx->psr = 0x01000000U;
	ctx->pc  = cur->state;
	ctx->lr  = core_tsk_break;

	sft_id sft = (sft_id)ctx - 1;

	sft->lr  = ~2U; // EXC_RETURN: return from psp

	return sft;
}

/* -------------------------------------------------------------------------- */

os_id core_tsk_handler( os_id sp )
{
	tsk_id cur;
#if OS_ROBIN == 0
	core_tmr_handler();
#endif
	port_isr_lock();
	core_ctx_reset();

	cur = System.cur;
	cur->sp = sp;

	if (cur->obj.id == ID_READY)
	{
		core_tsk_remove(cur);
		priv_tsk_insert(cur);
	}

	cur = System.cur = IDLE.obj.next;
	sp  = priv_tsk_prepare(cur); // prepare task stack if necessary

	port_isr_unlock();

	return sp;
}

/* -------------------------------------------------------------------------- */
// SYSTEM TIMER SERVICES
/* -------------------------------------------------------------------------- */

tmr_t WAIT = { { .id=ID_TIMER, .prev=&WAIT, .next=&WAIT }, .delay=INFINITE }; // timers queue

/* -------------------------------------------------------------------------- */
static inline
void priv_tmr_insert( tmr_id tmr, unsigned id )
{
	tmr_id nxt = &WAIT;

	if    (tmr->delay != INFINITE)
	do     nxt = nxt->obj.next;
	while (nxt->delay != INFINITE &&
	       nxt->delay <= tmr->start + tmr->delay - nxt->start);

	priv_rdy_insert(&tmr->obj, id, &nxt->obj);
}

/* -------------------------------------------------------------------------- */

void core_tmr_insert( tmr_id tmr, unsigned id )
{
	priv_tmr_insert(tmr, id);
	port_tmr_force();
}

/* -------------------------------------------------------------------------- */

void core_tmr_remove( tmr_id tmr )
{
	priv_rdy_remove(&tmr->obj);
}

/* -------------------------------------------------------------------------- */

#if OS_ROBIN && OS_TIMER

static inline
bool priv_tmr_expired( tmr_id tmr )
{
	port_tmr_stop();

	if (tmr->delay == INFINITE)
	return false; // return if timer counting indefinitly

	if (tmr->delay <= Counter - tmr->start)
	return true;  // return if timer finished counting

	port_tmr_start(tmr->start + tmr->delay);

	if (tmr->delay >  Counter - tmr->start)
	return false; // return if timer still counts

	port_tmr_stop();

	return true;  // however timer finished counting
}

/* -------------------------------------------------------------------------- */

#else

static inline
bool priv_tmr_expired( tmr_id tmr )
{
	if (tmr->delay >= Counter - tmr->start + 1)
	return false; // return if timer still counts or counting indefinitly

	return true;  // timer finished counting
}

#endif

/* -------------------------------------------------------------------------- */

static inline
void priv_tmr_wakeup( tmr_id tmr, unsigned event )
{
	tmr->start += tmr->delay;
	tmr->delay  = tmr->period;

	if (tmr->state)
		tmr->state();

	core_tmr_remove(tmr);
	if (tmr->delay)
	priv_tmr_insert(tmr, ID_TIMER);

	core_all_wakeup(tmr, event);
}

/* -------------------------------------------------------------------------- */

void core_tmr_handler( void )
{
	tmr_id tmr;

	port_isr_lock();

	while (priv_tmr_expired(tmr = WAIT.obj.next))
	{
		if (tmr->obj.id == ID_TIMER)
			priv_tmr_wakeup((tmr_id)tmr, E_SUCCESS);

		else      /* id == ID_DELAYED */
			core_tsk_wakeup((tsk_id)tmr, E_TIMEOUT);
	}

	port_isr_unlock();
}

/* -------------------------------------------------------------------------- */
// SYSTEM ALLOC SERVICES
/* -------------------------------------------------------------------------- */

#if OS_HEAP_SIZE

os_id core_sys_alloc( size_t size )
{
	static  char     Heap[ASIZE(OS_HEAP_SIZE)] __osalign;
	#define HeapEnd (Heap+ASIZE(OS_HEAP_SIZE))

	static
	char *heap = Heap;
	char *base = 0;

	if (size)
	{
		size = ASIZE(size);

		if (heap + size <= HeapEnd)
		{
			base = heap;
			heap = base + size;

			uint64_t *_mem = (uint64_t *)(base);
			uint64_t *_end = (uint64_t *)(heap);
			do *_mem++ = 0; while (_mem < _end);
		}
	}

	return base;
}

/* -------------------------------------------------------------------------- */

#else

os_id core_sys_alloc( size_t size )
{
	char *base = 0;

	if (size)
	{
		size = ASIZE(size);

		base = malloc(size);

		if (base)
		{
			uint64_t *_mem = (uint64_t *)(base);
			uint64_t *_end = (uint64_t *)(base + size);
			do *_mem++ = 0; while (_mem < _end);
		}
	}

	return base;
}

#endif

/* -------------------------------------------------------------------------- */
