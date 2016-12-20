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
 * File:   SharedMemMsgPool.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef SHAREDMEMMSGPOOL_H
#define SHAREDMEMMSGPOOL_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"
#include "GenericMsgPool.h"

/// Memory management framework for GenericMsg objects under
/// "asynchronous message processing", i.e. when there are separate
/// listener- and message processing threads. There are two classes -
/// SharedMemMsgPool and RemoteMemMsgPool - that together implement
/// the GenericMsgPool interface. The SharedMemMsgPool objects are
/// used to allocate message objects (at the producer side), and
/// RemoteMemMsgPool objects are used to reclaim the objects once they
/// are no longer used (at the consumer side).
/// 
/// Each SharedMemMsgPool and its corresponding RemoteMemMsgPool
/// objects share a "return queue" (GenericMsgP2PQueue): when a
/// message has been processed by a message handler (the consumer), it
/// is returned to the "owner" listener thread (the producer) where
/// they are reused. The scheme does not require any (costly)
/// synchronization between listener and message handling threads.
///
/// Since the framework allows multiple listener threads and multiple
/// threads processing the corresponding message objects, and each
/// return queue is point-to-point (connecting one single message
/// handler and one single listener), there is a matrix of return
/// queues connecting each pair of receiving- and processing-
/// threads. The matrix is constructed by the overall ndb event
/// processing subsystem controller - currently, the
/// AsyncEvHandlingSys object (see AsyncEvHandlingSys.h for details).
///
/// The implementation uses the return queues themselves as storage of
/// the free message objects. Using queues provides a FIFO service
/// discipline, as opposed to the LIFO one as implemented by
/// LocalMsgPool. It might look the FIFO discipline is bound to
/// deliver worse data locality, BUT: (a) the message objects are read
/// by another core/processor, so they might be out of cache already
/// upon arrival through return queues; (b) the dequeue() method of
/// the GenericMsgP2PQueueHead implementation does prefetching of the
/// returned object, so by the time we try to dequeue a next element,
/// that should not cause a cache miss, (c) message objects from the
/// pool are used for writing (which is easier for the memory
/// subsystem), and, probably most importantly, (d) eager iterative
/// draining all return queues into a local stack can cause a lot of
/// cache misses (that cannot be alleviated).
///
/// However, the current implementation is still rather
/// "simple-minded".  I can think of the following improvements:
/// - introduce an "outgoing direction hint" argument to
///   getMsgObject() method, which would tell the message pool on
///   which return queue the new message will return. This way,
///   individual messages would be constantly sent to the same
///   listener - and not to a random one.
/// - dynamically monitor and limit the size of the return queues,
///   reducing latency of message objects reuse, and, thus, improving
///   cache hit rate and reducing cache pressure. "Excess" objects can
///   be "shed off" into a local stack and/or even returned to the
///   global memory manager.
/// - parametrize the class by the nQueues parameters, as opposed to a
///   runtime consturctor argument
template<typename MsgType>
class SharedMemMsgPool : public GenericMsgPool<MsgType> {
public:
  SharedMemMsgPool(unsigned int nQueues);
  ~SharedMemMsgPool();

  /// references to queue heads are needed in order to initialize
  /// those together with corresponding queue tails;
  GenericMsgP2PQueueHead<MsgType>* getQueueHead(unsigned int n) const;

  /// Allocate or re-use a MsgType object - an implementation of the
  /// GenericMsgPool<>::getMsgObject() method.
  MsgType* getMsgObject();

private:
  /// return queues, one per destination event message handling thread.
  GenericMsgP2PQueueHead<MsgType> * const rqs;
  unsigned int const nRqs;      //!< number of registered return queues;
  unsigned int lastCheckedRq;	//!< round-robin return queue scavenging;
  /// last dequeued message objects, one per queue (which cannot be
  /// used until next dequeue, due to GenericMsgP2PQueue limitation);
  MsgType ** const lastMsg;
};

#include "SharedMemMsgPool.tcpp"

#endif // SHAREDMEMMSGPOOL_H
