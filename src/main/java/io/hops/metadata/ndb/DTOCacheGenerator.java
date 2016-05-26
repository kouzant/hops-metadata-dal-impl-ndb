package io.hops.metadata.ndb;

import io.hops.exception.StorageException;
import io.hops.metadata.ndb.wrapper.HopsSession;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.List;

/**
 * Created by antonis on 5/26/16.
 */
public class DTOCacheGenerator implements Runnable {

    private final Log LOG = LogFactory.getLog(DTOCacheGenerator.class);

    private final DBSessionProvider sessionProvider;

    public DTOCacheGenerator(DBSessionProvider sessionProvider) {
        this.sessionProvider = sessionProvider;
        LOG.info("Just spawned DTO generator daemon");
    }

    @Override
    public void run() {
        while (!Thread.currentThread().isInterrupted()) {
            for (DBSession session : sessionProvider.getPreparingSessionPool()) {
                // Remove them from preparing session pool
                //LOG.info("Removing session: " + session.toString() + " from preparing pool");
                sessionProvider.getPreparingSessionPool().remove(session);

                // While session.cache is not full, populate
                List<Class> notFullTypes = session.getSession().getNotFullTypes();

                //LOG.info("Session: " + session.toString());
                for (Class type : notFullTypes) {
                    boolean notFull = true;
                    try {
                        while (notFull) {
                            //LOG.info("Generated DTO");
                            notFull = session.getSession().putToCache(type, generateDTO(session.getSession(), type));
                        }
                        //LOG.info("Session cache is full");
                    } catch (StorageException ex) {
                        LOG.error(ex, ex);
                    }
                }

                // Put them into ready set
                //LOG.info("Adding session: " + session.toString() + " to ready pool");
                sessionProvider.getReadySessionPool().add(session);
            }
        }
    }

    private <T> T generateDTO(HopsSession session, Class<T> type) throws StorageException {
        return session.cacheNewInstance(type);
    }
}
