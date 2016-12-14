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
 * File:   GenericMsgHandler.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef GENERICMSGHANDLER_H
#define GENERICMSGHANDLER_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "GenericMsg.h"

/// GenericMsgHandler is the generic interface to a "message
/// processing service". This allows different implementations:
/// - handling messages in the same "listener" thread (a
///   "single-thread" event processing implementation)
/// - handling messages asynchronously in another thread: instead of
///   handling messages immediately, they are queued and processed by
///   that other thread.
/// - handling messages by a pool of threads: instead of one single
///   queue and corresponding processing thread, there is a number of
///   queues and threads; messages are multiplixed over those using
///   the GenericMsg<>::hashKey() method.
/// Note there can be also multiple listener threads - e.g. processing
/// different input streams - that can do either of the above.
/// 
/// Note that the message handler object is in general stateful.
template<class MsgType, template<class TemplateMsgType> class MsgPool>
class GenericMsgHandler {
public:
  GenericMsgHandler() {}
  ~GenericMsgHandler() {}

  /// message handling method "per se";
  void handleMsg(MsgType *msg) { assert(0); }

  /// .. just reclaim the message object, without processing it -
  /// i.e. discard it but reuse the memory. This is useful for
  /// handling termination, as well as special treatment of message
  /// objects sent over GenericMsgP2PQueue, see GenericMsgP2PQueue.h
  /// and GenericMsgHandlingThread.cpp for details;
  void reclaimMsgObj(MsgType *msg) { assert(0); }
};

#endif // GENERICMSGHANDLER_H
