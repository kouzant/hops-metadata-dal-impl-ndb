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

#ifndef PENDINGEVENTTABLELISTENER_H
#define PENDINGEVENTTABLELISTENER_H

/* 
 * File:   GenericTableListener.h
 * Author: ???, Konstantin Popov <kost@sics.se>
 *
 */

#include "GenericTableListener.h"
#include <EventMsg.h>

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

/// Re-implementation of the PendingEventTableTailer, using the
/// "generic listener" framework (see GenericTableListener.h).
/// 
/// In fact, there is no such thing as PendingEventTableListener
/// class: specific table listeners are implemented by
/// GenericTableListener, instantiated with a proper
/// ListenerWatchTable - which is defined below.

/// the "ndb event definition" tables are inherited/reused from
/// PendingEventTableTailer.h:
extern const string _pendingEvents_table;
extern const int _pendingEvents_noCols;
extern const string _pendingEvents_cols[];
extern const int _pendingEvents_noEvents; 
extern const NdbDictionary::Event::TableEvent _pendingEvents_events[];

/// indices of the ndb event columns - must adhere to the definition
/// of _pendingEvents_cols[] in PendingEventTableTailer.cpp. These
/// values are also indices into pendingEventMsgFields below;
typedef enum {
  PE_Col_ID = 0,
  PE_Col_RMNODE_ID = 1,
  PE_Col_TYPE = 2,
  PE_Col_STATUS = 3,
  PE_Col_CONTAINS = 4
} PendingEventTableColIndex;

/// indices of EventMsg fields - for the MSG_PendingEventTable event
/// type.  Should be assigned sequentially from 0, and must be less
/// than EventMsg::MAX_NUM_COLS;
typedef enum {
  PE_Msg_RMNODE_ID = 0,
  PE_Msg_TYPE = 1,
  PE_Msg_STATUS = 2,
  PE_Msg_CONTAINS = 3
} PendingEventTableMsgIndex;

/// location (index) of the pending event column in the ndb event;
extern const PendingEventField _pendingEvents_peCol;

/// mapping from ndb event columns to event [representation] type and
/// EventMsg' field index;
extern const WatchTableMsgField _pendingEvents_msgFields[];

/// .. resulting complete "pending event" description table:
extern const ListenerWatchTable pendingEventTable;

// polling timeout;
extern const unsigned int _pendingEvents_timeout;

#endif /* PENDINGEVENTTABLELISTENER_H */
