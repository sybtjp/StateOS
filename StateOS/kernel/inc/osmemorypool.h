/******************************************************************************

    @file    StateOS: osmemorypool.h
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

#ifndef __STATEOS_MEM_H
#define __STATEOS_MEM_H

#include "oskernel.h"
#include "oslist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */

#define MSIZE( size ) \
 ALIGNED_SIZE( size, que_t )

/******************************************************************************
 *
 * Name              : memory pool
 *
 ******************************************************************************/

typedef struct __mem mem_t, * const mem_id;

struct __mem
{
	tsk_t  * queue; // inherited from list
	void   * res;   // allocated memory pool object's resource
	que_t    head;  // inherited from list

	unsigned limit; // size of a memory pool (max number of objects)
	unsigned size;  // size of memory object (in words)
	void   * data;  // pointer to memory pool buffer
};

/******************************************************************************
 *
 * Name              : _MEM_INIT
 *
 * Description       : create and initialize a memory pool object
 *
 * Parameters
 *   size            : size of memory object (in bytes)
 *   data            : memory pool data buffer
 *
 * Return            : memory pool object
 *
 * Note              : for internal use
 *
 ******************************************************************************/

#define               _MEM_INIT( _limit, _size, _data ) { 0, 0, _QUE_INIT(), _limit, MSIZE(_size), _data }

/******************************************************************************
 *
 * Name              : _MEM_DATA
 *
 * Description       : create a memory pool data buffer
 *
 * Parameters
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 * Return            : memory pool data buffer
 *
 * Note              : for internal use
 *
 ******************************************************************************/

#ifndef __cplusplus
#define               _MEM_DATA( _limit, _size ) (void *[_limit * (1 + MSIZE(_size))]){ 0 }
#endif

/******************************************************************************
 *
 * Name              : OS_MEM
 *
 * Description       : define and initialize a memory pool object
 *
 * Parameters
 *   mem             : name of a pointer to memory pool object
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 ******************************************************************************/

#define             OS_MEM( mem, limit, size )                                \
                       void*mem##__buf[limit*(1+MSIZE(size))];                 \
                       mem_t mem##__mem = _MEM_INIT( limit, size, mem##__buf ); \
                       mem_id mem = & mem##__mem

/******************************************************************************
 *
 * Name              : static_MEM
 *
 * Description       : define and initialize a static memory pool object
 *
 * Parameters
 *   mem             : name of a pointer to memory pool object
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 ******************************************************************************/

#define         static_MEM( mem, limit, size )                                \
                static void*mem##__buf[limit*(1+MSIZE(size))];                 \
                static mem_t mem##__mem = _MEM_INIT( limit, size, mem##__buf ); \
                static mem_id mem = & mem##__mem

/******************************************************************************
 *
 * Name              : MEM_INIT
 *
 * Description       : create and initialize a memory pool object
 *
 * Parameters
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 * Return            : memory pool object
 *
 * Note              : use only in 'C' code
 *
 ******************************************************************************/

#ifndef __cplusplus
#define                MEM_INIT( limit, size ) \
                      _MEM_INIT( limit, size, _MEM_DATA( limit, size ) )
#endif

/******************************************************************************
 *
 * Name              : MEM_CREATE
 * Alias             : MEM_NEW
 *
 * Description       : create and initialize a memory pool object
 *
 * Parameters
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 * Return            : pointer to memory pool object
 *
 * Note              : use only in 'C' code
 *
 ******************************************************************************/

#ifndef __cplusplus
#define                MEM_CREATE( limit, size ) \
           (mem_t[]) { MEM_INIT  ( limit, size ) }
#define                MEM_NEW \
                       MEM_CREATE
#endif

/******************************************************************************
 *
 * Name              : mem_bind
 *
 * Description       : initialize data buffer of a memory pool object
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mem_bind( mem_t *mem );

/******************************************************************************
 *
 * Name              : mem_init
 *
 * Description       : initialize a memory pool object
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *   data            : memory pool data buffer
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mem_init( mem_t *mem, unsigned limit, unsigned size, void *data );

/******************************************************************************
 *
 * Name              : mem_create
 * Alias             : mem_new
 *
 * Description       : create and initialize a new memory pool object
 *
 * Parameters
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 * Return            : pointer to memory pool object (memory pool successfully created)
 *   0               : memory pool not created (not enough free memory)
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

mem_t *mem_create( unsigned limit, unsigned size );

__STATIC_INLINE
mem_t *mem_new( unsigned limit, unsigned size ) { return mem_create(limit, size); }

/******************************************************************************
 *
 * Name              : mem_kill
 *
 * Description       : wake up all waiting tasks with 'E_STOPPED' event value
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mem_kill( mem_t *mem );

/******************************************************************************
 *
 * Name              : mem_delete
 *
 * Description       : reset the memory pool object and free allocated resource
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *
 * Return            : none
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

void mem_delete( mem_t *mem );

/******************************************************************************
 *
 * Name              : mem_waitFor
 *
 * Description       : try to get memory object from the memory pool object,
 *                     wait for given duration of time while the memory pool object is empty
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *   data            : pointer to store the pointer to the memory object
 *   delay           : duration of time (maximum number of ticks to wait while the memory pool object is empty)
 *                     IMMEDIATE: don't wait if the memory pool object is empty
 *                     INFINITE:  wait indefinitely while the memory pool object is empty
 *
 * Return
 *   E_SUCCESS       : pointer to memory object was successfully transfered to the data pointer
 *   E_STOPPED       : memory pool object was killed before the specified timeout expired
 *   E_TIMEOUT       : memory pool object is empty and was not received data before the specified timeout expired
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

__STATIC_INLINE
unsigned mem_waitFor( mem_t *mem, void **data, cnt_t delay ) { return lst_waitFor((lst_t *)mem, data, delay); }

/******************************************************************************
 *
 * Name              : mem_waitUntil
 *
 * Description       : try to get memory object from the memory pool object,
 *                     wait until given timepoint while the memory pool object is empty
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *   data            : pointer to store the pointer to the memory object
 *   time            : timepoint value
 *
 * Return
 *   E_SUCCESS       : pointer to memory object was successfully transfered to the data pointer
 *   E_STOPPED       : memory pool object was killed before the specified timeout expired
 *   E_TIMEOUT       : memory pool object is empty and was not received data before the specified timeout expired
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

__STATIC_INLINE
unsigned mem_waitUntil( mem_t *mem, void **data, cnt_t time ) { return lst_waitUntil((lst_t *)mem, data, time); }

/******************************************************************************
 *
 * Name              : mem_wait
 *
 * Description       : try to get memory object from the memory pool object,
 *                     wait indefinitely while the memory pool object is empty
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *   data            : pointer to store the pointer to the memory object
 *
 * Return
 *   E_SUCCESS       : pointer to memory object was successfully transfered to the data pointer
 *   E_STOPPED       : memory pool object was killed
 *
 * Note              : use only in thread mode
 *
 ******************************************************************************/

