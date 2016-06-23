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
package io.hops.metadata.ndb;

import com.mysql.clusterj.ClusterJException;
import com.mysql.clusterj.ClusterJHelper;
import com.mysql.clusterj.Constants;
import io.hops.exception.StorageException;
import io.hops.metadata.ndb.dalimpl.yarn.*;
import io.hops.metadata.ndb.wrapper.HopsExceptionHelper;
import io.hops.metadata.ndb.wrapper.HopsSession;
import io.hops.metadata.ndb.wrapper.HopsSessionFactory;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.*;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

public class DBSessionProvider implements Runnable {

  static final Log LOG = LogFactory.getLog(DBSessionProvider.class);
  static HopsSessionFactory sessionFactory;
  private ConcurrentLinkedQueue<DBSession> nonCachedSessionPool =
          new ConcurrentLinkedQueue<DBSession>();
  private ConcurrentLinkedQueue<DBSession> readySessionPool =
      new ConcurrentLinkedQueue<DBSession>();
  private final ConcurrentLinkedQueue<DBSession> preparingSessionPool =
          new ConcurrentLinkedQueue<DBSession>();
  private ConcurrentLinkedQueue<DBSession> toGC =
      new ConcurrentLinkedQueue<DBSession>();
  private final int MAX_REUSE_COUNT;
  private Properties conf;
  private final Random rand;
  private AtomicInteger sessionsCreated = new AtomicInteger(0);
  private long rollingAvg[];
  private AtomicInteger rollingAvgIndex = new AtomicInteger(-1);
  private boolean automaticRefresh = false;
  private Thread thread;
  private DTOCacheGenerator cacheGenerator;
  private Thread dtoCacheGeneratorThread;

  public DBSessionProvider(Properties conf, int reuseCount, int initialPoolSize)
      throws StorageException {
    this.conf = conf;
    if (reuseCount <= 0) {
      System.err.println("Invalid value for session reuse count");
      System.exit(-1);
    }
    this.MAX_REUSE_COUNT = reuseCount;
    rand = new Random(System.currentTimeMillis());
    rollingAvg = new long[initialPoolSize];
    start(initialPoolSize);
  }

  public void initDTOCache() throws StorageException {
    // TODO: Get number of cached sessions from configuration file
    int NUM_OF_CACHE_ENABLED_SESSIONS = 40;

    for (int i = 0; i < (NUM_OF_CACHE_ENABLED_SESSIONS / 2); i++) {
      readySessionPool.add(initSession(true));
    }

    for (int i = 0; i < (NUM_OF_CACHE_ENABLED_SESSIONS / 2); i++) {
      preparingSessionPool.add(initSession(true));
    }

    if (NUM_OF_CACHE_ENABLED_SESSIONS > 0) {
      cacheGenerator = new DTOCacheGenerator(this, 4, 10);
      dtoCacheGeneratorThread = new Thread(cacheGenerator);
      dtoCacheGeneratorThread.setDaemon(true);
      dtoCacheGeneratorThread.setName("DTO Cache Generator");
      dtoCacheGeneratorThread.start();
    }
  }

  private void start(int initialPoolSize) throws StorageException {
    System.out.println("Database connect string: " +
        conf.get(Constants.PROPERTY_CLUSTER_CONNECTSTRING));
    System.out.println(
        "Database name: " + conf.get(Constants.PROPERTY_CLUSTER_DATABASE));
    System.out.println("Max Transactions: " +
        conf.get(Constants.PROPERTY_CLUSTER_MAX_TRANSACTIONS));
    try {
      sessionFactory =
          new HopsSessionFactory(ClusterJHelper.getSessionFactory(conf));
    } catch (ClusterJException ex) {
      throw HopsExceptionHelper.wrap(ex);
    }

    for (int i = 0; i < initialPoolSize; ++i) {
      nonCachedSessionPool.add(initSession(false));
    }

    thread = new Thread(this, "Session Pool Refresh Daemon");
    thread.setDaemon(true);
    automaticRefresh = true;
    thread.start();
  }

  private DBSession initSession(boolean cacheEnabled) throws StorageException {
    Long startTime = System.currentTimeMillis();
    HopsSession session = sessionFactory.getSession();

    if (cacheEnabled) {
      session.createDTOCache();
      // TODO: Is this a good place to register DTOs to cache?
      // TODO: Parse initial size, max size and step from a configuration file
      session.registerType(PendingEventClusterJ.PendingEventDTO.class, 20000, 30000, 400);
      session.registerType(UpdatedContainerInfoClusterJ.UpdatedContainerInfoDTO.class, 2000, 7000, 200);
      session.registerType(NodeHBResponseClusterJ.NodeHBResponseDTO.class, 2000, 7000, 200);
    }
    Long sessionCreationTime = (System.currentTimeMillis() - startTime);
    rollingAvg[rollingAvgIndex.incrementAndGet() % rollingAvg.length] =
        sessionCreationTime;

    int reuseCount = rand.nextInt(MAX_REUSE_COUNT) + 1;
    DBSession dbSession = new DBSession(session, reuseCount);
    sessionsCreated.incrementAndGet();
    return dbSession;
  }

