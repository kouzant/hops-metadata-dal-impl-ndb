/*
 * Copyright 2016 Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package io.hops.metadata.ndb.dalimpl.yarn.rmstatestore;

import com.mysql.clusterj.annotation.Column;
import com.mysql.clusterj.annotation.PersistenceCapable;
import com.mysql.clusterj.annotation.PrimaryKey;
import io.hops.exception.StorageException;
import io.hops.metadata.ndb.ClusterjConnector;
import io.hops.metadata.ndb.wrapper.*;
import io.hops.metadata.yarn.TablesDef;
import io.hops.metadata.yarn.dal.rmstatestore.GarbageCollectorRPCDataAccess;
import io.hops.metadata.yarn.entity.appmasterrpc.GarbageCollectorRPC;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

public class GarbageCollectorRPCClusterJ implements TablesDef.GarbageCollectorRPC,
        GarbageCollectorRPCDataAccess<GarbageCollectorRPC> {

    @PersistenceCapable(table = TABLE_NAME)
    public interface GarbageCollectorDTO {

        @PrimaryKey
        @Column(name = RPC_ID)
        int getrpcid();
        void setrpcid(int rpcid);

        @Column(name = TYPE)
        String gettype();
        void settype(String type);
    }

    private final ClusterjConnector connector = ClusterjConnector.getInstance();

    @Override
    public void add(GarbageCollectorRPC garbageCollectorRPC) throws StorageException {
        HopsSession session = connector.obtainSession();
        GarbageCollectorDTO dto = createPersistable(garbageCollectorRPC, session);
        session.savePersistent(dto);
        session.release(dto);
    }

    @Override
    public void addAll(Collection<GarbageCollectorRPC> toPersist) throws StorageException {
        if (toPersist.isEmpty()) {
            return;
        }
        HopsSession session = connector.obtainSession();
        List<GarbageCollectorDTO> persistable =
                new ArrayList<GarbageCollectorDTO>(toPersist.size());
        for (GarbageCollectorRPC rpc : toPersist) {
            persistable.add(createPersistable(rpc, session));
        }

        session.savePersistentAll(persistable);
        session.release(persistable);
    }

    @Override
    public void removeAll(Collection<GarbageCollectorRPC> toRemove) throws StorageException {
        if (toRemove.isEmpty()) {
            return;
        }

        HopsSession session = connector.obtainSession();
        List<GarbageCollectorDTO> removable =
                new ArrayList<GarbageCollectorDTO>(toRemove.size());
        for (GarbageCollectorRPC rpc : toRemove) {
            removable.add(createPersistable(rpc, session));
        }

        session.deletePersistentAll(removable);
        session.release(removable);
    }

    @Override
    public List<GarbageCollectorRPC> getSubset(int limit) throws StorageException {
        if (limit < 1) {
            return null;
        }

        HopsSession session = connector.obtainSession();
        HopsQueryBuilder qb = session.getQueryBuilder();
        HopsQueryDomainType<GarbageCollectorDTO> qdt = qb
                .createQueryDefinition(GarbageCollectorDTO.class);
        HopsQuery<GarbageCollectorDTO> query = session.createQuery(qdt);
        query.setLimits(0, limit);
        List<GarbageCollectorDTO> queryResultList = query.getResultList();

        List<GarbageCollectorRPC> resultlist = createHopsGCRPC(queryResultList);
        session.release(queryResultList);

        return resultlist;
    }

    @Override
    // Mostly for testing, use the getSubset instead
    public List<GarbageCollectorRPC> getAll() throws StorageException {
        HopsSession session = connector.obtainSession();
        HopsQueryBuilder qb = session.getQueryBuilder();
        HopsQueryDomainType<GarbageCollectorDTO> qdt = qb
                .createQueryDefinition(GarbageCollectorDTO.class);
        HopsQuery<GarbageCollectorDTO> query = session.createQuery(qdt);
        List<GarbageCollectorDTO> queryResultList = query.getResultList();

        List<GarbageCollectorRPC> resultList = createHopsGCRPC(queryResultList);
        session.release(queryResultList);

        return resultList;
    }

    private GarbageCollectorDTO createPersistable(GarbageCollectorRPC rpc, HopsSession session)
        throws StorageException {
        GarbageCollectorDTO dto = session.newInstance(GarbageCollectorDTO.class);
        dto.setrpcid(rpc.getRpcid());
        dto.settype(rpc.getType().toString());

        return dto;
    }

    private List<GarbageCollectorRPC> createHopsGCRPC(List<GarbageCollectorDTO> dtos) {
        List<GarbageCollectorRPC> resultSet =
                new ArrayList<GarbageCollectorRPC>(dtos.size());

        for (GarbageCollectorDTO dto : dtos) {
            resultSet.add(new GarbageCollectorRPC(dto.getrpcid(),
                    GarbageCollectorRPC.TYPE.valueOf(dto.gettype())));
        }

        return resultSet;
    }
}
