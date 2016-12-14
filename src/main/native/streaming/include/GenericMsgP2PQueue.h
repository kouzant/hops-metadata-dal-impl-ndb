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
 * File:   GenericMsgP2PQueue.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICMSGP2PQUEUE_H
#define GENERICMSGP2PQUEUE_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

template<typename MsgType> class GenericMsgP2PQueueHead;
template<typename MsgType> class GenericMsgP2PQueueTail;

/// A thread-safe "point-to-point" (that is, single producer/single
/// consumer) queue. Restricting the queue to single producer and
/// single consumer allows the implementation to avoid using any
/// atomic synchronization instructions - but it still requires
/// write-to-write ordering, specifically that the GenericMsg::next
/// field pointing to another GenericMsg object being added to the queue
/// is update after the new object is completed in memory. Equally,
/// the reading thread's access to that GenericMsg::next field should be
/// a read-acquire operation, i.e. precede all operations on the
/// object pointed by that Event::next pointer.
///
/// The queue is used, in particular, for delivering EventMsg objects
/// from threads receiving ndb update events to threads that aggregate
/// information across multple ndb tables and mediate that aggregated
/// information to upper layers using JNI upcalls. The queue is also
/// used for returning the "spent" objects in the opposite direction
/// (so that EventMsg objects can be "free-listed" by the single
/// thread, without any inter-thread synchronization overhead).
///
/// The producer and consumer threads operate (mostly) using the pair
/// of objects - a "tail" and "head" queue objects, respectively. The
/// tail object allows to enqueue message objects, and the head one
/// allows to dequeue message objects. The tail and head objects are
/// accessed only by the respective threads. The tail and the head of
/// a queue communicate over the GenericMsg::next field of the last
/// GenericMsg object in the queue. Specifically, the tail overwrites
/// the object's NIL pointer with the pointer to a next GenericMsg
/// object, while the head attempts to dequeue an GenericMsg object
/// pointed by the last dequeued object's GenericMsg::next field.
/// When the queue is empty, the tail and the head objects point to
/// the same (already dequeued, and in the case of a new queue -
/// specially allocated) GenericMsg object.
///
/// Since the last dequeued message object is used internally by the
/// queue implementation, GenericMsgP2PQueue relies on the following
/// special discipline controlling reclamation of GenericMsg objects.
/// There are exactly two events that must happen before a message
/// object can be reclaimed: (1) the queue implementation no longer
/// needs the object, and (2) the message handler no longer needs the
/// object. Note that the event (1) occurs upon next successful
/// dequeue() operation (for which there must be a preceding enqueue()
/// operation), and the event (2) may happen quite some after the
/// initial handleMsg() invocation because the latter method is
/// allowed to save the message object for later processing, e.g. when
/// data from multiple messages must be combined. Thus, since the
/// relative order of these two events is generally unknown, the
/// GenericMsg::safeToReclaim() method must be called when either of
/// these events occurs. Upon second call, GenericMsg::safeToReclaim()
/// returns true meaning the object can be reclaimed. Note that both
/// invocations happen in the same thread, therefore
/// GenericMsg::safeToReclaim() does not need to be thread-safe.

/// Only the initP2PQueue<MsgType message P2P queue" object can
/// initialize the head and tail objects.
template<typename MsgType>
void initGenericMsgP2PQueue(MsgType *initMsg,
			    GenericMsgP2PQueueHead<MsgType> *head,
			    GenericMsgP2PQueueTail<MsgType> *tail);

//
template<typename MsgType>
MsgType* flushMsgP2PQueue(GenericMsgP2PQueueHead<MsgType> *head,
			   GenericMsgP2PQueueTail<MsgType> *tail);

/// The thread-safe "point-to-point" queue's "head" object - provides
/// the dequeue() operation.
template<typename MsgType>
class GenericMsgP2PQueueHead {
  template<typename MsgType>
  friend void initP2PQueue(MsgType *initMsg,
			   GenericMsgP2PQueueHead<MsgType> *head,
			   GenericMsgP2PQueueTail<MsgType> *tail);
  template<typename MsgType>
  friend MsgType* flushMsgP2PQueue(GenericMsgP2PQueueHead<MsgType> *head,
				   GenericMsgP2PQueueTail<MsgType> *tail);
public:
  /// objects must be initialized with GenericMsgP2PQueue::init()
  GenericMsgP2PQueueHead() : last((MsgType *) NULL) {}
  ~GenericMsgP2PQueueHead() {}

  /// returns NULL if (currently) empty:
  MsgType* dequeue() {
    assert(last != (MsgType *) NULL);
    MsgType *next = static_cast<MsgType *>(last->getNext());
    if (next != (MsgType *) NULL) {
#if defined(__GNUC__)
      // prefetch the body of the dequeued element. This should be
      // useful even when the queue is used as a "return queue" in
      // SharedMemMsgPool objects - where the object content will not
      // read, but the next field still will be.
      __builtin_prefetch(next);
#endif
      // note that at this point we do nothing with the last->next
      // field - it becomes a dangling pointer (which is fine since
      // only this dequeue() method could access it - but it cannot
      // since the message object is dequeued).
      last = next;
    }
    return (next);
  }

  /// prefetch the (first cache line of the) next element.
  void prefetchNext() const {
#if defined(__GNUC__)
    MsgType *next = last->getNext();
    if (next != (MsgType *) NULL)
      __builtin_prefetch(next);
#endif
  }

  ///
  bool isEmpty() const {
    MsgType * const next = last->getNext();
    return (next == (MsgType *) NULL);
  }

private:
  /// location of a GenericMsg object that was last dequeued.
  MsgType *last;
};

/// The thread-safe "point-to-point" queue's "tail" object - provides
/// the enqueue() operation.
template<typename MsgType>
class GenericMsgP2PQueueTail {
  template<typename MsgType>
  friend void initP2PQueue(MsgType *initMsg,
			   GenericMsgP2PQueueHead<MsgType> *head,
			   GenericMsgP2PQueueTail<MsgType> *tail);
  template<typename MsgType>
  friend MsgType* flushMsgP2PQueue(GenericMsgP2PQueueHead<MsgType> *head,
				   GenericMsgP2PQueueTail<MsgType> *tail);
public:
  GenericMsgP2PQueueTail() : last((MsgType *) NULL) {}
  ~GenericMsgP2PQueueTail() {}

  /// msg must be a pointer to a valid object;
  void enqueue(MsgType *msg) {
    assert(msg != (MsgType *) NULL);
    assert(last != (MsgType *) NULL);
    // note that GenericMsgP2PQueueHead::dequeue() does not do
    // anything with the dequeued message's 'next' field, so it
    // contains a dangling pointer - until now:
    msg->setNext((MsgType *) NULL);
    // message processing thread may not observe a dangling
    // 'msg->next' pointer when 'msg' gets visible for that thread.
    // This barrier also "flushes" all store operations of message
    // construction.
    StoreStore_BARRIER();
    last->setNext(msg);      // 'msg' is enqueued (i.e. visible for 'dequeue()')
    last = msg;		     // .. next enqueue() will enqueue after 'msg';
  }

private:
  /// location of a GenericMsg object that was last enqueued.
  MsgType *last;
};

#endif // GENERICMSGP2PQUEUE_H
