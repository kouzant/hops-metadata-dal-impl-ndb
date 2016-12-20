/*
 * Copyright (C) 2016 Hops.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* 
 * File:   Platform.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include "common.h"
#include <stdint.h>

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

// TODO - these shoud come from the build environment (e.g. autoconfig
// checks)
#define ARCH	i686_arch

// CPU instructions..
#define CC_x86_XCHG
#define CC_x86_CMPXCHG
#define CC_x86_CMPXCHG8B
#define CC_x86_INC
#define CC_x86_DEC
#define CC_x86_XADD
#define CC_x86_MFENCE
#define CC_x86_SFENCE
#define CC_x86_LFENCE
#define CC_x86_RDTSC
// GCC intrinsics..
#define CC_MEM_ACC_BARRIER
#define CC_SYNC_BOOL_COMPARE_AND_SWAP
#define CC_SYNC_SYNCHRONIZE
#define CC_BUILTIN_EXPECT
#define CC_BUILTIN_PREFETCH
#define CC_BUILTIN_CTZ
#define CC_BUILTIN_CLZ


// platform-specific assembly snippets;

#if ARCH == i686_arch

//
#if defined(CC_x86_XCHG)
//
#define ASM_SWAP(cell, value)					\
{ 								\
  __asm__ __volatile__ ("xchgl %3,%0"				\
                        :"=r" (value), "=m" (cell)		\
                        :"0" (value), "m" (cell));		\
}

/*
 the same as above - with in/out ("+") values;
#define ASM_SWAP(cell, value)					\
{ 								\
  __asm__ __volatile__ ("xchgl %0,%1"				\
                        :"+r" (value), "+m" (cell) : );		\
}
*/

// 'xchg' can also take memory and register arguments in opposite
// order (hugh - what for is this other form?), and instruction
// argument specification - %1 - cab refer to 2nd output argument
// ("=m" cell), not necessarily to the 2nd input one ("m" (cell)),
// which are the same;
#define ASM_SWAP_ALT(cell, value)				\
{ 								\
  __asm__ __volatile__ ("xchgl %0,%1"				\
                        :"=r" (value), "=m" (cell)		\
                        :"0" (value), "m" (cell));		\
}
#endif

//
#if defined(CC_x86_CMPXCHG)

#define ASM_CAS(cell, old, new)					\
{ 								\
  __asm__ __volatile__ ("lock cmpxchg %4,%0"			\
                        : "=m" (cell), "=a" (old)		\
                        : "1" (old), "m" (cell), "r" (new)	\
                        : "cc");				\
}
#endif

//
#if defined(CC_x86_CMPXCHG8B)

#define ASM_CAS8B(cell, old, new)				\
{ 								\
 __asm__ __volatile__("lock cmpxchg8b %3"			\
		      : "=A"(old)				\
		      : "b"((unsigned long) new),		\
			"c"((unsigned long) (new >> 32)),	\
			"m"(cell),				\
			"0"(old)				\
		      : "memory","cc");				\
}
#endif

//
#if defined(CC_x86_INCW)

#define ASM_ATOMIC_INC(cell)					\
{ 								\
  __asm__ __volatile__ ("lock incw %0"				\
                        : "=m" (cell)				\
                        : "m" (cell)				\
                        : "cc");				\
}
#endif

//
#if defined(CC_x86_DECW)

#define ASM_ATOMIC_DEC(cell)					\
{ 								\
  __asm__ __volatile__ ("lock decw %0"				\
                        : "=m" (cell)				\
                        : "m" (cell)				\
                        : "cc");				\
}
#endif

//
#if defined(CC_x86_XADD)

#define ASM_EXCH_ADD(cell, value)				\
{ 								\
  __asm__ __volatile__ ("lock xadd %1,%0"			\
                        : "=m" (cell), "=r" (value)		\
                        : "m" (cell), "1" (value)		\
                        : "cc");				\
}
#endif

#if defined(CC_x86_SFENCE)
#define x86_SFENCE()	{__asm__ __volatile__ ("sfence" : : : "memory" );}
#endif

#if defined(CC_x86_MFENCE)
#define x86_MFENCE()	{__asm__ __volatile__ ("mfence" : : : "memory" );}
#endif

//
// also some low-level (fast) operations:
//   'test-and-test-and-set' lock (using CAS);
// subsumes a load barrier (by the CAS instruction);
inline void takeLock(unsigned int volatile &lockRef) {
  while (1) {
    while (lockRef == 1) ;
    unsigned int v = 0;
    unsigned int now = 1;
#if defined(CC_SYNC_BOOL_COMPARE_AND_SWAP)
    if (__sync_bool_compare_and_swap(&lockRef, v, now))
      break;
#elif ARCH == i686_arch && defined(ASM_CAS)
    ASM_CAS(lockRef, v, now);
    if (v == 0)
      break;
#else
    NOT IMPLEMENTED!;
#endif
  }
}

