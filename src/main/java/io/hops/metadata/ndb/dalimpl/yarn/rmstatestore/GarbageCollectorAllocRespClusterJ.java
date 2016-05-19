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
import io.hops.metadata.yarn.dal.rmstatestore.GarbageCollectorAllocRespDataAccess;
import io.hops.metadata.yarn.entity.rmstatestore.GarbageCollectorAllocResp;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * Created by antonis on 5/16/16.
 */
public class GarbageCollectorAllocRespClusterJ implements TablesDef.GarbageCollectedAllocateResponsesTableDef,
        GarbageCollectorAllocRespDataAccess<GarbageCollectorAllocResp> {

    @PersistenceCapable(table = TABLE_NAME)
    public interface GCAllocRespDTO {
        @PrimaryKey
        @Column(name = APPLICATIONATTEMPTID)
        String getApplicationAttemptId();
        void setApplicationAttemptId(String applicationAttemptId);

        @PrimaryKey
        @Column(name = RESPONSEID)
        int getResponseId();
        void setResponseId(int responseId);

        @PrimaryKey
        @Column(name = TYPE)
        String getType();
        void setType(String type);
    }

    private final ClusterjConnector connector = ClusterjConnector.getInstance();

    @Override
    public void addAll(Collection<GarbageCollectorAllocResp> toPersist) throws StorageException {
        if (toPersist.isEmpty()) {
            return;
        }

        List<GCAllocRespDTO> persistable = new ArrayList<GCAllocRespDTO>(toPersist.size());
        HopsSession session = connector.obtainSession();
        for (GarbageCollectorAllocResp resp : toPersist) {
            persistable.add(createPersistable(resp, session));
        }

        session.savePersistentAll(persistable);
        session.release(persistable);
    }

    @Override
    public List<GarbageCollectorAllocResp> getSubset(int limit) throws StorageException {
        if (limit < 1) {
            return null;
        }

        HopsSession session = connector.obtainSession();
        HopsQueryBuilder qb = session.getQueryBuilder();
        HopsQueryDomainType<GCAllocRespDTO> qdt = qb
                .createQueryDefinition(GCAllocRespDTO.class);
        HopsQuery<GCAllocRespDTO> query = session.createQuery(qdt);
        query.setLimits(0, limit);
        List<GCAllocRespDTO> queryResultList = query.getResultList();

        List<GarbageCollectorAllocResp> resultlist = createHopsGCAllocResp(queryResultList);
        session.release(queryResultList);

        return resultlist;
    }

    public List<GarbageCollectorAllocResp> getAll() throws StorageException {
        HopsSession session = connector.obtainSession();
        HopsQueryBuilder qb = session.getQueryBuilder();
        HopsQueryDomainType<GCAllocRespDTO> qdt = qb
                .createQueryDefinition(GCAllocRespDTO.class);
        HopsQuery<GCAllocRespDTO> query = session.createQuery(qdt);
        List<GCAllocRespDTO> queryResultList = query.getResultList();

        List<GarbageCollectorAllocResp> resultSet = createHopsGCAllocResp(queryResultList);
        session.release(queryResultList);

        return resultSet;
    }

    @Override
    public void removeAll(Collection<GarbageCollectorAllocResp> toRemove) throws StorageException {
        if (toRemove.isEmpty()) {
            return;
        }

        HopsSession session = connector.obtainSession();
        List<GCAllocRespDTO> removable =
                new ArrayList<GCAllocRespDTO>(toRemove.size());
        for (GarbageCollectorAllocResp resp : toRemove) {
            removable.add(createPersistable(resp, session));
        }

        session.deletePersistentAll(removable);
        session.release(removable);
    }

    private List<GarbageCollectorAllocResp> createHopsGCAllocResp(List<GCAllocRespDTO> dtos) {
        List<GarbageCollectorAllocResp> resultSet =
                new ArrayList<GarbageCollectorAllocResp>(dtos.size());

        for (GCAllocRespDTO dto : dtos) {
            resultSet.add(new GarbageCollectorAllocResp(dto.getApplicationAttemptId(),
                    dto.getResponseId(), GarbageCollectorAllocResp.TYPE.valueOf(dto.getType())));
        }

        return resultSet;
    }

    private GCAllocRespDTO createPersistable(GarbageCollectorAllocResp resp, HopsSession session)
            throws StorageException {
        GCAllocRespDTO dto = session.newInstance(GCAllocRespDTO.class);
        dto.setApplicationAttemptId(resp.getApplicationAttemptID());
        dto.setResponseId(resp.getResponseID());
        dto.setType(resp.getType().toString());

        return dto;
    }
}
