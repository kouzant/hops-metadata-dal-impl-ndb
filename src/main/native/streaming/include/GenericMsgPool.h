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
 * File:   GenericMsgPool.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICMSGPOOL_H
#define GENERICMSGPOOL_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

/// Memory management framework for EventMsg objects in the
/// "asynchronous event handling" framework.
///
/// MsgType objects are allocated through pool objects, one per
/// listener (that is, there may be several). GenericMsgPool is the
/// generic interface for message allocation and reclamation.
/// Currently the interface is implemented by LocalMsgPool and
/// SharedMemMsgPool. The LocalMsgPool objects keep a stack of free
/// objects, and malloc() new ones when running out of free ones
/// (there is no truncation of free list, at the moment).
/// SharedMemMsgPool is suitable for asynchronous message processing
/// (i.e. when there are separate listener- and message processing
/// threads), which makes message objects returned to the "owner"
/// listener where they are reclaimed using the local message pool.
template<class MsgType>
class GenericMsgPool {
public:
  GenericMsgPool() {}
  ~GenericMsgPool() {}

  /// Allocate or re-use an MsgType object.
  MsgType* getMsgObject() { assert(0); }
  void freeMsgObject(MsgType *msgObj) { assert(0); } //!< make it free;
};

#endif // GENERICMSGPOOL_H
