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

template<template<class TemplateMsgType> class MsgPool>
EventMsgHandler::EventMsgHandler(JavaVM* jvm,
				 MsgPool<EventMsg> *msgPool,
				 unsigned int msgAccSizeBits,
				 unsigned int keyReduce)
  : jvm(jvm), msgPool(msgPool), msgAcc(msgAccSizeBits, keyReduce)
{}

template<template<class TemplateMsgType> class MsgPool>
EventMsgHandler::handleMsg(EventMsg *msg)
{
  PendingEventID const peID = msg->getPendingEventID();
  EventMsgType const msgType = msg->getType();
  //
  EventMsgContainer * const container = msgAcc.lookup(peID);
  //
  switch (msgType) {
  case MSG_PendingEventTable: 
    int32_t const pet_contains = 
      msg->getInt32Value((WatchTableMsgFieldIndex) PE_Msg_CONTAINS);
    // pet_contains is the number of messages for each
    // MSG_UpdatedContainerInfoTableTailer and
    // MSG_ContainerStatusTableTailer types, and then there must be 3
    // more messages of remaining "obligatory" types;
    container->setNEntries(pet_contains * 2 + 3);
    // fall through;
  case MSG_RMNodeTable:
  case MSG_ResourceTableTailer:
    // message of all of the above 3 types appear in the container
    // exactly once:
    container->setMsg(msgType, msg);
    break;
    
  case MSG_UpdatedContainerInfoTableTailer:
  case MSG_ContainerStatusTableTailer:
    // 0 or more entries for these types:
    container->addMsg(msgType, msg);
    break;
  }

  //
  if (container->isComplete()) {
    // PendingEventTable
    EventMsg * const petMsg = container->getMsg(MSG_PendingEventTable);
    char * const pet_rmnodeId = 
      petMsg->getStringValue((WatchTableMsgFieldIndex) PE_Msg_RMNODE_ID);
    char * const pet_type = 
      petMsg->getStringValue((WatchTableMsgFieldIndex) PE_Msg_TYPE);
    char * const pet_status =
      petMsg->getStringValue((WatchTableMsgFieldIndex) PE_Msg_STATUS);
    int32_t const pet_contains = 
      petMsg->getInt32Value((WatchTableMsgFieldIndex) PE_Msg_CONTAINS);

    // RMNodeTable
    EventMsg * const rmntMsg = container->getMsg(MSG_RMNodeTable);
    char * const rmnt_rmnodeId = 
      rmntMsg->getStringValue((WatchTableMsgFieldIndex) RMN_Msg_RMNODE_ID);
    char * const rmnt_hostName = 
      rmntMsg->getStringValue((WatchTableMsgFieldIndex) RMN_Msg_HOST_NAME);
    int32_t const rmnt_commandPort = 
      rmntMsg->getInt32Value((WatchTableMsgFieldIndex) RMN_Msg_COMMAND_PORT);
    int32_t const rmnt_httpPort = 
      rmntMsg->getInt32Value((WatchTableMsgFieldIndex) RMN_Msg_HTTP_PORT);
    char * const rmnt_healthReport =
      rmntMsg->getStringValue((WatchTableMsgFieldIndex) RMN_Msg_HEALTH_REPORT);
    int64_t const rmnt_lastHealthReportTime =
      rmntMsg->getInt64Value((WatchTableMsgFieldIndex) RMN_Col_LAST_HEALTH_REPORT_TIME);
    char * const rmnt_currentState =
      rmntMsg->getStringValue((WatchTableMsgFieldIndex) RMN_Msg_CURRENT_STATE);
    char * const rmnt_nodeManagerVersion = 
      rmntMsg->getStringValue((WatchTableMsgFieldIndex) RMN_Msg_NODEMANAGER_VERSION);

    // ResourceTable
    EventMsg * const rtMsg = container->getMsg(MSG_ResourceTableTailer);
    char * const rt_id = 
      rtMsg->getStringValue((WatchTableMsgFieldIndex) Res_Msg_ID);
    int32_t const rt_memory = 
      rtMsg->getInt32Value((WatchTableMsgFieldIndex) Res_Msg_MEMORY);
    int32_t const rt_virtualcores =
      rtMsg->getInt32Value((WatchTableMsgFieldIndex) Res_Msg_VIRTUALCORES);

    // UpdatedContainerInfoTable
    EventMsg *ucitMsg = container->getMsg(MSG_UpdatedContainerInfoTableTailer);
    // there should be pet_contains messages of the type;
    while (ucitMsg) {
      char * const ucit_rmnodeId = 
	ucitMsg->getStringValue((WatchTableMsgFieldIndex) UCI_Msg_RMNODE_ID);
      char * const ucit_containerId = 
	ucitMsg->getStringValue((WatchTableMsgFieldIndex) UCI_Msg_CONTAINER_ID);
      int32_r const ucit_updatedContainerInfoId =
	ucitMsg->getInt32Value((WatchTableMsgFieldIndex) UCI_Msg_UPDATED_CONTAINER_INFO_ID);

      //
      ucitMsg = ucitMsg->getNextContainerMsg();
    }

    // ContainerStatusTable
    EventMsg *cstMsg = container->getMsg(MSG_ContainerStatusTableTailer);
    while (cstMsg) {
      char * const cst_containerId =
	cstMsg->getStringValue((WatchTableMsgFieldIndex) CS_Msg_CONTAINER_ID);
      char * const cst_rmnodeId =
	cstMsg->getStringValue((WatchTableMsgFieldIndex) CS_Msg_RMNODE_ID);
      char * const cst_type =
	cstMsg->getStringValue((WatchTableMsgFieldIndex) CS_Msg_TYPE);
      char * const cst_state =
	cstMsg->getStringValue((WatchTableMsgFieldIndex) CS_Msg_STATE);
      char * const cst_diagnostics =
	cstMsg->getStringValue((WatchTableMsgFieldIndex) CS_Msg_DIAGNOSTICS);
      int32_t const cst_exitStatus =
	cstMsg->getInt32Value((WatchTableMsgFieldIndex) CS_Msg_EXIT_STATUS);
      int32_t const cst_uciId =
	cstMsg->getInt32Value((WatchTableMsgFieldIndex) CS_Msg_UCIID);

      //
      cstMsg = cstMsg->getNextContainerMsg();
    }

    // TODO - make JNI call(s)..
    // note that memory for char* data "belongs" to event messages.

    // release event messages, following AsyncMsgBase message
    // reclamation protocol (see AsyncMsgBase.h, GenericMsgP2PQueue.h
    // for further details, and GenericMsgHandlingThread.cpp where
    // messages are declared safeToReclaim() when it is indeed safe).
    if (petMsg->safeToReclaim())
      reclaimMsgObj(petMsg);
    if (rmntMsg->safeToReclaim())
      reclaimMsgObj(rmntMsg);
    if (rtMsg->safeToReclaim())
      reclaimMsgObj(rtMsg);
    if (ucitMsg->safeToReclaim())
      reclaimMsgObj(ucitMsg);
    if (cstMsg->safeToReclaim())
      reclaimMsgObj(cstMsg);
    //
    msgAcc.remove(container);
  }
}

//
EventMsgAccumulator::EventMsgAccumulator(unsigned int sizeBits,
					 unsigned int keyReduce)
  : sizeBits(sizeBits),
    sizeMask((0x1 << sizeBits) - 1),
    keyReduce(keyReduce),
    table(new EventMsgContainer[0x1 << sizeBits]),
    freeList((EventMsgContainer *) NULL)
{}

EventMsgAccumulator::~EventMsgAccumulator()
{
  EventMsgContainer *msg;
  while (msg = freeList) {
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
  bool found = false;

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
