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

#ifndef RESOURCETABLELISTENER_H
#define RESOURCETABLELISTENER_H

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

/// the "ndb event definition" tables are inherited/reused from
/// ResourceTableTailer.h. See PendingEventTableListener.h for further
/// comments;
extern const string _resource_table;
extern const int _resource_noCols;
extern const string _resource_cols[];
extern const int _resource_noEvents; 
extern const NdbDictionary::Event::TableEvent _resource_events[];

/// indices of the ndb event columns - must adhere to the definition
/// of _resource_cols[] in ResourceTableTailer.cpp. These values are
/// also indices into pendingEventMsgFields below;
typedef enum {
  Res_Col_ID = 0,
  Res_Col_MEMORY = 1,
  Res_Col_VIRTUALCORES = 2,
  Res_Col_PENDING_EVENT_ID = 3
} ResourceTableColIndex;

typedef enum {
  Res_Msg_ID = 0,
  Res_Msg_MEMORY = 1,
  Res_Msg_VIRTUALCORES = 2
} ResourceTableMsgIndex;

extern const PendingEventField _resource_peCol;

extern const WatchTableMsgField _resource_msgFields[];

extern const ListenerWatchTable resourceTable;

extern const unsigned int _resource_timeout;

#endif /* RESOURCETABLELISTENER_H */