__STATIC_INLINE
unsigned mem_wait( mem_t *mem, void **data ) { return lst_wait((lst_t *)mem, data); }

/******************************************************************************
 *
 * Name              : mem_take
 * ISR alias         : mem_takeISR
 *
 * Description       : try to get memory object from the memory pool object,
 *                     don't wait if the memory pool object is empty
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *   data            : pointer to store the pointer to the memory object
 *
 * Return
 *   E_SUCCESS       : pointer to memory object was successfully transfered to the data pointer
 *   E_TIMEOUT       : memory pool object is empty
 *
 * Note              : may be used both in thread and handler mode
 *
 ******************************************************************************/

__STATIC_INLINE
unsigned mem_take( mem_t *mem, void **data ) { return lst_take((lst_t*)mem, data); }

__STATIC_INLINE
unsigned mem_takeISR( mem_t *mem, void **data ) { return lst_takeISR((lst_t *)mem, data); }

/******************************************************************************
 *
 * Name              : mem_give
 * ISR alias         : mem_giveISR
 *
 * Description       : transfer memory object to the memory pool object,
 *
 * Parameters
 *   mem             : pointer to memory pool object
 *   data            : pointer to memory object
 *
 * Return            : none
 *
 * Note              : may be used both in thread and handler mode
 *
 ******************************************************************************/

__STATIC_INLINE
void mem_give( mem_t *mem, const void *data ) { lst_give((lst_t *)mem, data); }

__STATIC_INLINE
void mem_giveISR( mem_t *mem, const void *data ) { lst_giveISR((lst_t *)mem, data); }

#ifdef __cplusplus
}
#endif

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus

/******************************************************************************
 *
 * Class             : MemoryPoolT<>
 *
 * Description       : create and initialize a memory pool object
 *
 * Constructor parameters
 *   limit           : size of a buffer (max number of objects)
 *   size            : size of memory object (in bytes)
 *
 ******************************************************************************/

template<unsigned limit_, unsigned size_>
struct MemoryPoolT : public __mem
{
	 MemoryPoolT( void ): __mem _MEM_INIT(limit_, size_, data_) { mem_bind(this); }
	~MemoryPoolT( void ) { assert(__mem::queue == nullptr); }

	void     kill     ( void )                             {        mem_kill     (this);                }
	unsigned waitFor  (       void **_data, cnt_t _delay ) { return mem_waitFor  (this, _data, _delay); }
	unsigned waitUntil(       void **_data, cnt_t _time )  { return mem_waitUntil(this, _data, _time);  }
	unsigned wait     (       void **_data )               { return mem_wait     (this, _data);         }
	unsigned take     (       void **_data )               { return mem_take     (this, _data);         }
	unsigned takeISR  (       void **_data )               { return mem_takeISR  (this, _data);         }
	void     give     ( const void  *_data )               {        mem_give     (this, _data);         }
	void     giveISR  ( const void  *_data )               {        mem_giveISR  (this, _data);         }

	private:
	void *data_[limit_ * (1 + MSIZE(size_))];
};

/******************************************************************************
 *
 * Class             : MemoryPoolTT<>
 *
 * Description       : create and initialize a memory pool object
 *
 * Constructor parameters
 *   limit           : size of a buffer (max number of objects)
 *   T               : class of a memory object
 *
 ******************************************************************************/

template<unsigned limit_, class T>
struct MemoryPoolTT : public MemoryPoolT<limit_, sizeof(T)>
{
	MemoryPoolTT( void ): MemoryPoolT<limit_, sizeof(T)>() {}

	unsigned waitFor  ( T **_data, cnt_t _delay ) { return mem_waitFor  (this, reinterpret_cast<void **>(_data), _delay); }
	unsigned waitUntil( T **_data, cnt_t _time )  { return mem_waitUntil(this, reinterpret_cast<void **>(_data), _time);  }
	unsigned wait     ( T **_data )               { return mem_wait     (this, reinterpret_cast<void **>(_data));         }
	unsigned take     ( T **_data )               { return mem_take     (this, reinterpret_cast<void **>(_data));         }
	unsigned takeISR  ( T **_data )               { return mem_takeISR  (this, reinterpret_cast<void **>(_data));         }
};

#endif//__cplusplus

/* -------------------------------------------------------------------------- */

#endif//__STATEOS_MEM_H
