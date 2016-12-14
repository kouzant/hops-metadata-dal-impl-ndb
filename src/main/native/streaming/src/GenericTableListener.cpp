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
 * File:   GenericTableListener.cpp
 * Author: Mahmoud Ismail<maism@kth.se>, Konstantin Popov <kost@sics.se>
 *
 */

#include "GenericTableListener.h"

using namespace Utils;
using namespace Utils::NdbC;

//
ListenerWatchTable::ListenerWatchTable(const string mTableName,
				       const string* mColumnNames,
				       const int mNoColumns,
				       const NdbDictionary::Event::TableEvent* mWatchEvents,
				       const int mNoEvents,
				       const WatchTableColIndex pendingEventIdCol,
				       const WatchTableMsgField msgFields[],
				       const EventMsgType msgType)
  : mTableName(mTableName),
    mColumnNames(mColumnNames),
    mNoColumns(mNoColumns),
    mWatchEvents(mWatchEvents),
    mNoEvents(mNoEvents),
    pendingEventIdCol(pendingEventIdCol),
    msgFields(msgFields),
    msgType(msgType)
{}

template<class MsgPoolAlloc, class MsgPoolFree, class MsgHandler>
GenericTableListener::GenericTableListener(Ndb* ndb, 
					   MsgPoolAlloc<MsgType> *msgPool,
					   MsgHandler<MsgType, MsgPoolFree> * const msgHandler,
					   const ListenerWatchTable table, 
					   const unsigned long pollTimeout)
  : GenericListener(pollTimeout), 
    mNdbConnection(ndb),
    msgPool(msgPool),
    msgHandler(msgHandler),
    recAttr(new NdbRecAttr*[table.mNoColumns]),
    recAttrPre(new NdbRecAttr*[table.mNoColumns]),
    mEventName(concat("tail-", table.mTableName)), 
    mTable(table)
{
  LOG_INFO("create generic tabletailer" << mEventName);

  // "event creation" - where the notion of [ndb] "event" is some sort
  // of pattern of database modifications to watch for. The following
  // code replicates more or less verbatim the code example from MySQL
  // Cluster API Developer Guide - ndbapi_event.cpp
  LOG_INFO("create Event for [" << mEventName << "]");
  NdbDictionary::Dictionary *myDict = mNdbConnection->getDictionary();
  if (!myDict) LOG_NDB_API_ERROR(mNdbConnection->getNdbError());

  const NdbDictionary::Table *table = myDict->getTable(mTable.mTableName.c_str());
  if (!table) LOG_NDB_API_ERROR(myDict->getNdbError());

  NdbDictionary::Event myEvent(mEventName.c_str(), *table);

  for(int i=0; i< mTable.mNoEvents; i++){
    myEvent.addTableEvent(mTable.mWatchEvents[i]);
  }
  const char* columns[mTable.mNoColumns];
  for(int i=0; i< mTable.mNoColumns; i++){
    columns[i] = mTable.mColumnNames[i].c_str();
  }
  myEvent.addEventColumns(mTable.mNoColumns, columns);
  //myEvent.mergeEvents(merge_events);

  // Add event to database
  if (myDict->createEvent(myEvent) == 0)
    myEvent.print();		// kost@: what for? and where is that print() defined?
  else if (myDict->getNdbError().classification ==
	   NdbError::SchemaObjectExists) {
    LOG_ERROR("Event creation failed, event exists, dropping Event...");
    if (myDict->dropEvent(mEventName.c_str())) LOG_NDB_API_ERROR(myDict->getNdbError());
    // try again
    // Add event to database
    if (myDict->createEvent(myEvent)) LOG_NDB_API_ERROR(myDict->getNdbError());
  } else
    LOG_NDB_API_ERROR(myDict->getNdbError());
  
  // once the ndb "event" is creaated, we can create the
  // NdbEventOperation object and "execute it" which, apparently,
  // causes NDB to start producing update events:
  LOG_INFO("create EventOperation for [" << mEventName << "]");
  if ((ndbOp = mNdbConnection->createEventOperation(mEventName.c_str())) == NULL)
    LOG_NDB_API_ERROR(mNdbConnection->getNdbError());

  // primary keys should always be a part of the result
  for (int i = 0; i < mTable.mNoColumns; i++) {
    recAttr[i] = ndbOp->getValue(mTable.mColumnNames[i].c_str());
    recAttrPre[i] = ndbOp->getPreValue(mTable.mColumnNames[i].c_str());
  }

  LOG_INFO("Execute " << mEventName);
  // This starts changes to "start flowing"
  if (ndbOp->execute())
    LOG_NDB_API_ERROR(ndbOp->getNdbError());
}

