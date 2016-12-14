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
 * File:   GenericAsyncMsgHandler.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICASYNCMSGHANDLER_H
#define GENERICASYNCMSGHANDLER_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "GenericMsgHandler.h"
#include "GenericAsyncMsg.h"
#include "GenericMsgP2PQueue.h"

/// GenericAsyncMsgHandler provides an interface to asynchronous
/// handling of [ndb update] event messages: calling
/// EventAsyncMsgHandler::handleEventMsg(EventMsg *eventMsg) causes
/// the argument EventMsg object be handled asynchronously (by a pool
/// of threads), while the method itself returns immediately.
///
/// Note that this is an implementation of the GenericMsgHandler
/// interface.
template<class MsgType, template<class TemplateMsgType> class MsgPool>
class GenericAsyncMsgHandler : public GenericMsgHandler<MsgType, MsgPool> {
public:
  GenericAsyncMsgHandler(unsigned int nQueues);
  ~GenericAsyncMsgHandler();

  /// references to queue heads are needed in order to initialize
  /// those together with corresponding queue tails;
  GenericMsgP2PQueueHead<MsgType>* const getQueueTail(unsigned int n) const;

  /// [instead of queuing] send the message using the queue determined
  /// from the message's hash. Obviously, GenericAsyncMsgHandler
  /// objects of all sibling listeners must be initialized with the
  /// same number of queues;
  void handleMsg(MsgType *msg) const {
    unsigned long const key = msg->hasKey();
    unsigned int const qn = key % nQs;
    mqs[qn].enqueue(msg);
  }

  // note that reclaimMsgObj() is not implemented (and no MspPool
  // objects are used at all);

private:
  /// message queues (of some specific type), selected based on message hash;
  GenericMsgP2PQueueTail<MsgType>* const mqs;
  unsigned int const nQs;	    //!< number of queues
};

#endif // GENERICASYNCMSGHANDLER_H
