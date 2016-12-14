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
 * File:   EventMsgHandler.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef EVENTMSGHANDLER_H
#define EVENTMSGHANDLER_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "GenericMsgHandler.h"
#include "EventMsg.h"
#include "GenericTableListener.h"

/// EventMsgHandler handles all messages pertinent to monitoring
/// database transactions concerning cluster status:
///   PendingEventTableTailer
///   RMNodeTableTailer
///   ResourceTableTailer
///   UpdatedContainerInfoTableTailer
///   ContainerStatusTableTailer
/// 
template<template<class TemplateMsgType> class MsgPool>
class EventMsgHandler : public GenericMsgHandler<EventMsg, MsgPool> {
public:
  EventMsgHandler(JavaVM* jvm,
		  MsgPool<EventMsg> *msgPool,
		  unsigned int msgAccSizeBits,
		  unsigned int keyReduce);
  ~EventMsgHandler() {}

  /// 
  void handleMsg(EventMsg *msg);
  ///
  void reclaimMsgObj(MsgType *msg) { msgPool->freeMsgObject(msg); }

private:
  JavaVM* const jvm;
  MsgPool<EventMsg> * const msgPool;
  EventMsgAccumulator msgAcc;
};

/// Auxiliary "event message container" for keeping track of arriving
/// messages - until all messages with the same pending event id are
/// present. There are NumberMsgTypes such messages. The
/// implementation keeps a thread-local free list;
class EventMsgContainer {
public:
  EventMsgContainer()
    : cnt(0), next((EventMsgContainer *) NULL) {}
  EventMsgContainer(PendingEventID key)
    : key(key),
      entries(0),
      cnt(0),
      next((EventMsgContainer *) NULL),
      msgs[MSG_UpdatedContainerInfoTableTailer]((EventMsg *) NULL),
      msgs[MSG_ContainerStatusTableTailer]((EventMsg *) NULL)
  {}
  ~EventMsgContainer() {}
  //
  void *operator new(size_t size) { return (malloc(size)); }
  void* operator new(size_t, void *place) { return (place); }

  /// reinit() makes the object as it were just created;
  void reinit() { (void) new (this) EventMsgContainer(); }
  void reinit(PendingEventID key) { (void) new (this) EventMsgContainer(key); }

  //
  bool isUsed() const { return (cnt > 0); }
  // as long as there is no message from the pending event table
  // (i.e. entries == 0), the container is not complete per definition;
  bool isComplete() const { return (entries != 0 && cnt == entries); }

  //
  PendingEventID getKey() const {
    assert(cnt == NumberMsgTypes);
    return (key);
  }
  //
  void setNEntries(unsigned short n) { entries = n; }
  unsigned short getNEntries() { return (entries); }

  /// setMsg() is applicable for event messages that appear in the
  /// table exactly once;
  void setMsg(EventMsgType idx, EventMsg *msg) {
    assert(!isComplete());
    msgs[idx] = msg;
    cnt++;
  }
  /// setMsg() is applicable for event messages that appear in the
  /// table 0 or more times;
  void addMsg(EventMsgType idx, EventMsg *msg) {
    assert(!isComplete());
    msg->setNextContainerMsg(msgs[idx]);
    msgs[idx] = msg;
    cnt++;
  }
  EventMsg *getMsg(EventMsgType idx) const {
    assert(isComplete());
    return (msgs[idx]);
  }

  //
  void setNext(EventMsgContainer *c) { next = c; }
  EventMsgContainer *getNext() const { return (next); }

private:
  PendingEventID key;
  /// total number of expected entries. Originally it is 0, and it can
  /// remain so for awhile - until the message from pending event
  /// table arrives;
  unsigned short entries;
  unsigned short cnt;		//!< current number of entries;
  EventMsgContainer *next;
  /// msgs[] is indexed by a EventMsgType value;
  EventMsg * const msgs[NumberMsgTypes];
};

/// A custom hash table for EventMsgContainer objects.  When an event
/// message with a given pending event id arrives (in some undefined
/// order), the handler looks up the container for the id. EventMsg is
/// stored there, and if the container becomes full - it is processed,
/// and deleted from the table.
///
/// Given NumberMsgTypes > 1 messages per container, the most common
/// operation is successful lookup - successful with the probability
/// (NumberMsgTypes - 1)/NumberMsgTypes. If a lookup fails, we
/// immediately insert a new entry - and return it. Thus, the table's
/// lookup operation always returns a container. After NumberMsgTypes
/// lookups, the entry is deleted.
///
/// Since pending event id"s are allocated sequentially, and there is
/// temporal locality for messages with numerically close id"s - which
/// we'd like to exploit for better spatial locality & better cache
/// performance, we don't do proper hashing: table index is equal to
/// pending id modulo table size (essentially, the table is a circular
/// buffer with protection for overflow).
class EventMsgAccumulator {
public:
  /// the logarithm of table size, and the number of event
  /// handlers/threads in the system. Using the latter, we exploit the
  /// facts that (a) event message hash key is just the pending event
  /// id which, in turn, is allocated sequentially, and (b)
  /// asynchronous message handler (GenericAsyncMsgHandler)
  /// distributes messages between handlers in a round-robin fashion.
  /// Thus, each handler receives messages with every keyReduce"th
  /// pending event id, and, therefore, pending event id"s divided by
  /// keyReduce constitute an isomorphic but continuous domain of
  /// message keys - that we use in getIndex() for our circular buffer
  /// usage.
  EventMsgAccumulator(unsigned int sizeBits, unsigned int keyReduce);
  ~EventMsgAccumulator();

  /// lookup() finds an existing container or allocates a new, empty
  /// one - but always returns a valid container. Note that the
  /// clients may not retain the EventMsgContainer pointers (received
  /// from lookup()) across other invocations of lookup() and remove()
  /// - because the implementation actually moves entries 
  EventMsgContainer* lookup(PendingEventID key);
  /// remove() removes the container (and manages its memory) - but
  /// does nothing with message objects inside it (if
  /// any). EventMsgContainer object must be in the table;
  void remove(PendingEventID key, EventMsgContainer *container);

private:
  static unsigned long getIndex(PendingEventID value) { 
    return ((value / keyReduce) & sizeMask); 
  }
  /// getContainer() returns a usable - but not initialized -
  /// container. Currently, free list is not trimmed dynamically.
  EventMsgContainer* getContainer(PendingEventID key);
  void freeContainer(EventMsgContainer *msg);

private:
  /// 'table' points to an array of EventMsgContainer objects of size
  /// EventMsgAccumulator::size
  EventMsgContainer * const table;
  unsigned int sizeBits;
  unsigned long sizeMask;
  unsigned int keyReduce;
  EventMsgContainer* freeList;
};

#endif // EVENTMSGHANDLER_H
