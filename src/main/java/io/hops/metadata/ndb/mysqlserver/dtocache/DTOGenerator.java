package io.hops.metadata.ndb.mysqlserver.dtocache;

import io.hops.exception.StorageException;
import io.hops.metadata.ndb.ClusterjConnector;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * Created by antonis on 5/23/16.
 */
public class DTOGenerator implements Runnable {

    private final Log LOG = LogFactory.getLog(DTOGenerator.class);

    private final ClusterjConnector connector;

    public DTOGenerator() {
        connector = ClusterjConnector.getInstance();
    }

    @Override
    public void run() {
        while (!Thread.currentThread().isInterrupted()) {
            try {
                for (Class type : DTOCache.getNotFullTypes()) {
                    DTOCache.put(generateDTO(type), type);
                }
            } catch (StorageException ex) {
                LOG.error("Error while generating DTO", ex);
            }
        }
    }

    private <T> T generateDTO(Class<T> type) throws StorageException {
        return connector.obtainSession().newInstance(type);
    }
}
