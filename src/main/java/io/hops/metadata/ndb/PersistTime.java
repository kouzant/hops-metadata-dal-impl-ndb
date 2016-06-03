package io.hops.metadata.ndb;

import java.io.FileWriter;
import java.io.IOException;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Just for profiling
 *
 * Created by antonis on 6/2/16.
 */
public class PersistTime {
    private static PersistTime instance = null;
    private final String home = "/home/antonis/";
    private FileWriter pendingEventWriter;
    private FileWriter updatedContainerWriter;
    private FileWriter nodeHBWriter;
    private FileWriter nextHBWriter;
    private FileWriter totalPersistWriter;
    private final ReentrantLock peLock = new ReentrantLock(true);
    private final ReentrantLock ucLock = new ReentrantLock(true);
    private final ReentrantLock nhLock = new ReentrantLock(true);
    private final ReentrantLock nxLock = new ReentrantLock(true);
    private final ReentrantLock totalLock = new ReentrantLock(true);

    private PersistTime() {

    }

    public synchronized static PersistTime getInstance() {
        if (instance == null) {
            instance = new PersistTime();
        }

        return instance;
    }

    public synchronized void init() {
        try {
            //pendingEventWriter = new FileWriter(home + "pendingEvents", true);
            //updatedContainerWriter = new FileWriter(home + "updatedContainers", true);
            //nodeHBWriter = new FileWriter(home + "nodeHBResponse", true);
            nextHBWriter = new FileWriter(home + "nextHeartbeat", true);
            //totalPersistWriter = new FileWriter(home + "totalPersistTime", true);
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    public synchronized void close() {
        try {
            if (pendingEventWriter != null) {
                pendingEventWriter.flush();
                pendingEventWriter.close();
            }

            if (updatedContainerWriter != null) {
                updatedContainerWriter.flush();
                updatedContainerWriter.close();
            }

            if (nodeHBWriter != null) {
                nodeHBWriter.flush();
                nodeHBWriter.close();
            }

            if (totalPersistWriter != null) {
                totalPersistWriter.flush();
                totalPersistWriter.close();
            }
        } catch (IOException ex) {
            ex.printStackTrace();
        }
    }

    public void writePendingEventTime(long time) {
        try {
            peLock.lock();
            pendingEventWriter.write(time + ",");
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            peLock.unlock();
        }
    }

    public void writeUpdatedContainerTime(long time) {
        try {
            ucLock.lock();
            updatedContainerWriter.write(time + ",");
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            ucLock.unlock();
        }
    }

    public void writeNodeHBTime(long time) {
        try {
            nhLock.lock();
            nodeHBWriter.write(time + ",");
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            nhLock.unlock();
        }
    }

    public void writeNextHeartbeatTime(long time) {
        try {
            nxLock.lock();
            nextHBWriter.write(time + ",");
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            nxLock.unlock();
        }
    }

    public void writeTotalPersistTime(long time) {
        try {
            totalLock.lock();
            totalPersistWriter.write(time + ",");
        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            totalLock.unlock();
        }
    }
}