template<class MsgPoolAlloc, class MsgPoolFree, class MsgHandler>
GenericTableListener::~GenericTableListener()
{
  LOG_INFO("delete generic tabletailer" << mEventName);

  // from the ndbapi_event.cpp example it looks like the "event
  // operation" has to be "dropped" first:
  mNdbConnection->dropEventOperation(ndbOp);

  NdbDictionary::Dictionary *myDict = mNdbConnection->getDictionary();
  if (!myDict) LOG_NDB_API_ERROR(mNdbConnection->getNdbError());
  // remove event from database
  if (myDict->dropEvent(mEventName.c_str())) LOG_NDB_API_ERROR(myDict->getNdbError());

  delete [] recAttr;
  delete [] recAttrPre;
  delete mNdbConnection;
}

template<class MsgPoolAlloc, class MsgPoolFree, class MsgHandler>
void GenericTableListener::handleEvents(unsigned long timeout)
{
  unsigned accNEvents = 0;
  for (unsigned int iter = 0; iter < pollEventsIterations; iter++) {
    int r = mNdbConnection->pollEvents2(timeout);
    if (r > 0) {
      NdbEventOperation* op;
      while ((op = mNdbConnection->nextEvent2())) {
	accNEvents++;
	NdbDictionary::Event::TableEvent event = op->getEventType2();
	if (event != NdbDictionary::Event::TE_EMPTY) {
	  LOG_TRACE(mEventName << " Got Event [" << event << ","  << 
		    getEventName(event) << "] Epoch " << op->getEpoch());
	}

	switch (event) {
	case NdbDictionary::Event::TE_INSERT:
	case NdbDictionary::Event::TE_DELETE:
	case NdbDictionary::Event::TE_UPDATE: {
	  // handle the event - given the "watch table" description.
	  // First create the event message object, and "handle" it -
	  // which can be either synchronous (i.e. "right away") or
	  // asynchronous (send the message object to another "message
	  // handling" thread);
	  EventMsg * const msg = msgPool->getMsgObject();

	  //
	  PendingEventID const pendingEventId = 
	    recAttr[mTable.pendingEventIdCol]->int32_value();
	  msg->reinit(mTable.msgType, pendingEventId);

	  //
	  for (int col = 0; col < mTable._pendingEvents_noCols; col++) {
	    WatchTableMsgFieldType const colType = mTable.msgFields[col].type;
	    WatchTableMsgFieldIndex const msgIndex = mTable.msgFields[col].msgIdx;
	    
	    switch (colType) {
	    case WatchTable_Int32Field:
	      int32_t const i32 = recAttr[col]->int32_value();
	      msg->setInt32Value(msgIndex, i32);
	      break;

	    case WatchTable_StringField:
	      char * const cstr = get_cstring(recAttr[col], msg);
	      msg->setStringValue(msgIndex, cstr);
	      break;

	    case WatchTable_EXCLUDED:
	      break;
	    }
	  }

	  msgHandler->handleMsg(msg);
	  break;
	}

	default:
	  break;
	}
      }
    }
  }
  return (accNEvents);
}

template<class MsgPoolAlloc, class MsgPoolFree, class MsgHandler>
const char* GenericTableListener::getEventName(NdbDictionary::Event::TableEvent event)
{
  switch (event) {
  case NdbDictionary::Event::TE_INSERT:
    return "INSERT";
  case NdbDictionary::Event::TE_DELETE:
    return "DELETE";
  case NdbDictionary::Event::TE_UPDATE:
    return "UPDATE";
  case NdbDictionary::Event::TE_DROP:
    return "DROP";
  case NdbDictionary::Event::TE_ALTER:
    return "ALTER";
  case NdbDictionary::Event::TE_CREATE:
    return "CREATE";
  case NdbDictionary::Event::TE_GCP_COMPLETE:
    return "GCP_COMPLETE";
  case NdbDictionary::Event::TE_CLUSTER_FAILURE:
    return "CLUSTER_FAILURE";
  case NdbDictionary::Event::TE_STOP:
    return "STOP";
  case NdbDictionary::Event::TE_NODE_FAILURE:
    return "NODE_FAILURE";
  case NdbDictionary::Event::TE_SUBSCRIBE:
    return "SUBSCRIBE";
  case NdbDictionary::Event::TE_UNSUBSCRIBE:
    return "UNSUBSCRIBE";    
  case NdbDictionary::Event::TE_EMPTY:
    return "EMPTY";
  case NdbDictionary::Event::TE_INCONSISTENT:
    return "INCONSISTENT";
  case NdbDictionary::Event::TE_OUT_OF_MEMORY:
    return "OUT_OF_MEMORY";
  case NdbDictionary::Event::TE_ALL:
    return "ALL";      
  }
  return "UNKOWN";
}