  private void closeSession(DBSession dbSession) throws StorageException {
    Long startTime = System.currentTimeMillis();
    dbSession.getSession().close();
    Long sessionCreationTime = (System.currentTimeMillis() - startTime);
    rollingAvg[rollingAvgIndex.incrementAndGet() % rollingAvg.length] =
        sessionCreationTime;
  }

  public void stop() throws StorageException {
    automaticRefresh = false;
    if (dtoCacheGeneratorThread != null) {
      dtoCacheGeneratorThread.interrupt();
    }

    drainSessionPool(nonCachedSessionPool);
    drainSessionPool(readySessionPool);
    drainSessionPool(preparingSessionPool);
  }

  private void drainSessionPool(ConcurrentLinkedQueue<DBSession> pool) throws StorageException {
    DBSession session = null;
    while ((session = pool.poll()) != null) {
      closeSession(session);
    }
  }

  public DBSession getCachedSession() throws StorageException {
    try {
      DBSession session = readySessionPool.remove();
      return session;
    } catch (NoSuchElementException e) {
      try {
        DBSession session = preparingSessionPool.remove();
        LOG.warn("maregka Using NOT READY session");
        return session;
      } catch (NoSuchElementException e0) {
        LOG.warn("maregka There are no ready, nor preparing sessions, creating a new one");
        return initSession(true);
      }
    }
  }

  public DBSession getSession() throws StorageException {
    try {
      return nonCachedSessionPool.remove();
    } catch (NoSuchElementException ex) {
      LOG.warn("No available cache-disabled session, creating new one");
      return initSession(false);
    }
  }

  public void returnSession(DBSession returnedSession, boolean forceClose, boolean isCacheEnabled) {
    //session has been used, increment the use counter
    returnedSession
        .setSessionUseCount(returnedSession.getSessionUseCount() + 1);

    if ((returnedSession.getSessionUseCount() >=
        returnedSession.getMaxReuseCount()) ||
        forceClose) {
      // session can be closed even before the reuse count has expired. Close the session incase of database errors.
      toGC.add(returnedSession);
    } else {
      // increment the count and return it to the pool
      // Put it in the preparing pool so that it gets its cache filled
      if (isCacheEnabled) {
        preparingSessionPool.add(returnedSession);
        if (cacheGenerator != null) {
          cacheGenerator.releaseWaitSemaphore();
        }
      } else {
        nonCachedSessionPool.add(returnedSession);
      }
    }
  }

  public double getSessionCreationRollingAvg() {
    double avg = 0;
    for (int i = 0; i < rollingAvg.length; i++) {
      avg += rollingAvg[i];
    }
    avg = avg / rollingAvg.length;
    return avg;
  }

  public int getTotalSessionsCreated() {
    return sessionsCreated.get();
  }

  public int getAvailableReadySessions() {
    return readySessionPool.size();
  }

  public List<DBSession> getAllPreparingSessions() {
    return getPreparingSessions(preparingSessionPool.size());
  }

  public List<DBSession> getPreparingSessions(int limit) {
    List<DBSession> returnSet = new ArrayList<DBSession>();

    DBSession session;
    while ((session = preparingSessionPool.poll()) != null
            && limit > 0) {
      returnSet.add(session);
      limit--;
    }

    return returnSet;
  }

  public void addToReadySessionPool(DBSession session) {
    readySessionPool.add(session);
  }

  @Override
  public void run() {
    while (automaticRefresh) {
      try {
        int toGCSize = toGC.size();
        int cacheDisabledSessions = 0;
        int cacheEnabledSessions = 0;

        if (toGCSize > 0) {
          LOG.debug("Renewing a session(s) " + toGCSize);
          for (int i = 0; i < toGCSize; i++) {
            DBSession session = toGC.remove();
            if (session.getSession().isCachedEnabled()) {
              cacheEnabledSessions++;
            } else {
              cacheDisabledSessions++;
            }
            session.getSession().close();
          }
          //System.out.println("CGed " + toGCSize);

          for (int i = 0; i < cacheDisabledSessions; i++) {
            nonCachedSessionPool.add(initSession(false));
          }

          for (int i = 0; i < cacheEnabledSessions; i++) {
            preparingSessionPool.add(initSession(true));
          }
          //System.out.println("Created " + toGCSize);
        }
        //                for (int i = 0; i < 100; i++) {
        //                    DBSession session = sessionPool.remove();
        //                    double percent = (((double) session.getSessionUseCount() / (double) session.getMaxReuseCount()) * (double) 100);
        //                    // System.out.print(session.getSessionUseCount()+","+session.getMaxReuseCount()+","+percent+" ");
        //                    if (percent > 80) { // more than 80% used then recyle it
        //                        session.getSession().close();
        //                        System.out.println("Recycled a session");
        //                        //add a new session
        //                        sessionPool.add(initSession());
        //                    } else {
        //                        sessionPool.add(session);
        //                    }
        //                }
        Thread.sleep(5);
      } catch (NoSuchElementException e) {
        //System.out.print(".");
        // TODO: Will we ever follow this path?
        for (int i = 0; i < 100; i++) {
          try {
            nonCachedSessionPool.add(initSession(false));
          } catch (StorageException e1) {
            LOG.error(e1);
          }
        }
      } catch (InterruptedException ex) {
        LOG.warn(ex);
      } catch (StorageException e) {
        LOG.error(e);
      }
    }
  }
}
