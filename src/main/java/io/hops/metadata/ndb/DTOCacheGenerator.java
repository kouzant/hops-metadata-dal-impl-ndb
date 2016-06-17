package io.hops.metadata.ndb;

import io.hops.exception.StorageException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.concurrent.*;

/**
 * Created by antonis on 5/26/16.
 */
public class DTOCacheGenerator implements Runnable {

    private final Log LOG = LogFactory.getLog(DTOCacheGenerator.class);

    private final int SESSIONS_TO_PREPARE = 20;
    private final int sessionsThreshold;
    private final DBSessionProvider sessionProvider;
    private final ExecutorService exec;
    private final List<Future<?>> workers;
    private final Semaphore semaphore;
    private final Semaphore waitForSessions;
    private boolean warmup = true;

    public DTOCacheGenerator(DBSessionProvider sessionProvider) {
        this(sessionProvider, 2, 200);
    }

    public DTOCacheGenerator(DBSessionProvider sessionProvider, int maxNumberOfThreads,
            int sessionsThreshold) {
        this.sessionProvider = sessionProvider;
        this.exec = Executors.newCachedThreadPool();
        this.sessionsThreshold = sessionsThreshold;
        semaphore = new Semaphore(maxNumberOfThreads, true);
        waitForSessions = new Semaphore(0, true);
        this.workers = new ArrayList<Future<?>>(maxNumberOfThreads);
        LOG.info("Just spawned DTO generator daemon");
    }

    @Override
    public void run() {
        try {
            while (!Thread.currentThread().isInterrupted()) {
                List<DBSession> toBePreparedSessions;

                if (warmup) {
                    toBePreparedSessions = sessionProvider.getAllPreparingSessions();
                    warmup = false;
                } else {
                    toBePreparedSessions = sessionProvider.getPreparingSessions(SESSIONS_TO_PREPARE);
                }

                int preparingSizeNow = toBePreparedSessions.size();

                // Split the toBePreparedSessions, fork and join
                if (preparingSizeNow <= sessionsThreshold) {
                    populatePreparingSessions(toBePreparedSessions);
                } else {
                    // Split them
                    int numOfThreads = (int) Math.floor(preparingSizeNow / sessionsThreshold);
                    // TODO: A little bit nasty, but it's easy to sublist
                    List<DBSession> sessionsList = new ArrayList<DBSession>(toBePreparedSessions);

                    int head = 0;
                    for (int i = 0; i < numOfThreads; ++i) {
                        List<DBSession> subList = sessionsList.subList(head, head + sessionsThreshold);
                        semaphore.acquire();
                        //LOG.info("Submitted sessions [" + head + "," + (head + sessionsThreshold) + "]");
                        workers.add(exec.submit(
                                new Generator(subList, semaphore)));
                        head += sessionsThreshold;
                    }

                    if (head < toBePreparedSessions.size()) {
                        List<DBSession> subList = sessionsList.subList(head, preparingSizeNow);
                        semaphore.acquire();
                        //LOG.info("Submitted sessions [" + head + "," + preparingSizeNow + "]");
                        workers.add(exec.submit(
                                new Generator(subList, semaphore)));
                    }

                    for (Future worker : workers) {
                        worker.get();
                    }

                    workers.clear();
                }

                waitForSessions.acquire(SESSIONS_TO_PREPARE);
            }

        } catch (InterruptedException ex) {
            exec.shutdownNow();
        } catch (ExecutionException ex) {
            LOG.error(ex, ex);
        }
    }

    public void releaseWaitSemaphore() {
        waitForSessions.release();
    }

    private void populatePreparingSessions(Collection<DBSession> sessions) {
        for (DBSession session : sessions) {
            // Preparing DBSessions are removed from the preparing pool when they are fetched
            try {
                List<Class> notFullTypes = session.getSession().getNotFullTypes();

                for (Class type : notFullTypes) {
                    boolean notFullYet = true;

                    try {
                        while (notFullYet) {
                            notFullYet = session.getSession().putToCache(type,
                                    session.getSession().newInstance(type));
                        }
                    } catch (StorageException ex) {
                        LOG.error(ex, ex);
                    }
                }

                sessionProvider.addToReadySessionPool(session);
            } catch (StorageException ex) {
                LOG.error(ex, ex);
            }
        }
    }

    private class Generator implements Runnable {
        private final Collection<DBSession> sessions;
        private final Semaphore semaphore;

        public Generator(Collection<DBSession> sessions, Semaphore semaphore) {
            this.sessions = sessions;
            this.semaphore = semaphore;
        }

        @Override
        public void run() {
            populatePreparingSessions(sessions);
            semaphore.release();
        }
    }
}
