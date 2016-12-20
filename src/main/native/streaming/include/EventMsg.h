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
 * File:   EventMsg.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef EVENTMSG_H
#define EVENTMSG_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"
#include "Platform.h"
#include "GenericMsg.h"
#include "AsyncMsgBase.h"

//
typedef int32_t PendingEventID;
typedef unsigned int WatchTableMsgFieldIndex;

//
typedef enum {
  MSG_PendingEventTable = 0,
  MSG_RMNodeTable = 1,
  MSG_ResourceTableTailer = 2,
  MSG_UpdatedContainerInfoTableTailer = 3,
  MSG_ContainerStatusTableTailer = 4
} EventMsgType;

// effectively, NumberMsgTypes determines the size of the "event
// message container" by message handlers - for holding individual
// messages until all messages with the same pendeing event id arrive;
unsigned int const NumberMsgTypes = 5;

//
typedef union {
  char *strValue;
  int32_t int32Value;
  int64_t int64Value;
} EventMsgField;

/// EventMsg_OBJ_SIZE defines the maximum object size.
/// TODO: replace 512 with proper value - large enough for all table
/// listeners;
unsigned int const EventMsg_OBJ_SIZE = 512 - MALLOC_OVERHEAD;

/// The EventMsg class defines "container" objects that store data
/// from ndb update events. Objects are constructed when ndb events
/// are received (using Ndb::pollEvents2() and Ndb::nextEvent2()
/// methods).  Objects are destroyed when the data is conveyed to the
/// Java layer *and* the object (its EventMsg::next field) is no
/// longer needed for the GenericMsgP2PQueue implementation, see
/// GenericMsgP2PQueue.h for details.
///
/// EventMsg extends the GenericMsg base class/interface
/// specification, in particular, it provides the hashKey() method
/// (and keeps the data needed for that).
///
/// EventMsg objects, essentially, just accumulate the data from ndb
/// events. EventMsgHandler objects accumulate them in a hash table
/// until all relevant objects are collected. Currently, update events
/// from the following tables can be handled:
///
///   Pending Event
///   RMNode
///   Resource
///   Updated Container Info
///   Container Status
/// 
/// Handling these tables requires at most MAX_NUM_COLS EventMsgField
/// event message fields. Furthermore, there is a small heap for
/// keeping char string column values. This way, the ndb event
/// listening service requires one single message type and, hence, one
/// single object message pool - with no further memory management.
class EventMsg : public GenericMsg<EventMsg,AsyncMsgBase> {
public:
  /// the "empty" message constructor is used when message bodies are
  /// allocated by message pool(s);
  EventMsg();
  /// this "proper" message constructor is not used directly, in fact
  /// - but is used by reinit()
  EventMsg(EventMsgType t, PendingEventID id)
    : msgType(t),
      pendingEventId(id),
      nextContainerMsg((EventMsg *) NULL),
      allocIdx(0) {}
  ~EventMsg() {}

  /// reinit() makes the object as it were just created;
  void reinit(EventMsgType t, int32_t id) {
    (void) new (this) EventMsg(t, id);
  }

  /// operator new() provides for custom (native) memory allocation of
  /// EventMsg objects: makes some space for mem[] character heap
  /// (configurable through EventMsg_OBJ_SIZE). In fact, it must be
  /// provided explicitly since we redefine the new(size_t, void *)
  /// version;
  void *operator new(size_t size);

  /// MAX_NUM_COLS is the maximum number of columns, excluding the
  /// "pending event id", in ndb events we're receiving:
  static unsigned int const MAX_NUM_COLS = 8;

  // 
  EventMsgType getType() const { return ((EventMsgType) msgType); }
  //
  PendingEventID getPendingEventID() const { return (pendingEventId); }
  // note that efficient operation of event message handlers -
  // specifically, their message accumulators (EventMsgAccumulator
  // objects), depends on how messages are distributed across message
  // handlers (seeEventMsgHandler.hh), which, in turn, depends on the
  // hashKey() function, and its usage in
  // GenericAsyncMsgHandler<>::handleMsg(MsgType *)
  unsigned long hashKey() const { return (getPendingEventID()); }

  // needless to say, the implementation of the get/set methods can
  // be, if needed, improved to make sure the union is accessed in a
  // type-safe way;
  void setStringValue(unsigned int idx, char *str) { 
    assert(idx < MAX_NUM_COLS);
    data[idx].strValue = str;
  }
  char *getStringValue(unsigned int idx) const {
    assert(idx < MAX_NUM_COLS);
    return (data[idx].strValue);
  }
  void setInt32Value(unsigned int idx, int32_t i) {
    assert(idx < MAX_NUM_COLS);
    data[idx].int32Value = i;
  }
  int32_t getInt32Value(unsigned int idx) const {
    assert(idx < MAX_NUM_COLS);
    return (data[idx].int32Value);
  }
  void setInt64Value(unsigned int idx, int64_t i) {
    assert(idx < MAX_NUM_COLS);
    data[idx].int64Value = i;
  }
  int64_t getInt64Value(unsigned int idx) const {
    assert(idx < MAX_NUM_COLS);
    return (data[idx].int64Value);
  }

  /// allocate memory within the object itself - for character string
  /// arguments;
  char *calloc(size_t size) {
    /// index of the first character in mem[] that is not available:
    const size_t newIdx = ((size_t) allocIdx) + size;
    if (newIdx <= outOfMemIdx) {
      char *ptr = &mem[allocIdx];
      allocIdx = newIdx;
      return (ptr);
    } else {
      return ((char *) NULL);
    }
  }

  /// setNextContainerMsg() and getNextContainerMsg() support EventMsg
  /// links in EventMsgContainer;
  void setNextContainerMsg(EventMsg *msg) { nextContainerMsg = msg; }
  EventMsg* getNextContainerMsg() { return (nextContainerMsg); }

  /*
   * Currently, all column data that needs dynamic memory (in
   * particular, character strings) is allocated using
   * EventMsg::calloc() (i.e. in a "micro-heap" provided by EventMsg
   * objects themselves, see also Utils.h, get_cstring()) - thus,
   * there is no need to distinguish memory allocated by either
   * EventMsg::calloc() or malloc()
   *
  /// there is no deallocation for EventMsg::calloc() -allocated
  /// memory, but we need to know there was no fallback to malloc()
  bool isCallocMem(char *ptr) {
    const size_t outOfMemIdx = EventMsg_OBJ_SIZE - sizeof(EventMsg);
    // should be &mem[0], but using this is sufficient for the purpose:
    char * const start = (char * const) this;
    char * const end = &mem[outOfMemIdx];
    return (ptr >= start && ptr < end);
  }
  */

private:
  /// Note that EventMsg::msgType and AsyncMsgBase::senderIdx are
  /// adjacent short integers - to conserve space.
  unsigned short const msgType;
  PendingEventID const pendingEventId;
  // (since nextContainerMsg is used when the message is already in
  //  the container, it could be overloaded with msgType,
  //  pendingEventId and allocIdx - but it's not worth the trouble;)
  EventMsg *nextContainerMsg;	//!< for EventMsg lists in EventMsgContainer"s
  EventMsgField data[MAX_NUM_COLS];
  unsigned short allocIdx;	//!< allocation position in mem[];
  char mem[];

  // note the size_t type, as calloc() may be asked to allocate large
  // chunks of memory;
  static const size_t outOfMemIdx;

private:
  /// the new() operator for re-initialization:
  void* operator new(size_t, void *place) { return (place); }
};

#endif // EVENTMSG_H
