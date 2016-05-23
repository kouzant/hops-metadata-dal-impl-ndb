package io.hops.metadata.ndb.mysqlserver.dtocache;

import java.util.*;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Created by antonis on 5/23/16.
 */
public class CacheEntry<T> {
    private final int cacheLimit;
    private final Queue<T> cache =
            new LinkedList<T>();
    private final ReentrantLock lock =
            new ReentrantLock(true);
    private final Condition notFull =
            lock.newCondition();
    private final Condition notEmpty =
            lock.newCondition();

    public CacheEntry(int cacheLimit) {
        this.cacheLimit = cacheLimit;
    }

    public void add(T element) {
        cache.add(element);
    }

    public T get() {
        return cache.poll();
    }

    public ReentrantLock getLock() {
        return lock;
    }

    public Condition getNotFull() {
        return notFull;
    }

    public Condition getNotEmpty() {
        return notEmpty;
    }

    public boolean isFull() {
        return cache.size() == cacheLimit;
    }

    public boolean isEmpty() {
        return cache.isEmpty();
    }

    public int getCacheSize() {
        return cache.size();
    }
}
