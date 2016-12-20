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
 * File:   RemoteMemMsgPool.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef REMOTEMEMMSGPOOL_H
#define REMOTEMEMMSGPOOL_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"
#include "GenericMsgPool.h"

/// The complementary "remote part" for SharedMemMsgPool objects:
/// while SharedMemMsgPool objects are used to allocate message
/// objects (at the producer side), RemoteMemMsgPool objects are used
/// to reclaim the objects once they are no longer used (at the
/// consumer side). See SharedMemMsgPool for more details.
template<typename MsgType>
class RemoteMemMsgPool : public GenericMsgPool<MsgType> {
public:
  RemoteMemMsgPool(unsigned int nQueues);
  ~RemoteMemMsgPool();

  /// references to queue tails are needed in order to initialize
  /// those together with corresponding queue tails;
  GenericMsgP2PQueueTail<MsgType>* getQueueTail(unsigned int n);

  /// reclaim a MsgType object - an implementation of the
  /// GenericMsgPool<>::freeMsgObject() method.
  void freeMsgObject(MsgType *msgObj) const {
    assert(msgObj != (MsgType *) NULL);
    unsigned int senderIdx = msgObj->getSenderIdx();
    rqs[senderIdx].enqueue(msgObj);
  }

private:
  /// return queues, one per destination event message handling thread.
  GenericMsgP2PQueueTail<MsgType> * const rqs;
  unsigned int const nRqs;    //!< number of registered return queues;
};

#include "RemoteMemMsgPool.tcpp"

#endif // REMOTEMEMMSGPOOL_H
