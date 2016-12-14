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
 * File:   conf.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef __CONF_H
#define __CONF_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

// various configuration parameters

unsigned int const CACHE_LINE = 64;
/// a HINT of the memory overhead for malloc() and friends - such that
/// memory fragmentation could be avoided with objects that should be
/// memory-aligned but can be "stretched" to exploit available space.
/// For example, EventMsg objects should be cache line aligned, and
/// include a small memory heap - then (N*CACHE_LINE -
/// MALLOC_OVERHEAD) is the object size that fully exploits N cache
/// lines.
unsigned int const MALLOC_OVERHEAD = 16;

#endif // __CONF_H
