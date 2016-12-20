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
 * File:   PoolMsgBase.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef POOLMSGBASE_H
#define POOLMSGBASE_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "common.h"
#include "GenericMsg.h"

/// PoolMsgBase is the "message base class" for message objects that
/// are managed through a message pool.
template<typename TopClass>
class PoolMsgBase : public MsgHandlingBase<TopClass> {
public:
  /// The constructor should have been private - and allowed only for
  /// GenericMsgPool, but that's not possible in (current) C++.
  PoolMsgBase() : next((TopClass *) NULL) {}
  ~PoolMsgBase() {}

  void reinit() {		//!< equivalent to the constructor;
    next = (TopClass *) NULL;
  }

  /// getNext() and setNext() enable both queue- and stack- access;
  TopClass* getNext() const { return (next); }
  /// getNext() and setNext() enable both queue- and stack- access;
  void setNext(TopClass *n) {
    assert(next == (TopClass *) NULL);
    next = n;
  }

private:
  TopClass * volatile next;
};

#endif // POOLMSGBASE_H
