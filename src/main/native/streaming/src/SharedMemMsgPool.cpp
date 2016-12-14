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
 * File:   SharedMemMsgPool.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

template<class MsgType>
SharedMemMsgPool::SharedMemMsgPool(unsigned int nQueues)
  : nRqs(nQueues),
    rqs(new GenericMsgP2PQueueHead<MsgType>[nQueues]),
    lastMsg(new MsgType*[nQueues]),
    lastCheckedRq(0)
{
  assert(nQueues > 0);
  assert(initMsg != (MsgType *) NULL);
  for (unsigned int i = 0; i < nQueues; i++)
    lastMsg[i] = new MsgType();
}

template<class MsgType>
SharedMemMsgPool::~SharedMemMsgPool()
{
  for (unsigned int i = 0; i < nQueues; i++) {
    MsgType *last = lastMsg[i];
    while (MsgType *msg = rqs[i].dequeue()) {
      delete last;
      last = msg;
    }
    delete last;
  }
  delete [] rqs;
  delete [] lastMsg;
}

template<class MsgType>
SharedMemMsgPool::getQueueHead(unsigned int n) const
{
  assert(n < nRqs);
  return (&rqs[n]);
}

template<class MsgType>
MsgType* SharedMemMsgPool::getMsgObject()
{
  MsgType *ret;
  if (nQueues > 1) {
    unsigned int i;
    MsgType *msg = (MsgType *) NULL;

    // start scanning from the next queue;
    lastCheckedRq = (lastCheckedRq + 1) % nQueues;
    for (i = 0; i < nQueues; i++) {
      msg = rqs[lastCheckedRq].dequeue();
      if (msg != (MsgType *) NULL)
	break;
      else
	lastCheckedRq = (lastCheckedRq + 1) % nQueues;
    }

    if (msg != (MsgType *) NULL) {
      // found one in the queue i;
      ret = lastMsg[i];
      lastMsg[i] = msg;
    } else {
      ret = new MsgType();
    }

    //
  } else {
    MsgType *msg = rqs[0].dequeue();
    if (msg != (MsgType *) NULL) {
      ret = lastMsg[0];
      lastMsg[0] = msg;
    } else {
      ret = new MsgType();
    }
  }

  return (ret);
}
