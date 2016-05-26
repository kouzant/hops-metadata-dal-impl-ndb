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
import io.hops.metadata.ndb.dalimpl.yarn.NextHeartbeatClusterJ;
import io.hops.metadata.ndb.dalimpl.yarn.rmstatestore.AllocateRPCClusterJ;
import io.hops.metadata.ndb.dalimpl.yarn.rmstatestore.GarbageCollectorRPCClusterJ;
import io.hops.metadata.ndb.dalimpl.yarn.rmstatestore.HeartBeatRPCClusterJ;
import io.hops.metadata.ndb.dalimpl.yarn.rmstatestore.RPCClusterJ;
import io.hops.metadata.ndb.wrapper.HopsExceptionHelper;
import io.hops.metadata.ndb.wrapper.HopsSession;
import io.hops.metadata.ndb.wrapper.HopsSessionFactory;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.NoSuchElementException;
import java.util.Properties;
import java.util.Random;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

public class DBSessionProvider implements Runnable {

  static final Log LOG = LogFactory.getLog(DBSessionProvider.class);
  static HopsSessionFactory sessionFactory;
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
  private Thread dtoCacheGenerator;

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

    for (int i = 0; i < initialPoolSize; i++) {
      preparingSessionPool.add(initSession());
    }

    dtoCacheGenerator = new Thread(new DTOCacheGenerator(this));
    dtoCacheGenerator.setDaemon(true);
    dtoCacheGenerator.setName("DTO Cache Generator");
    dtoCacheGenerator.start();

    thread = new Thread(this, "Session Pool Refresh Daemon");
    thread.setDaemon(true);
    automaticRefresh = true;
    thread.start();
  }

  private DBSession initSession() throws StorageException {
    Long startTime = System.currentTimeMillis();
    HopsSession session = sessionFactory.getSession();

    session.createDTOCache();
    // TODO: Is this a good place to register DTOs to cache?
    session.registerType(NextHeartbeatClusterJ.NextHeartbeatDTO.class, 6000);
    session.registerType(RPCClusterJ.RPCDTO.class, 100);
    session.registerType(HeartBeatRPCClusterJ.HeartBeatRPCDTO.class, 100);
    session.registerType(AllocateRPCClusterJ.AllocateRPCDTO.class, 100);
    session.registerType(GarbageCollectorRPCClusterJ.GarbageCollectorDTO.class, 100);

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
    if (dtoCacheGenerator != null) {
      dtoCacheGenerator.interrupt();
    }

    while (!readySessionPool.isEmpty()) {
      DBSession dbsession = readySessionPool.remove();
      closeSession(dbsession);
    }
    while(!preparingSessionPool.isEmpty()) {
      DBSession dbSession = preparingSessionPool.remove();
      closeSession(dbSession);
    }
  }

  public DBSession getSession() throws StorageException {
    try {
      DBSession session = readySessionPool.remove();
      LOG.info("Using session: " + session.toString());
      return session;
    } catch (NoSuchElementException e) {
      LOG.info("There are no ready, nor preparing sessions, creating a new one");
      return initSession();
    }
  }

  public void returnSession(DBSession returnedSession, boolean forceClose) {
    //session has been used, increment the use counter
    returnedSession
        .setSessionUseCount(returnedSession.getSessionUseCount() + 1);

    if ((returnedSession.getSessionUseCount() >=
        returnedSession.getMaxReuseCount()) ||
        forceClose) { // session can be closed even before the reuse count has expired. Close the session incase of database errors.
      toGC.add(returnedSession);
    } else { // increment the count and return it to the pool
      // Put it in the preparing pool so that it gets its cache filled
      preparingSessionPool.add(returnedSession);
      LOG.info("Adding to preparing set returned session: " + returnedSession.toString());
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

  public ConcurrentLinkedQueue<DBSession> getPreparingSessionPool() {
    return preparingSessionPool;
  }

  public ConcurrentLinkedQueue<DBSession> getReadySessionPool() {
    return readySessionPool;
  }

  @Override
  public void run() {
    while (automaticRefresh) {
      try {
        int toGCSize = toGC.size();

        if (toGCSize > 0) {
          LOG.debug("Renewing a session(s) " + toGCSize);
          for (int i = 0; i < toGCSize; i++) {
            DBSession session = toGC.remove();
            session.getSession().close();
          }
          //System.out.println("CGed " + toGCSize);

          for (int i = 0; i < toGCSize; i++) {
            preparingSessionPool.add(initSession());
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
        for (int i = 0; i < 100; i++) {
          try {
            readySessionPool.add(initSession());
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
