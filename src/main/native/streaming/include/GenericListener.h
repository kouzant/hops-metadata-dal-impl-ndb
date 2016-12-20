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
 * File:   GenericListener.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICLISTENER_H
#define GENERICLISTENER_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"

/// GenericListener is the generic interfrace to listener thread'
/// objects: it defines blocking and non-blocking "do some listening &
/// polling" operations, such that multiple listeners can share the
/// same thread provided by a GenericListenerThread object.
class GenericListener {
public:
  GenericListener(const unsigned long pollTimeout)
    : timeout(pollTimeout) {}
  ~GenericListener() {}

public:
  unsigned int handleEventsNonBlocking() { assert(0); }
  unsigned int handleEventsTimeout() { assert(0); }
  unsigned long getTimeout() { return (timeout); }
    
private:
  const unsigned long timeout;
};

#endif // GENERICLISTENER_H

