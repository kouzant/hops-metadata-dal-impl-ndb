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
 * File:   EventMsgHandler.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#include "EventMsgHandler.h"

//
EventMsgAccumulator::EventMsgAccumulator(unsigned int sizeBits,
					 unsigned int keyReduce)
  : table(new EventMsgContainer[0x1 << sizeBits]),
    sizeBits(sizeBits),
    sizeMask((0x1 << sizeBits) - 1),
    keyReduce(keyReduce),
    freeList((EventMsgContainer *) NULL)
{}

EventMsgAccumulator::~EventMsgAccumulator()
{
  EventMsgContainer *msg;
  while ((msg = freeList) != (EventMsgContainer *) NULL) {
    freeList = freeList->getNext();
    delete msg;
  }
  delete [] table;
}

EventMsgContainer* EventMsgAccumulator::getContainer(PendingEventID key)
{
  EventMsgContainer *c;
  if (freeList) {
    c = freeList;
    freeList = freeList->getNext();
  } else {
    c = new EventMsgContainer(key);
  }
  return (c);
}

void EventMsgAccumulator::freeContainer(EventMsgContainer *msg)
{
  // freeList keeps "dirty", i.e. not re-initialized containers;
  msg->setNext(freeList);
  freeList = msg;
}

EventMsgContainer* EventMsgAccumulator::lookup(PendingEventID key)
{
  unsigned int const index = getIndex(key);
  EventMsgContainer *c = &table[index];

  // the frequent case: check just the table entry itself:
  if (!c->isUsed()) {
    c->reinit(key);		// that's the new one;
    return (c);
  } else if (c->getKey() == key) {
    return (c);		// found it;
  }
  // else table[index] is occupied by something else:
  EventMsgContainer *prev = c;
  c = c->getNext();

  while (c) {
    if (c->getKey() == key)
      return (c);		// found it;
    prev = c;
    c = c->getNext();
  }
  // .. did not find it: make & link up a new one;

  c = getContainer(key);
  c->reinit(key);
  prev->setNext(c);
  return (c);
}

void EventMsgAccumulator::remove(PendingEventID key,
				 EventMsgContainer *container)
{
  unsigned int const index = getIndex(key);
  EventMsgContainer *c = &table[index];
  if (container == c) {
    // the simple (and usual) case: container is directly in the
    // table. The special subcase is, however, when there is a next
    // container chained up to msg - in which case we have to move it
    // to the table's slot:
    EventMsgContainer *next = c->getNext();
    if (next) {
      *c = *next;		// including the next field;
      // note that next became free:
      freeContainer(next);
    } else {
      c->reinit();
    }
  } else {
    // container was explicitly allocated - find it, unlink and deallocate;
    while (1) {
      EventMsgContainer *prev = c;
      c = c->getNext();
      assert(c);		// container must be present somewhere;
      if (c == container) {
	EventMsgContainer *next = c->getNext();
	prev->setNext(next);
	freeContainer(c);
      }
    }
  }
}
