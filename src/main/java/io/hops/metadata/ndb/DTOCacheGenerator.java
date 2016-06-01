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

    private final int sessionsThreshold;
    private final DBSessionProvider sessionProvider;
    private final ExecutorService exec;
    private final List<Future<?>> workers;
    private final Semaphore semaphore;

    public DTOCacheGenerator(DBSessionProvider sessionProvider) {
        this(sessionProvider, 2, 200);
    }

    public DTOCacheGenerator(DBSessionProvider sessionProvider, int maxNumberOfThreads,
            int sessionsThreshold) {
        this.sessionProvider = sessionProvider;
        this.exec = Executors.newCachedThreadPool();
        this.sessionsThreshold = sessionsThreshold;
        semaphore = new Semaphore(maxNumberOfThreads);
        this.workers = new ArrayList<Future<?>>(maxNumberOfThreads);
        LOG.info("Just spawned DTO generator daemon");
    }

    @Override
    public void run() {
        try {
            while (!Thread.currentThread().isInterrupted()) {
                List<DBSession> toBePreparedSessions =
                        sessionProvider.getPreparingSessions(600);

                int preparingSizeNow = toBePreparedSessions.size();
                //LOG.info("toBePreparedSessions is: " + preparingSizeNow);

                if (preparingSizeNow == 0) {
                    TimeUnit.MILLISECONDS.sleep(20);
                    //LOG.info("Preparing sessions is empty, skip...");
                    continue;
                }

                // TODO: Split the toBePreparedSessions, fork and join
                if (preparingSizeNow <= sessionsThreshold) {
                    //LOG.info("Creating one generator thread");
                    semaphore.acquire();
                    workers.add(exec.submit(
                            new Generator(toBePreparedSessions, semaphore)));
                } else {
                    // Split them
                    int numOfThreads = (int) Math.floor(preparingSizeNow / sessionsThreshold);
                    //LOG.info("Creating at least " + numOfThreads + " threads");
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
                        //LOG.info("Last thread");
                        //LOG.info("Submitted sessions [" + head + "," + preparingSizeNow + "]");
                        workers.add(exec.submit(
                                new Generator(subList, semaphore)));
                    }
                }

                //LOG.info("Waiting for workers to finish! " + workers.size());
                for (Future worker : workers) {
                    worker.get();
                }

                workers.clear();
                //LOG.info("All workers are done!");

                // Take some time to rest
                TimeUnit.SECONDS.sleep(1);
            }

        } catch (InterruptedException ex) {
            exec.shutdownNow();
        } catch (ExecutionException ex) {
            LOG.error(ex, ex);
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
            long start = System.currentTimeMillis();
            for (DBSession session : sessions) {
                // I remove the DBSessions from the preparing pool when I get them

                List<Class> notFullTypes = session.getSession().getNotFullTypes();

                for (Class type : notFullTypes) {
                    boolean notFull = true;
                    try {
                        // While cache is not full, populate it
                        while (notFull) {
                            notFull = session.getSession().putToCache(type,
                                    session.getSession().cacheNewInstance(type));
                        }
                    } catch (StorageException ex) {
                        LOG.error(ex, ex);
                    }
                }

                // Put them into ready set
                sessionProvider.getReadySessionPool().add(session);
            }
            semaphore.release();
            //LOG.info("Time to populate " + sessions.size() + ": " + (System.currentTimeMillis() - start));
            //LOG.info("Released semaphore");
        }
    }
}
