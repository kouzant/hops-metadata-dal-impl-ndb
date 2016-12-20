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
 * File:   LocalMsgPool.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef LOCALMSGPOOL_H
#define LOCALMSGPOOL_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"
#include "GenericMsgPool.h"

/// The LocalMsgPool objects provide per-thread free stack of MsgType
/// objects. MsgType is expected to provide getNext() and setNext()
/// methods, as provided by e.g. PoolMsgBase
template<typename MsgType>
class LocalMsgPool : public GenericMsgPool<MsgType> {
public:
  LocalMsgPool() : top((MsgType *) NULL) {}
  ~LocalMsgPool();

  /// Allocate or re-use an MsgType object.
  MsgType* getMsgObject() {
    if (top != (MsgType *) NULL) {
      MsgType *obj = top;
      top = top->getNext();
      // note we do not needLocalMsgPool::top immediately thereafter,
      // thus this memory load, if missed, would not block subsequent
      // computation - but rather acts as a prefetch;
      obj->reinit();
      return (obj);
    } else {
      return (new MsgType());
    }
  }

  /// make it free;
  void freeMsgObject(MsgType *msgObj) {
    msgObj->setNext(top);
    top = msgObj;
  }

private:
  MsgType *top;		//!< the top of the free stack;
};

#include "LocalMsgPool.tcpp"

#endif // LOCALMSGPOOL_H