// an expression;
inline unsigned int tryLock(unsigned int volatile &lockRef) {
  unsigned int v = 0;
  unsigned int now = 1;
#if defined(CC_SYNC_BOOL_COMPARE_AND_SWAP)
  return (__sync_bool_compare_and_swap(&lockRef, v, now));
#elif ARCH == i686_arch && defined(ASM_CAS)
  ASM_CAS(lockRef, v, now);
  return (v == 0);
#else
  NOT IMPLEMENTED!;
#endif
}

// subsumes a store barrier;
inline void releaseLock(unsigned int volatile &lockRef) {
  x86_MFENCE();
  lockRef = 0;
}

#if defined(CC_x86_RDTSC)
inline uint64_t rdtsc_unsync()
{
  uint32_t lo, hi;
  __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
  return ((uint64_t) hi << 32 | lo);
}
#endif

#endif


// gcc intrinsics;
//
#if defined(CC_SYNC_SYNCHRONIZE)
#define CC_FULL_BARRIER()		__sync_synchronize()
#elif ARCH == i686_arch
#define CC_FULL_BARRIER()		x86_MFENCE()
#else
      NOT IMPLEMENTED!;
#endif

//
#if defined(CC_BUILTIN_EXPECT)
#define CC_EXPECT(Exp, Val)		__builtin_expect(Exp, Val)
#else
#define CC_EXPECT(Exp, Val)		(Exp)
#endif

//
#if defined(CC_BUILTIN_CONSTANT_P)
#define CC_CONSTANT(Exp)		__builtin_constant_p(Exp)
#else
#define CC_CONSTANT(Exp)		0
#endif

//
#if defined(CC_BUILTIN_PREFETCH)
#define CC_PREFETCH(Addr)		__builtin_prefetch(Addr)
#else
#define CC_PREFETCH(Addr)		;
#endif

//
#if defined(CC_BUILTIN_CLZ)
#define CC_CLZ(Exp)			__builtin_clz(Exp)
#else
      NOT IMPLEMENTED!;
#endif

#if defined(CC_BUILTIN_CTZ)
#define CC_CTZ(Exp)			__builtin_ctz(Exp)
#else
      NOT IMPLEMENTED!;
#endif


// memory barriers (on top of the above);

// 'Full_BARRIER()';
#if defined(CC_SYNC_SYNCHRONIZE)
#define Full_BARRIER()			CC_FULL_BARRIER()
#elif defined(x86_MFENCE)
#define Full_BARRIER()			x86_MFENCE()
#else
    NOT IMPLEMENTED!;
#endif

// memory access barrier - i.e. preventing the compiler from moving
// around memory accesses;
#if defined(CC_MEM_ACC_BARRIER)
#define MEM_ACC_BARRIER()		__asm__ __volatile__ ( "" : : : "memory" );
#else
#define MEM_ACC_BARRIER()		Full_BARRIER()
#endif

#if ARCH == i686_arch

//
#define LoadStore_BARRIER()
#define LoadLoad_BARRIER()
#define StoreStore_BARRIER()
#if defined(CC_SYNC_SYNCHRONIZE)
#define StoreLoad_BARRIER()		CC_FULL_BARRIER()
#elif defined(x86_MFENCE)
#define StoreLoad_BARRIER()		x86_MFENCE()
#else
    NOT IMPLEMENTED!;
#endif

// combined LoadLoad + StoreLoad :
#define Load_BARRIER()			StoreLoad_BARRIER()
// combined StoreLoad + StoreStore :
#define Store_BARRIER()			StoreLoad_BARRIER()

#else
    NOT IMPLEMENTED!;
#endif


// various platform-specific constants

#if ARCH == i686_arch
unsigned int const CACHE_LINE = 64;
#else
    NOT IMPLEMENTED!;
#endif

#  if __GLIBC__ >= 2
/// a HINT of the memory overhead for malloc() and friends - such that
/// memory fragmentation could be avoided with objects that should be
/// memory-aligned but can be "stretched" to exploit available space.
/// For example, EventMsg objects should be cache line aligned, and
/// include a small memory heap - then (N*CACHE_LINE -
/// MALLOC_OVERHEAD) is the object size that fully exploits N cache
/// lines.
unsigned int const MALLOC_OVERHEAD = 16;
#else
    NOT IMPLEMENTED!;
#endif


#endif /* PLATFORM_H */
