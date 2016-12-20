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
 * File:   DataflowController.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef DATAFLOWCONTROLLER_H
#define DATAFLOWCONTROLLER_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include <pthread.h>
#include "common.h"
#include "Platform.h"

/// DataflowController provides for controlling dataflow between
/// multiple producers and a single consumer.
/// 
/// The requirements on the synchronization are as follows. When the
/// consumer runs out of work, it should block (as opposite to busy
/// waiting), and be waken up by a producer(s) immediately when more
/// data becomes available. In particular, waking up consumers may not
/// be implemented by timer-based periodic status checks. Furthermore,
/// producers are allowed to block indefinitely on their own (because
/// e.g. of missing its own inputs), thus, producers may stop calling
/// any DataflowController methods for an indefinite amount of time.
/// Note that these conditions imply that by the time both the
/// producers and the consumer are blocked, all queues must be
/// empty. Also, the controller should incur as little overhead as
/// possible when no blocking and wake-up signalling is required -
/// i.e. the system is loaded.
///
/// A DataflowController object is shared between a consumer and all
/// its producers. When the consumer runs out of work and needs to
/// block, it first calls waitingForData(), then checks the queue(s)
/// one more time, and if still empty - calls waitForData() which
/// blocks the thread. Note the checking the queue means just that -
/// if some data is suddenly discovered, then resumeWithData() should
/// be called first, and then the data dequeued and processed as
/// normal. When the consumer's thread is waken up, the consumer must
/// call the resumeWithData() method. On the producers' side, running
/// producers must inspect the consumer's status using the
/// checkConsumer() method, which checks the consumer's status and
/// wakes it up if needed. If a producer is to block on its own,
/// however, then it must use the checkConsumerSync() method instead
/// (which also flushes the data queue to memory, as discussed below).
/// 
/// The envisioned "usual case" scenario is of a single producer and
/// consumer(s) that are heavily loaded. The current implementation is
/// not optimized for the opposite case of multiple producers, and
/// consumers that block frequently: in this case there is lock
/// contention as producers that discover the consumer is to be waken
/// up need to acquire the same lock. A more elaborated scheme could
/// have been invented (e.g. implementing a reader-writer lock, or
/// deploying some form of speculative selection of producer for doing
/// actual signaling).
///
/// The DataflowController is implemented as follows. The consumer
/// tells the producer it is about to block using the
/// DataflowController::consumerToBlock flag in the shared memory.
/// The producer(s) check the flag when they produce more data, and
/// signal the consumer to wake it up if the flag is set.
/// 
/// Note that communication over shared memory is not instantaneous,
/// moreover, multiprocessors do not even implement sequential
/// consistency - thus there is no global order of all shared memory
/// operatioins, as observed by individual processors and processor
/// cores.  In the following we use the message-passing model to
/// explain the interaction between the producer and the consumers:
/// producers use the queues to send messages with data to the
/// consumer, and the consumer uses the consumerToBlock flag in order
/// to notify the producer(s) about its status.
/// 
/// When the consumer is about to block, the liveness-criticial case
/// is when the producer is also about to block around the same
/// time. Some data can arrive to the consumer between the last queue
/// check and thread blocking, which must be taken care of by the
/// producer - but if the producer blocks on its own too, then that
/// last data would remain in the queue, unprocessed. There are two
/// different scenario: either (a) a producer learns the consumer is
/// about to block - then we make sure the producer wakes up the
/// consumer, or (b) all producers block before receiving the
/// "consumer is to block" message (i.e. all producers observed the
/// consumerToBlock flag to be false) - then we must make sure the
/// consumer receives all data before it blocks.
///
/// The first case is implemented by a lock protecting two critical
/// sections: the consumer setting the consumerToBlock flag and
/// blocking on a conditional variable, and the producer taking the
/// same lock before waking up the consumer by means of a signal on
/// the conditional variable. Thus, by the time a producer can send a
/// signal the consumer is blocked, thus, the signal will not be lost
/// - and will wake up the consumer.
/// 
/// The second case is ensured by an additional, last queue check
/// after setting the consumerToBlock flag, together with strict
/// ordering of memory operations on the flag with respect to
/// operations on the data queues, as follows. First of all, the
/// producer(s) issue write-to-read (WR) memory barrier(s) between
/// enqueuing data and reading the consumerToBlock flag, i.e. all
/// queue data is forced to memory before the consumerToBlock flag is
/// read from it.  Similarly, the consumer issues a write-to-read
/// memory barrier between setting the flag and performing the last
/// queue check. Now, let us assume the consumer's last queue check
/// misses some data enqueued by a producer before the latter observed
/// the consumerToBlock flag to be (still) false. In this case, the
/// memory operations are ordered as follows:
///
///    queue write = data
///       --(WR barrier)--> 
///    flag read = false
///       --(memory coherence)-->
///    flag set = true
///       --(WR barrier)-->
///    queue read = (empty)
///
/// At the same time, we have:
///
///    queue read = (empty)
///       --(memory coherence)-->
///    queue write = data
///    
/// which together form a cyclic dependency on memory operations.
/// Hence, our assumption about the consumer observing the empty queue
/// is false.
///
/// Note also the checkConsumer() method's implementation is probably
/// suboptimal: in the case the producer is running (i.e. will not
/// block immediately after checking the consumer's status), we could
/// afford effectively wasting a signal which can happen when
/// pthread_cond_signal() is used without taking the mutex first
/// (specifically, a signal has no effect" when pthread_cond_signal()
/// invocation happens before the corresponding pthread_cond_wait()).
/// If a signal is wasted, the consumer would stil be waken up by a
/// next checkConsumer() invocation. However, the relative costs of
/// pthread_mutex_lock() and pthread_cond_signal() are unknown (so it
/// is unclear how much we'd gain should we omit locking), as well as
/// we'd need a scheme to avoid multiple pthread_cond_signal()
/// invocations by subsequent checkConsumer() calls, as long as
/// consumerToBlock is still true.
class DataflowController {
public:
  DataflowController();
  ~DataflowController();

