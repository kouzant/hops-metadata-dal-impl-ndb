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
 * File:   EventMsgP2PQueue.h
 * Author: Konstantin Popov <kost@sics.se>
 *
 */

#ifndef EVENTMSGP2PQUEUE_H
#define EVENTMSGP2PQUEUE_H

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

#include "GenericMsgP2PQueue.h"

/// Specific instance of GenericMsgP2PQueue class templates for
/// EventMsg message ojects;
typedef GenericMsgP2PQueueHead<EventMsg> EventMsgP2PQueueHead;
typedef GenericMsgP2PQueueTail<EventMsg> EventMsgP2PQueueTail;

//
void initEventMsgP2PQueue(EventMsg *initMsg,
			  EventMsgP2PQueueHead *head,
			  EventMsgP2PQueueTail *tail) {
  initGenericMsgP2PQueue(initMsg, head, tail);
}

#endif // EVENTMSGP2PQUEUE_H
