/*
 * Hops Database abstraction layer for storing the hops metadata in MySQL Cluster
 * Copyright (C) 2015  hops.io
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
package io.hops.metadata.ndb.dalimpl.yarn.rmstatestore;

import com.mysql.clusterj.annotation.Column;
import com.mysql.clusterj.annotation.PersistenceCapable;
import com.mysql.clusterj.annotation.PrimaryKey;
import io.hops.exception.StorageException;
import io.hops.metadata.ndb.ClusterjConnector;
import io.hops.metadata.ndb.wrapper.HopsQuery;
import io.hops.metadata.ndb.wrapper.HopsQueryBuilder;
import io.hops.metadata.ndb.wrapper.HopsQueryDomainType;
import io.hops.metadata.ndb.wrapper.HopsSession;
import io.hops.metadata.yarn.TablesDef;
import io.hops.metadata.yarn.dal.rmstatestore.HeartBeatRPCDataAccess;
import io.hops.metadata.yarn.entity.appmasterrpc.HeartBeatRPC;
import io.hops.metadata.yarn.entity.appmasterrpc.ToRemoveHBContainerStatus;
import io.hops.metadata.yarn.entity.appmasterrpc.ToRemoveHBKeepAliveApp;
import io.hops.metadata.yarn.entity.appmasterrpc.ToRemoveRPC;

import javax.annotation.OverridingMethodsMustInvokeSuper;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class HeartBeatRPCClusterJ implements TablesDef.HeartBeatRPCTableDef,
        TablesDef.HeartBeatContainerStatusesTableDef,
        TablesDef.HeartBeatKeepAliveApplications,
        HeartBeatRPCDataAccess<HeartBeatRPC> {

  @PersistenceCapable(table = TablesDef.HeartBeatRPCTableDef.TABLE_NAME)
  public interface HeartBeatRPCDTO {

    @PrimaryKey
    @Column(name = TablesDef.HeartBeatRPCTableDef.RPCID)
    int getrpcid();

    void setrpcid(int id);

    @Column(name = TablesDef.HeartBeatRPCTableDef.NODEID)
    String getNodeId();

    void setNodeId(String nodeId);

    @Column(name = TablesDef.HeartBeatRPCTableDef.RESPONSEID)
    int getResponseId();

    void setResponseId(int responseId);

    @Column(name = TablesDef.HeartBeatRPCTableDef.NODE_HEALTH_STATUS)
    byte[] getNodeHealthStatus();

    void setnodeHealthStatus(byte[] nodeHealthStatus);

    @Column(name = TablesDef.HeartBeatRPCTableDef.LAST_CONTAINER_TOKEN_KEY)
    byte[] getLastContainerTokenKey();

    void setLastContainerTokenKey(byte[] lastContainerTokenKey);

    @Column(name = TablesDef.HeartBeatRPCTableDef.LAST_NM_KEY)
    byte[] getLastNMKey();

    void setLastNMKey(byte[] lastNMKey);
  }

  @PersistenceCapable(table
          = TablesDef.HeartBeatContainerStatusesTableDef.TABLE_NAME)
  public interface HeartBeatContainerStatusDTO {

    @PrimaryKey
    @Column(name = TablesDef.HeartBeatContainerStatusesTableDef.RPCID)
    int getrpcid();

    void setrpcid(int id);

    @PrimaryKey
    @Column(name = TablesDef.HeartBeatContainerStatusesTableDef.CONTAINERID)
    String getContainerId();

    void setContainerId(String id);

    @Column(name = TablesDef.HeartBeatContainerStatusesTableDef.STATUS)
    byte[] getStatus();

    void setStatus(byte[] status);
  }

  @PersistenceCapable(table
          = TablesDef.HeartBeatKeepAliveApplications.TABLE_NAME)
  public interface HeartBeatKeepAliveApplicationDTO {

    @PrimaryKey
    @Column(name = TablesDef.HeartBeatKeepAliveApplications.RPCID)
    int getrpcid();

    void setrpcid(int id);

    @PrimaryKey
    @Column(name = TablesDef.HeartBeatKeepAliveApplications.APPID)
    String getAppId();

    void setAppId(String nodeId);
  }

  private final ClusterjConnector connector = ClusterjConnector.getInstance();

  @Override
  public void add(HeartBeatRPC toAdd)
          throws StorageException {
    HopsSession session = connector.obtainSession();
    HeartBeatRPCDTO hbDTO = createPersistableHeartBeatRPC(toAdd, session);
    List<HeartBeatContainerStatusDTO> containerStatusDTOs
            = new ArrayList<HeartBeatContainerStatusDTO>();
    for (String containerId : toAdd.getContainersStatuses().keySet()) {
      containerStatusDTOs.add(createPersistableContainerStatus(containerId,
              toAdd.getContainersStatuses().get(containerId), toAdd.
              getRpcId(), session));
    }
    List<HeartBeatKeepAliveApplicationDTO> keepAliveDTOs
            = new ArrayList<HeartBeatKeepAliveApplicationDTO>();
    for (String appId : toAdd.getKeepAliveApplications()) {
      keepAliveDTOs.add(createKeepAliveDTO(appId, toAdd.getRpcId(), session));
    }

    session.savePersistent(hbDTO);
    session.savePersistentAll(containerStatusDTOs);
    session.savePersistentAll(keepAliveDTOs);
    session.release(hbDTO);
    session.release(containerStatusDTOs);
    session.release(keepAliveDTOs);
  }

  public void removeAll(List<ToRemoveRPC> hbRPCsToRemove,
          List<ToRemoveHBContainerStatus> hbContStatusesToRemove,
          List<ToRemoveHBKeepAliveApp> hbKeepAliveAppsToRemove)
    throws StorageException {

    if (hbRPCsToRemove.isEmpty()
            && hbContStatusesToRemove.isEmpty()
            && hbKeepAliveAppsToRemove.isEmpty()) {
      return;
    }

    HopsSession session = connector.obtainSession();

    List<HeartBeatRPCDTO> hbToRemove = createRemovableHBRPCs(hbRPCsToRemove, session);
    List<HeartBeatContainerStatusDTO> hbContStat =
            createRemovableHBContainerStatuses(hbContStatusesToRemove, session);
    List<HeartBeatKeepAliveApplicationDTO> hbKeepAlive =
            createRemovableHBKeepAliveApps(hbKeepAliveAppsToRemove, session);

    if (hbToRemove != null) {
      session.deletePersistentAll(hbToRemove);
      session.release(hbToRemove);
    }

    if (hbContStat != null) {
      session.deletePersistentAll(hbContStat);
      session.release(hbContStat);
    }

    if (hbKeepAlive != null) {
      session.deletePersistentAll(hbKeepAlive);
      session.release(hbKeepAlive);
    }
  }

  private List<HeartBeatRPCDTO> createRemovableHBRPCs(List<ToRemoveRPC> toRemove, HopsSession session)
          throws StorageException {

    if (toRemove.isEmpty()) {
      return null;
    }

    List<HeartBeatRPCDTO> hbToRemove = new ArrayList<HeartBeatRPCDTO>(toRemove.size());
    for (ToRemoveRPC rpc : toRemove) {
      HeartBeatRPCDTO hbDTO = session.newInstance(HeartBeatRPCDTO.class, rpc.getRpcId());
      hbToRemove.add(hbDTO);
    }

    return hbToRemove;
  }

  private List<HeartBeatContainerStatusDTO> createRemovableHBContainerStatuses(
          List<ToRemoveHBContainerStatus> toRemove, HopsSession session)
    throws StorageException {

    if (toRemove.isEmpty()) {
      return null;
    }

    List<HeartBeatContainerStatusDTO> hbToRemove =
            new ArrayList<HeartBeatContainerStatusDTO>(toRemove.size());
    for (ToRemoveHBContainerStatus rpc : toRemove) {
      Object[] dtoParam = new Object[2];
      dtoParam[0] = rpc.getRpcId();
      dtoParam[1] = rpc.getContainerId();

      HeartBeatContainerStatusDTO hbDTO = session
              .newInstance(HeartBeatContainerStatusDTO.class, dtoParam);
      hbToRemove.add(hbDTO);
    }

    return hbToRemove;
  }

  private List<HeartBeatKeepAliveApplicationDTO> createRemovableHBKeepAliveApps(
          List<ToRemoveHBKeepAliveApp> toRemove, HopsSession session)
    throws StorageException {

    if(toRemove.isEmpty()) {
      return null;
    }

    List<HeartBeatKeepAliveApplicationDTO> hbToRemove =
            new ArrayList<HeartBeatKeepAliveApplicationDTO>(toRemove.size());
    for (ToRemoveHBKeepAliveApp rpc : toRemove) {
      Object[] dtoParam = new Object[2];
      dtoParam[0] = rpc.getRpcId();
      dtoParam[1] = rpc.getAppId();

      HeartBeatKeepAliveApplicationDTO hbDTO = session
              .newInstance(HeartBeatKeepAliveApplicationDTO.class, dtoParam);
      hbToRemove.add(hbDTO);
    }

    return hbToRemove;
  }

  @Override
  public Map<Integer, HeartBeatRPC> getAll() throws
          StorageException {

    HopsSession session = connector.obtainSession();
    HopsQueryBuilder qb = session.getQueryBuilder();

    HopsQueryDomainType<HeartBeatRPCDTO> hbDobj = qb.createQueryDefinition(
            HeartBeatRPCDTO.class);
    HopsQuery<HeartBeatRPCDTO> hbQuery = session.createQuery(hbDobj);
    List<HeartBeatRPCDTO> hbQueryResults = hbQuery.getResultList();

    HopsQueryDomainType<HeartBeatKeepAliveApplicationDTO> hbKeepAliveDobj = qb.
            createQueryDefinition(HeartBeatKeepAliveApplicationDTO.class);
    HopsQuery<HeartBeatKeepAliveApplicationDTO> hbKeepAliveQuery = session.
            createQuery(hbKeepAliveDobj);
    List<HeartBeatKeepAliveApplicationDTO> hbKeepAliveQueryResults
            = hbKeepAliveQuery.getResultList();

    HopsQueryDomainType<HeartBeatContainerStatusDTO> hbContainerStatusDobj = qb.
            createQueryDefinition(HeartBeatContainerStatusDTO.class);
    HopsQuery<HeartBeatContainerStatusDTO> hbContainerStatusQuery = session.
            createQuery(hbContainerStatusDobj);
    List<HeartBeatContainerStatusDTO> hbContainerStatusQueryResults
            = hbContainerStatusQuery.getResultList();

    Map<Integer, HeartBeatRPC> result = createHeartBeatRPCsMap(hbQueryResults,
            hbKeepAliveQueryResults, hbContainerStatusQueryResults);

    session.release(hbQueryResults);
    session.release(hbKeepAliveQueryResults);
    session.release(hbContainerStatusQueryResults);

    return result;
  }

  private HeartBeatRPCDTO createPersistableHeartBeatRPC(HeartBeatRPC hop,
          HopsSession session) throws StorageException {
    HeartBeatRPCDTO heartBeatRPCDTO = session.newInstance(HeartBeatRPCDTO.class);
    heartBeatRPCDTO.setrpcid(hop.getRpcId());
    heartBeatRPCDTO.setLastContainerTokenKey(hop.
            getLastKnownContainerTokenMasterKey());
    heartBeatRPCDTO.setLastNMKey(hop.getLastKnownNMTokenMasterKey());
    heartBeatRPCDTO.setNodeId(hop.getNodeId());
    heartBeatRPCDTO.setResponseId(hop.getResponseId());
    heartBeatRPCDTO.setnodeHealthStatus(hop.getNodeHealthStatus());

    return heartBeatRPCDTO;
  }

  private HeartBeatContainerStatusDTO createPersistableContainerStatus(
          String containerId, byte[] status,
          int rpcId, HopsSession session) throws StorageException {
    HeartBeatContainerStatusDTO dto = session.newInstance(
            HeartBeatContainerStatusDTO.class);
    dto.setStatus(status);
    dto.setrpcid(rpcId);
    dto.setContainerId(containerId);
    return dto;
  }

  private HeartBeatKeepAliveApplicationDTO createKeepAliveDTO(String appId,
          int rpcId, HopsSession session) throws StorageException {
    HeartBeatKeepAliveApplicationDTO dto = session.newInstance(
            HeartBeatKeepAliveApplicationDTO.class);
    dto.setAppId(appId);
    dto.setrpcid(rpcId);
    return dto;
  }

  private Map<Integer, HeartBeatRPC> createHeartBeatRPCsMap(
          List<HeartBeatRPCDTO> hbQueryResults,
          List<HeartBeatKeepAliveApplicationDTO> hbKeepAliveQueryResults,
          List<HeartBeatContainerStatusDTO> hbContainerStatusQueryResults) {

    Map<Integer, Map<String, byte[]>> containersStatuses
            = new HashMap<Integer, Map<String, byte[]>>();
    for (HeartBeatContainerStatusDTO dto : hbContainerStatusQueryResults) {
      Map<String, byte[]> containersStatusesRPC = containersStatuses.get(dto.
              getrpcid());
      if (containersStatusesRPC == null) {
        containersStatusesRPC = new HashMap<String, byte[]>();
        containersStatuses.put(dto.getrpcid(), containersStatusesRPC);
      }
      containersStatusesRPC.put(dto.getContainerId(), dto.getStatus());
    }

    Map<Integer, List<String>> keepAliveApps
            = new HashMap<Integer, List<String>>();
    for (HeartBeatKeepAliveApplicationDTO dto : hbKeepAliveQueryResults) {
      List<String> appIdsRPC = keepAliveApps.get(dto.getrpcid());
      if (appIdsRPC == null) {
        appIdsRPC = new ArrayList<String>();
        keepAliveApps.put(dto.getrpcid(), appIdsRPC);
      }
      appIdsRPC.add(dto.getAppId());
    }

    Map<Integer, HeartBeatRPC> result = new HashMap<Integer, HeartBeatRPC>();
    for (HeartBeatRPCDTO dto : hbQueryResults) {
      HeartBeatRPC rpc = new HeartBeatRPC(dto.getNodeId(), dto.getResponseId(),
              containersStatuses.get(dto.getrpcid()), keepAliveApps.get(dto.
                      getrpcid()),
              dto.getNodeHealthStatus(), dto.getLastContainerTokenKey(),
              dto.getLastNMKey(), dto.getrpcid());
      result.put(dto.getrpcid(), rpc);
    }
    return result;
  }
}
