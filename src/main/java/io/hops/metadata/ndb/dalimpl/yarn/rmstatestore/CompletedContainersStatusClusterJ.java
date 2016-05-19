/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package io.hops.metadata.ndb.dalimpl.yarn.rmstatestore;

import com.mysql.clusterj.annotation.Column;
import com.mysql.clusterj.annotation.PersistenceCapable;
import com.mysql.clusterj.annotation.PrimaryKey;
import com.sun.org.apache.regexp.internal.RE;
import io.hops.exception.StorageException;
import io.hops.metadata.ndb.ClusterjConnector;
import io.hops.metadata.ndb.wrapper.HopsPredicate;
import io.hops.metadata.ndb.wrapper.HopsQuery;
import io.hops.metadata.ndb.wrapper.HopsQueryBuilder;
import io.hops.metadata.ndb.wrapper.HopsQueryDomainType;
import io.hops.metadata.ndb.wrapper.HopsSession;
import io.hops.metadata.yarn.TablesDef;
import io.hops.metadata.yarn.dal.rmstatestore.CompletedContainersStatusDataAccess;
import io.hops.metadata.yarn.entity.rmstatestore.AllocateResponse;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutionException;

import io.hops.metadata.yarn.entity.rmstatestore.GarbageCollectorAllocResp;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class CompletedContainersStatusClusterJ implements
        TablesDef.CompletedContainersStatusTableDef,
        CompletedContainersStatusDataAccess<AllocateResponse> {
public static final Log LOG = LogFactory.getLog(CompletedContainersStatusClusterJ.class);
  @PersistenceCapable(table = TABLE_NAME)
  public interface CompletedContainerDTO {

    @PrimaryKey
    @Column(name = APPLICATIONATTEMPTID)
    String getapplicationattemptid();

    void setapplicationattemptid(String applicationattemptid);

    @PrimaryKey
    @Column(name = CONTAINERID)
    String getcontainerid();

    void setcontainerid(String containerid);
    
    @PrimaryKey
    @Column(name = RESPONSEID)
    int getresponseid();
    
    void setresponseid(int id);
    
    @Column(name = STATUS)
    byte[] getStatus();
    
    void setStatus(byte[] status);
  }

  private final ClusterjConnector connector = ClusterjConnector.getInstance();

  
  public void update(Collection<AllocateResponse> entries) throws
          StorageException {
    HopsSession session = connector.obtainSession();
    List<CompletedContainerDTO> toPersist
            = new ArrayList<CompletedContainerDTO>();
    session.flush();
    for (AllocateResponse resp : entries) {
      //put new values
      toPersist.addAll(createPersistable(resp, session));
      //remove old values
      // They are removed async by the GC service
    /*HopsQueryBuilder qb = session.getQueryBuilder();
      HopsQueryDomainType<CompletedContainerDTO> dobj = qb.
              createQueryDefinition(CompletedContainerDTO.class);
      HopsPredicate pred1 = dobj.get(APPLICATIONATTEMPTID).equal(dobj.param(
              APPLICATIONATTEMPTID));
      dobj.where(pred1);
      HopsPredicate pred2 = dobj.get(RESPONSEID).equal(dobj.param(RESPONSEID));
      dobj.where(pred2);
      HopsQuery<CompletedContainerDTO> query = session.createQuery(dobj);
      query.setParameter(APPLICATIONATTEMPTID, resp.getApplicationattemptid());
      query.setParameter(RESPONSEID, resp.getResponseId()-1);
      query.deletePersistentAll();*/
    }
    session.savePersistentAll(toPersist);
    session.release(toPersist);
  }

  @Override
  public void removeGarbage(Collection<AllocateResponse> garbage) throws StorageException {
    if (garbage.isEmpty()) {
      return;
    }

    HopsSession session = connector.obtainSession();
    HopsQueryBuilder qb = session.getQueryBuilder();
    HopsQueryDomainType<CompletedContainerDTO> qdt = qb
            .createQueryDefinition(CompletedContainerDTO.class);
    HopsPredicate pred1 = qdt.get(APPLICATIONATTEMPTID).equal(qdt
            .param(APPLICATIONATTEMPTID));
    HopsPredicate pred2 = qdt.get(RESPONSEID).equal(qdt
            .param(RESPONSEID));
    qdt.where(pred1.and(pred2));
    HopsQuery<CompletedContainerDTO> query = session.createQuery(qdt);

    for (AllocateResponse resp : garbage) {
      query.setParameter(APPLICATIONATTEMPTID, resp.getApplicationattemptid());
      query.setParameter(RESPONSEID, resp.getResponseId());
      query.deletePersistentAll();
    }
  }

  @Override
  public void removeAll(Collection<AllocateResponse> entries) throws
          StorageException {
    if (entries.isEmpty()) {
      return;
    }

    HopsSession session = connector.obtainSession();
    List<CompletedContainerDTO> toRemove =
            new ArrayList<CompletedContainerDTO>();
    for (AllocateResponse hop : entries) {
      List<CompletedContainerDTO> pers =
              createPersistable(hop, session);
      if (!pers.isEmpty()) {
        toRemove.addAll(pers);
      }
    }

    if (!toRemove.isEmpty()) {
      session.deletePersistentAll(toRemove);
      session.release(toRemove);
    }
  }

  public Map<String, List<AllocateResponse>> getAll() throws StorageException {
    HopsSession session = connector.obtainSession();
    HopsQueryBuilder qb = session.getQueryBuilder();
    HopsQueryDomainType<CompletedContainerDTO> dobj = qb.createQueryDefinition(
            CompletedContainerDTO.class);
    HopsQuery<CompletedContainerDTO> query = session.createQuery(dobj);
    List<CompletedContainerDTO> queryResults = query.getResultList();
    Map<String, List<AllocateResponse>> result = createHopCompletedContainersMap(
            queryResults);
    session.release(queryResults);
    return result;
  }

  private List<CompletedContainerDTO> createPersistable(AllocateResponse hop,
          HopsSession session) throws StorageException {
    List<CompletedContainerDTO> result = new ArrayList<CompletedContainerDTO>();
    for (String containerId : hop.getCompletedContainersStatus().keySet()) {
      CompletedContainerDTO completedContainerDTO = session.newInstance(
              CompletedContainerDTO.class);
      completedContainerDTO.setapplicationattemptid(hop.
              getApplicationattemptid());
      completedContainerDTO.setcontainerid(containerId);
      completedContainerDTO.setresponseid(hop.getResponseId());
      completedContainerDTO.setStatus(hop.getCompletedContainersStatus().get(containerId));
      result.add(completedContainerDTO);
    }
    return result;
  }

  private Map<String, List<AllocateResponse>> createHopCompletedContainersMap(
          List<CompletedContainerDTO> list) throws StorageException {
    /*Map<String, List<byte[]>> allocatedContainersMap
            = new HashMap<String, List<byte[]>>();

    for (CompletedContainerDTO dto : list) {
      if (allocatedContainersMap.get(dto.getapplicationattemptid()) == null) {
        allocatedContainersMap.put(dto.getapplicationattemptid(),
                new ArrayList<byte[]>());
      }
      allocatedContainersMap.get(dto.getapplicationattemptid()).add(dto.
              getStatus());
    }
    return allocatedContainersMap;*/

    // <ApplicationAttemptID, <ResponseID, [completedContainerStatus]>>
    Map<String, Map<Integer, Map<String, byte[]>>> intermediateMap =
            new HashMap<String, Map<Integer, Map<String, byte[]>>>();

    Map<String, List<AllocateResponse>> completedContainersMap =
            new HashMap<String, List<AllocateResponse>>();

    for (CompletedContainerDTO dto : list) {
      String appAttID = dto.getapplicationattemptid();
      Integer responseID = dto.getresponseid();
      byte[] status = dto.getStatus();
      String containerID = dto.getcontainerid();

      if (intermediateMap.get(appAttID) == null) {
        intermediateMap.put(appAttID, new HashMap<Integer, Map<String, byte[]>>());
      }

      if (intermediateMap.get(appAttID).get(responseID) == null) {
        intermediateMap.get(appAttID).put(responseID, new HashMap<String, byte[]>());
      }

      intermediateMap.get(appAttID).get(responseID).put(containerID, status);
    }

    for (Map.Entry<String, Map<Integer, Map<String, byte[]>>> entry : intermediateMap.entrySet()) {
      List<AllocateResponse> responses = new ArrayList<AllocateResponse>(entry.getValue().size());

      for (Map.Entry<Integer, Map<String, byte[]>> resp : entry.getValue().entrySet()) {
        responses.add(AllocateResponse.newAllocRespComplStatusInstance(entry.getKey(), resp.getKey(), resp.getValue()));
      }

      completedContainersMap.put(entry.getKey(), responses);
    }

    return completedContainersMap;
  }
}
