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
 * File:   AsyncEvHandlingSys.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef ASYNCEVHANDLINGSYS_H
#define ASYNCEVHANDLINGSYS_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"
#include "EventMsg.h"
#include "EventMsgP2PQueue.h"
#include "GenericAsyncMsgHandler.h"
#include "SharedMemMsgPool.h"
#include "RemoteMemMsgPool.h"
#include "EventMsgHandler.h"
#include "GenericListenerThread.h"
#include "GenericTableListener.h"
#include "GenericMsgHandlingThread.h"
#include "PendingEventTableListener.h"
#include "UpdatedContainerInfoTableListener.h"
#include "ContainerStatusTableListener.h"
#include "ResourceTableListener.h"
#include "RMNodeTableListener.h"

// TODO - PARAMETERS
//
// NOTE These are start up time parameters, so they can be changed
// until then;

/// the number of listeners - one per ndb connection/table - determines
/// the number of auxiliary components like message queues.
extern unsigned int numListeners;

/// the logarithm of the message handler's "accumulate allnumListeners
/// messages with the same pending event id" table.
extern unsigned int messageAccumulatorSizeBits;

// kludge for c++03 and earlier - without c++11's "alias declarations"
// (which is the c++ name for partial template instantiation): the
// GenericTableListener template's third parameter is a "MsgHandler"
// template with a single parameter being a MsgPool template. The
// MsgHandler template class is expected to implement the
// GenericMsgHandler interface specifically for EventMsg objects.  The
// scalable handling of EventMsg objects goes with message handlers
// working in separate threads, and, therefore, the listener(s) have
// to use the GenericAsyncMsgHandler implementation of the
// GenericMsgHandler interface - and GenericAsyncMsgHandler is,
// indeed, generic, hence, parameterized also by the message type.
// c++03 does not allow to partially instantiate the
// GenericAsyncMsgHandler template with MsgType being EventMsg,
// i.e. there is no way to derive a single-argument template from a
// two-argument one. Thus, we're forced to construct a new template
// with a trivial subclass of GenericAsyncMsgHandler:
template<template<class TemplateMsgType> class MsgPool>
class EventAsyncMsgHandler : public GenericAsyncMsgHandler<EventMsg, MsgPool> {
public:
  EventAsyncMsgHandler(unsigned int nQueues)
    : GenericAsyncMsgHandler<EventMsg, MsgPool>(nQueues) {}
  ~EventAsyncMsgHandler() {}
};

// define some common derived types - avoiding template clutter..
typedef SharedMemMsgPool<EventMsg> AllocMsgPool;
typedef RemoteMemMsgPool<EventMsg> FreeMsgPool;
typedef EventMsgHandler<RemoteMemMsgPool> MsgHandler;
typedef EventMsgP2PQueueHead MsgQueueHead;
typedef EventMsgP2PQueueTail MsgQueueTail;
typedef EventAsyncMsgHandler<RemoteMemMsgPool> AsyncMsgHandler;
typedef GenericTableListener<SharedMemMsgPool,RemoteMemMsgPool,EventAsyncMsgHandler> TableListener;
typedef GenericListenerThread<TableListener> ListenerThread;
typedef GenericMsgHandlingThread<EventMsg,MsgHandler> MsgHandlingThread;

/// The "overall" management interface of the "ndb update event
/// handling" subsystem. The implementation deploys, currently, a
/// single listener thread (in a ListenerThread object) and a fixed
/// pool of event handling threads (and can be easily extended for
/// multiple listener threads, up to one dedicated thread per ndb
/// connection). The setup() method constucts listeners and event
/// handlers, with their respective threads and message object pools,
/// and all point-to-point queues for communication between the
/// latter.

/// Upon construction, the service infrastructure is constructed:
/// ListenerThread objects (with encapsulated threads) for ndb event
/// polling (using Ndb::pollEvents2() and Ndb::nextEvent2()), and
/// EventMsgHandlingThread objects (with encapsulated threads) for
/// data aggregation and JNI upcalls, as well as the internal message
/// queues and memory management.
class AsyncEvHandlingSys {
public:
  AsyncEvHandlingSys(Ndb* ndb,
		     JavaVM* jvm,
		     unsigned int nMsgHandlers,
		     unsigned int msgHandlerTableSize);
  ~AsyncEvHandlingSys();

  void start();
  void stop();			// (currently cannot be restarted;)

private:
  Ndb* const ndb;
  JavaVM* const jvm;
  unsigned int const nMsgHandlers;
  unsigned int const msgHandlerTableSize;

  //
  AllocMsgPool *allocMsgPool;
  FreeMsgPool** const freeMsgPools;
  ListenerThread *listenerThread;
  AsyncMsgHandler *asyncMsgHandler;
  TableListener *petListener, *rmntListener, *rtListener, *ucitListener, *cstListener;
  DataflowController** const syncObj;
  MsgHandler** const msgHandlers;
  MsgHandlingThread** const msgHandlingThreads;
};

#endif // ASYNCEVHANDLINGSYS_H
