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
 * File:   GenericMsgP2PQueue.cpp
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#include "GenericMsgP2PQueue.h"

template<typename MsgType>
void initGenericMsgP2PQueue(MsgType *initMsg,
			    GenericMsgP2PQueueHead<MsgType> *head,
			    GenericMsgP2PQueueTail<MsgType> *tail)
{
  assert(initMsg != (MsgType *) NULL);
  assert(head->last == (MsgType *) NULL);
  assert(tail->last == (MsgType *) NULL);
  head->last = initMsg;
  tail->last = initMsg;
}

template<typename MsgType>
MsgType* flushMsgP2PQueue(GenericMsgP2PQueueHead<MsgType> *head,
			  GenericMsgP2PQueueTail<MsgType> *tail)
{
  return (tail->last);
}

