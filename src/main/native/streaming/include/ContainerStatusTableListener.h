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

#ifndef CONTAINERSTATUSTABLELISTENER_H
#define CONTAINERSTATUSTABLELISTENER_H

/* 
 * File:   ContainerStatusTableListener.h
 * Author: ???, Konstantin Popov <kost@sics.se>
 *
 */

#include "common.h"
#include "GenericTableListener.h"
#include "ContainerStatusTableTailer.h"

/// the "ndb event definition" tables are inherited/reused from
/// ContainerStatusTableTailer.h. See PendingEventTableListener.h for
/// further comments;
extern const string _containerStatus_table;
extern const int _containerStatus_noCols;
extern const string _containerStatus_cols[];
extern const int _containerStatus_noEvents; 
extern const NdbDictionary::Event::TableEvent _containerStatus_events[];

/// indices of the ndb event columns - must adhere to the definition
/// of _updatedContainerInfo_cols[] in UpdatedContainerInfoTable.cpp.
typedef enum {
  CS_Col_CONTAINER_ID = 0,
  CS_Col_RMNODE_ID = 1,
  CS_Col_TYPE = 2,
  CS_Col_STATE = 3,
  CS_Col_DIAGNOSTICS = 4,
  CS_Col_EXIT_STATUS = 5,
  CS_Col_UCIID = 6,
  CS_Col_PENDING_EVENT_ID = 7
} ContainerStatusTableColIndex;

typedef enum {
  CS_Msg_CONTAINER_ID = 0,
  CS_Msg_RMNODE_ID = 1,
  CS_Msg_TYPE = 2,
  CS_Msg_STATE = 3,
  CS_Msg_DIAGNOSTICS = 4,
  CS_Msg_EXIT_STATUS = 5,
  CS_Msg_UCIID = 6,
} ContainerStatusTableMsgIndex;

extern const WatchTableColIndex _containerStatus_peCol;

extern const WatchTableMsgField _containerStatus_msgFields[];

extern const ListenerWatchTable containerStatusTable;

extern const unsigned int _containerStatus_timeout;

#endif /* CONTAINERSTATUSTABLELISTENER_H */
