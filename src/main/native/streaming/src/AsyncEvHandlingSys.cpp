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
 * File:   AsyncEvHandlingSys.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#include "AsyncEvHandlingSys.h"

// PARAMETERS, TODO.
unsigned int numListeners = 5;
unsigned int messageAccumulatorSizeBits = 18;

//
AsyncEvHandlingSys::AsyncEvHandlingSys(Ndb* ndb,
				       JavaVM* jvm,
				       unsigned int nMsgHandlers,
				       unsigned int msgHandlerTableSize)
  : ndb(ndb),
    jvm(jvm),
    nMsgHandlers(nMsgHandlers),
    msgHandlerTableSize(msgHandlerTableSize),
    freeMsgPools(new FreeMsgPool*[nMsgHandlers]),
    syncObj(new DataflowController*[nMsgHandlers]),
    msgHandlers(new MsgHandler*[nMsgHandlers]),
    msgHandlingThreads(new MsgHandlingThread*[nMsgHandlers])
{
  unsigned int i;

  // the "allocate the message" message pool object - one single one
  // since for now we try to run with one single listening thread (and
  // all listeners share it);
  allocMsgPool = new AllocMsgPool(nMsgHandlers);

  // the "free the message" message pool objects - one per event
  // message handling thread;
  for (i = 0; i < nMsgHandlers; i++)
    // using a single listener thread implies each "free" message pool
    // has one single "return" queue;
    freeMsgPools[i] = new FreeMsgPool(1);

  // link up the "allocate" and "free" message pool objects (so that
  // freed objects can "return" to the allocation side):
  for (i = 0; i < nMsgHandlers; i++) {
    MsgQueueHead *msgQueueHead = allocMsgPool->getQueueHead(i);
    // .. again, one singe return queue 'cause one single listener:
    MsgQueueTail *msgQueueTail = freeMsgPools[i]->getQueueTail(0);
    EventMsg *initMsg = new EventMsg();
    //
    initEventMsgP2PQueue<EventMsg>(initMsg, msgQueueHead, msgQueueTail);
  }

  // the "asynchronous message handler" - one single one, shared by
  // all listeners (at the single listening thread). This one is not a
  // "real" handler but a proxy forwarding messages to the message
  // handling threads (and their real handlers). Nevertheless, it does
  // implement the same GenericMsgHandler interface.
  asyncMsgHandler = new AsyncMsgHandler(nMsgHandlers);
  // .. at this point, this handler is not yet usable since its queue
  // ends are not "connected" to the handler threads' ones;

  // (single) ndb listening thread
  listenerThread = new ListenerThread(numListeners);

  // all ndb listeners;
  petListener =  new TableListener(ndb,
				   allocMsgPool,
				   asyncMsgHandler,
				   pendingEventTable,
				   _pendingEvents_timeout);
  rmntListener = new TableListener(ndb,
				   allocMsgPool,
				   asyncMsgHandler,
				   rmnTable,
				   _rmnode_timeout);
  rtListener =   new TableListener(ndb,
				   allocMsgPool,
				   asyncMsgHandler,
				   resourceTable,
				   _resource_timeout);
  ucitListener = new TableListener(ndb,
				   allocMsgPool,
				   asyncMsgHandler,
				   updatedContainerInfoTable,
				   _updatedContainerInfo_timeout);
  cstListener =  new TableListener(ndb,
				   allocMsgPool,
				   asyncMsgHandler,
				   containerStatusTable,
				   _containerStatus_timeout);

  // one single dataflow controller per pair consisting of a
  // listening- and a message handling thread:
  for (i = 0; i < nMsgHandlers; i++)
    syncObj[i] = new DataflowController();

  // tell the listening thread about the listeners it should use
  listenerThread->registerListener(0, petListener, syncObj[0]);
  listenerThread->registerListener(1, rmntListener, syncObj[1]);
  listenerThread->registerListener(2, rtListener, syncObj[2]);
  listenerThread->registerListener(3, ucitListener, syncObj[3]);
  listenerThread->registerListener(4, cstListener, syncObj[4]);

  // ("real") event message handlers;
  for (i = 0; i < nMsgHandlers; i++)
    msgHandlers[i] = new MsgHandler(jvm,
				    freeMsgPools[i],
				    messageAccumulatorSizeBits,
				    nMsgHandlers);

  // event message handling threads;
  for (i = 0; i < nMsgHandlers; i++)
    // message handling threads receive messages from a single listener;
    msgHandlingThreads[i] = new MsgHandlingThread(1, msgHandlers[i]);

  // link up the message queues:
  MsgQueueTailTail *msgQueueTail = asyncMsgHandler->getQueueTail(0);
  for (i = 0; i < nMsgHandlers; i++) {
    MsgQueueHead *msgQueueHead = msgHandlingThreads[i]->getQueueHead(i);
    EventMsg *initMsg = new EventMsg();
    //
    initEventMsgP2PQueue<EventMsg>(initMsg, msgQueueHead, msgQueueTail);
  }
}

AsyncEvHandlingSys::~AsyncEvHandlingSys()
{
  unsigned int i;

  //
  MsgQueueTailTail *msgQueueTail = asyncMsgHandler->getQueueTail(0);
  for (i = 0; i < nMsgHandlers; i++) {
    MsgQueueHead *msgQueueHead = msgHandlingThreads[i]->getQueueHead(i);
    //
    EventMsg *msg = flushMsgP2PQueue<EventMsg>(initMsg, msgQueueHead, msgQueueTail);
    delete msg;
  }
  for (i = 0; i < nMsgHandlers; i++) {
    MsgQueueHead *msgQueueHead = allocMsgPool->getQueueHead(i);
    MsgQueueTail *msgQueueTail = freeMsgPools[i]->getQueueTail(0);
    //
    EventMsg *msg = flushMsgP2PQueue<EventMsg>(initMsg, msgQueueHead, msgQueueTail);
    delete msg;
  }

  for (i = 0; i < nMsgHandlers; i++) {
    delete msgHandlingThreads[i];
    delete msgHandlers[i];
    delete syncObj[i];
  }
  delete listenerThread;
  delete asyncMsgHandler;
  delete petListener;
  delete rmntListener;
  delete rtListener;
  delete ucitListener;
  delete cstListener;
  for (i = 0; i < nMsgHandlers; i++)
    delete freeMsgPools[i];
  delete allocMsgPool;
}

void AsyncEvHandlingSys::start()
{
  for (i = 0; i < nMsgHandlers; i++)
    msgHandlingThreads[i]->start();
  //
  listenerThread->start();
}

void AsyncEvHandlingSys::stop()
{
  listenerThread->stop();
  //
  for (i = 0; i < nMsgHandlers; i++)
    msgHandlingThreads[i]->stop();
}
