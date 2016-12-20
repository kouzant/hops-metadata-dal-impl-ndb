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

#ifndef UPDATEDCONTAINERINFOTABLELISTENER_H
#define UPDATEDCONTAINERINFOTABLELISTENER_H

/* 
 * File:   UpdatedContainerInfoTableListener.h
 * Author: ???, Konstantin Popov <kost@sics.se>
 *
 */

#include "common.h"
#include "GenericTableListener.h"
#include "EventMsg.h"

/**
 * @addtogroup AsyncUpdateEventHandling
 *
 * @{
 */

/// the "ndb event definition" tables are inherited/reused from
/// UpdatedContainerInfoTableTailer.h. See PendingEventTableListener.h
/// for further comments;
extern const string _updatedContainerInfo_table;
extern const int _updatedContainerInfo_noCols;
extern const string _updatedContainerInfo_cols[];
extern const int _updatedContainerInfo_noEvents; 
extern const NdbDictionary::Event::TableEvent _updatedContainerInfo_events[];

/// indices of the ndb event columns - must adhere to the definition
/// of _updatedContainerInfo_cols[] in UpdatedContainerInfoTable.cpp.
typedef enum {
  UCI_Col_RMNODE_ID = 0,
  UCI_Col_CONTAINER_ID = 1,
  UCI_Col_UPDATED_CONTAINER_INFO_ID = 2,
  UCI_Col_PENDING_EVENT_ID = 3
} UpdatedContainerInfoTableColIndex;

typedef enum {
  UCI_Msg_RMNODE_ID = 0,
  UCI_Msg_CONTAINER_ID = 1,
  UCI_Msg_UPDATED_CONTAINER_INFO_ID = 2,
} UpdatedContainerInfoTableMsgIndex;

extern const WatchTableColIndex _updatedContainerInfo_peCol;

extern const WatchTableMsgField _updatedContainerInfo_msgFields[];

extern const ListenerWatchTable updatedContainerInfoTable;

extern const unsigned int _updatedContainerInfo_timeout;

#endif /* UPDATEDCONTAINERINFOTABLELISTENER_H */
