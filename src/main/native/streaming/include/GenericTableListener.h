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
 * File:   GenericTableListener.h
 * Author: Mahmoud Ismail<maism@kth.se>, Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICTABLELISTENER_H
#define GENERICTABLELISTENER_H

#include <pthread.h>
#include "Utils.h"
#include <jni.h>
#include "TableTailer.h"
#include "EventMsg.h"
#include "GenericLister.h"

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

typedef unsigned int WatchTableColIndex;

typedef enum {
  WatchTable_Int32Field = 0,
  WatchTable_Int64Field = 1,
  WatchTable_StringField = 2,
  // in particular, the pending event column is treated specially:
  WatchTable_EXCLUDED
} WatchTableMsgFieldType;

/// For each element of the WatchTable::mColumnNames array (which
/// defines a column name we're interested in), we define its
/// type and location of the corresponding data field in EventMsg.
typedef struct {
  WatchTableMsgFieldType type;
  WatchTableMsgFieldIndex msgIdx;
} WatchTableMsgField;

/// enum Operation is defined in TableTailer.h, but WatchTable is extended:
class ListenerWatchTable : public WatchTable {
public:
  const WatchTableColIndex pendingEventIdCol;
  /// kost@ : cannot initialize a const array in a c++ dialect prior c++11 ??
  // const WatchTableMsgField fields[EventMsg::MAX_NUM_COLS];
  WatchTableMsgField const * const msgFields;
  const EventMsgType msgType;

public:
  /// (object initialization a'la structs' is prohibited since c++98)
  ListenerWatchTable(const string mTableName,
		     const string* mColumnNames,
		     const int mNoColumns,
		     const NdbDictionary::Event::TableEvent* mWatchEvents,
		     const int mNoEvents,
		     const WatchTableColIndex pendingEventIdCol,
		     const WatchTableMsgField msgFields[],
		     const EventMsgType msgType);
};

/// GenericTableListener is the specialization of the GenericListener
/// which, in turn, can be handled by GenericListenerThread objects
/// encapsulating native threads.GenericTableListener is specialized
/// further down to specific "table tailers"..
template<template<class TemplateMsgType> class MsgPoolAlloc,
	 template<class TemplateMsgType> class MsgPoolFree,
	 template<class MsgType, class MsgPool> class MsgHandler>
class GenericTableListener : public GenericListener {
public:
  GenericTableListener(Ndb* const ndb,
		       MsgPoolAlloc<EventMsg> * const msgPool,
		       MsgHandler<EventMsg, MsgPoolFree> * const msgHandler,
		       const ListenerWatchTable table,
		       const unsigned long pollTimeout);
  /// drops the NDB "event operation" and "event", and then destroys
  /// the Ndb object that was given the constructor;
  virtual ~GenericTableListener();

   unsigned int handleEventsNonBlocking() {
    return (handleEvents(0));
  }
  unsigned int handleEventsTimeout() {
    return (handleEvents(getTimeout()));
  }

protected:
  /// handleEvent() methods from subclasses provide table-specific
  /// event handling;
  virtual void handleEvent(NdbDictionary::Event::TableEvent eventType) = 0;

protected:
  /// message pool that allocates messages;
  MsgPoolAlloc<EventMsg> * const msgPool;
  /// handle to the message handler - with asynchronous processing,
  /// that's GenericAsyncMsgHandler;
  MsgHandler<EventMsg, MsgPoolFree> * const msgHandler;

  Ndb* const mNdbConnection;
  /// NdbRecAttr array where new values - of the chosen columns, when
  /// a column changes in the table in question;
  NdbRecAttr** const recAttr;
  /// NdbRecAttr array where previous values - of the chosen columns,
  /// when a column changes in the table in question;
  NdbRecAttr** const recAttrPre;

private:
  /// internal method that does the real event polling - configurable
  /// with a timeout value;
  void handleEvents(unsigned long timeout);
  const char* getEventName(NdbDictionary::Event::TableEvent event);

private:
  NdbEventOperation* ndbOp;	//!< kept for the destructor;
  const string mEventName;
  static const ListenerWatchTable mTable;
  /// how many times Ndb::pollEvents2() is called per handleEvent()
  /// invocation - TODO ?
  const unsigned int pollEventsIterations = 1;
};

#endif // GENERICTABLELISTENER_H

