package io.hops.metadata.ndb.dalimpl.yarn;

import io.hops.metadata.yarn.dal.NextHeartbeatDataAccess;
import io.hops.metadata.yarn.dal.util.YARNOperationType;
import io.hops.metadata.yarn.entity.NextHeartbeat;
import io.hops.transaction.handler.LightWeightRequestHandler;
import org.junit.Ignore;
import org.junit.Test;

import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by antonis on 5/19/16.
 */
public class BenchDTOCreation extends NDBBaseTest {

    private final int RUNS = 0;
    private final int MIN_HB = 100;
    private final int MAX_HB = 10000;
    private final int HB_STEP = 50;

    private final List<NextHeartbeat> toPersist =
            new ArrayList<NextHeartbeat>();

    @Test
    public void createNxtHB() throws Exception {
        storageFactory.getConnector().initDTOCache();
        FileWriter writer;

        for (int i = 0; i < RUNS; ++i) {
            writer = new FileWriter("dto_bench/nxtHB_blah_run" + i, true);
            writer.write("NumOfDTOs,DTO creation,Commit\n");
            System.out.println("RUN: " + i);
            for (int j = MIN_HB; j <= MAX_HB; j += HB_STEP) {

                for (int h = 0; h < j; ++h) {
                    toPersist.add(createNxtHB("rmnode_" + (j + h), true, 0));
                }

                NxtHBAdd adder = new NxtHBAdd(toPersist);

                long commitStart = System.currentTimeMillis();
                Long dtoCreationTime = (Long) adder.handle();
                long commitTime = System.currentTimeMillis() - commitStart;

                writer.write(j + "," + dtoCreationTime + "," + commitTime + "\n");
                //System.out.println("DTO creation time: " + dtoCreationTime);
                //System.out.println("Commit time: " + commitTime);
                toPersist.clear();
                writer.flush();
            }
            writer.close();
            connector.formatStorage(NextHeartbeatDataAccess.class);
        }
    }

    private NextHeartbeat createNxtHB(String rmNodeId, boolean flag, int pendingId) {
        return new NextHeartbeat(rmNodeId, flag, pendingId);
    }


    private class NxtHBAdd extends LightWeightRequestHandler {

        private final List<NextHeartbeat> toPersist;

        public NxtHBAdd(List<NextHeartbeat> toPersist) {
            super(YARNOperationType.TEST);
            this.toPersist = toPersist;
        }

        @Override
        public Object performTask() throws IOException {
            connector.beginTransaction();
            connector.writeLock();

            NextHeartbeatDataAccess nxtHBDAO = (NextHeartbeatDataAccess) storageFactory
                    .getDataAccess(NextHeartbeatDataAccess.class);

            //Long dtoCreationTime = nxtHBDAO.updateAll(toPersist);
            nxtHBDAO.updateAll(toPersist);
            Long dtoCreationTime = -1L;

            connector.commit();
            return dtoCreationTime;
        }
    }
}