  /// is used by the consumer to declare it run out of work and would
  /// need to block;
  void waitingForData() {
    pthread_mutex_lock(&sleepM);
    consumerToBlock = true;
    StoreLoad_BARRIER();
  }

  /// if there is still no data, the consumer thread blocks (waiting
  /// for data) using pthread_cond_wait().
  void waitForData() {
    assert(consumerToBlock);
    pthread_cond_wait(&sleepC, &sleepM);
  }

  /// is used by the consumer when it is waken up - and resumes
  /// processing the queue data;
  void resumeWithData() { 
    assert(consumerToBlock);
    consumerToBlock = false;
    pthread_mutex_unlock(&sleepM);
  }

  /// is used by the producer to check the consumer's status, and try
  /// to wakeup it up if needed, in the "usual" case when the producer
  /// keeps running and enqueuing more data for the consumer. If the
  /// consumerToBlock flag is discoverd to be true, then the consumer
  /// is waken up, and in the highly unlikely event it (consumer)
  /// still misses data in the queue (because of the store buffer
  /// memory reordering at the producer thread), it will be blocked -
  /// and signaled again. If the flag is discovered to be false, then
  /// no synchronization with the consumer is needed anyway - and we
  /// do not want to interrupt the producer's operations by a memory
  /// barrier.
  /// 
  /// The method returns a boolean indicating whether a wakeup signal
  /// could not be delivered - and, thus, it is "pending" and should
  /// be retried again.
  bool checkConsumer() { 
    if (consumerToBlock) {
      if (!pthread_mutex_trylock(&sleepM)) {
	pthread_cond_signal(&sleepC);
	pthread_mutex_unlock(&sleepM);
	return (false);		// successfully waken up the consumer;
      } else {
	return (true);		// this consumer still needs a wakeup;
      }
    } else {
      return (false);		// nothing to do;
    }
  } 

  /// is used by the producer to check the consumer's status in the
  /// case when the producer is about the block itself, in which case
  /// we must also ensure no data gets "stuck" in the queue;
  void checkConsumerSync() {
    // the store barrier makes sure that if the subsequent read
    // operation on consumerToBlock wins the race with the consumer
    // setting consumerToBlock to true (thus, false is returned), then
    // all data stores here, at the producer, precede the consumer's
    // final data check that follows the store barrier in
    // waitingForData(). Note mutex does not need to be taken before
    // the producer discovers consumerToBlock is set - thus no further
    // overhead (other than the store barrier) is paid when the
    // consumer needs no wakeup signal.
    StoreLoad_BARRIER();
    if (consumerToBlock) {
      pthread_mutex_lock(&sleepM);
      pthread_cond_signal(&sleepC);
      pthread_mutex_unlock(&sleepM);
    }
  }

private:
  pthread_cond_t sleepC;
  pthread_mutex_t sleepM;

  volatile bool consumerToBlock;
};

#endif // DATAFLOWCONTROLLER_H
