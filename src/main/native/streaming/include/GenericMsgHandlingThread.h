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
 * File:   GenericMsgHandlingThread.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef EVENTMSGHANDLINGTHREAD_H
#define EVENTMSGHANDLINGTHREAD_H

/// GenericMsgHandlingThread objects provide threads that receive
/// messages from listener threads, and apply to them message
/// handlers. 
template<class MsgType, class MsgHandler>
class GenericMsgHandlingThread {
public:
  GenericMsgHandlingThread(unsigned int nQueues,
			   MsgHandler *msgHandler);
  ~GenericMsgHandlingThread();

  ///
  DataflowController* getDataflowController() { return (syncObj); }
  /// references to queue heads are needed in order to initialize
  /// those together with corresponding queue tails;
  GenericMsgP2PQueueHead<MsgType>* const getQueueHead(unsigned int n) const;

  /// start the native thread.
  /// In the current implementation all queues must be initialized beforehand.
  void start();
  void stop();

private:
  static void* runproc(void *gptr); //!< the native threads' procedure;

  unsigned int nextQueueIdx(unsigned int n) { return ((n + 1) % nQs); }

private:
  /// message queues (of some specific type), accessed round-robin;
  GenericMsgP2PQueueHead<MsgType>* const mqs;
  unsigned int const nQs;	    //!< number of input queues (of each type);
  /// last dequeued message objects, one per queue - which cannot be
  /// declared for reclaimation until next dequeue, due to
  /// GenericMsgP2PQueue limitation;
  MsgType ** const lastMsg;
  /// message handler - must be 
  MsgHandler * const msgHandler;    //!< Message processing handler
  /// A DataflowController object, providing for controling dataflow
  /// from the listener threads;
  DataflowController * const syncObj;
  /// is true if the thread is started (and nativeThread is initialized);
  bool started;
  /// when is set to false, the thread will eventually stop;
  volatile bool running;
};

#endif // EVENTMSGHANDLINGTHREAD_H
