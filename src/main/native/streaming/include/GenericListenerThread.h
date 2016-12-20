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
 * File:   GenericListenerThread.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICLISTENERTHREAD_H
#define GENERICLISTENERTHREAD_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include <pthread.h>
#include "common.h"
#include "DataflowController.h"

/// "generic listener thread" that is used to poll events from the ndb
/// event queue using "generic listeners" that, in turn, are refined
/// to "table tailers". One single thread can be configured to handle
/// multiple tables, if the system load allows. Each listener is
/// supposed to perform non-blocking polls, and perform a finite
/// amount of work (e.g. poll at most certain number of ndb
/// events). When all listeners run out of work - signified by a
/// complete round of table tailer invocations without any new events
/// processed - the thread selects the listener with the least
/// "pollTimeout", and invokes that listener in the "blocking mode"
/// with its polling timeout. Once the listener returns, either
/// because it received new events or timeout, the listener thread
/// resumes its operation as usual (which may result in its blocking
/// if there are still no events to receive).
template<class Listener>
class GenericListenerThread {
  template<class ListenerP> friend void* runproc(void* gptr);
public:
  GenericListenerThread(unsigned int nListeners);
  /// (native) thread is NOT terminated in the current implementation;
  ~GenericListenerThread();

  /// all "generic listeners" must be registered before starting the
  /// thread.  Also, the GenericListenerThread object assumes the
  /// "ownershio" of the listener (including destruction);
  void registerListener(unsigned int n,
			Listener *listener,
			DataflowController *syncObj);
  /// a (native) thread, returning true if all is right. It must be
  /// called exactly once.
  bool start();
  /// asynchronously (eventually) terminates the thread;
  bool stop();

private:
  Listener** const listeners;   //!< listeners' array, of size nTailers;
  unsigned int const nListeners;
  /// A DataflowController object, providing for controling dataflow
  /// to the message handling threads;
  DataflowController** const syncObj;

  //
  unsigned long pollTimeout;    //!< minimal polling timeout among registered;
  unsigned int blockingListenerId;   //!< of the listener with pollTimeout;
  pthread_t nativeThread;
  /// is true if the thread is started (and nativeThread is initialized);
  bool started;
  /// when is set to false, the thread will eventually stop;
  volatile bool running;
};

#include "GenericListenerThread.tcpp"

#endif // GENERICLISTENERTHREAD_H
