package io.hops.metadata.ndb.cache;

import java.util.concurrent.ArrayBlockingQueue;

/**
 * Created by antonis on 6/1/16.
 */
public class CacheEntry<T> {
    private final ArrayBlockingQueue<T> cache;

    public CacheEntry(int cacheSize) {
        cache = new ArrayBlockingQueue<T>(cacheSize, true);
    }

    public boolean add(T element) {
        return cache.offer(element);
    }

    public T get() {
        return cache.poll();
    }

    public boolean isFull() {
        return cache.remainingCapacity() == 0;
    }

    public boolean isEmpty() {
        return cache.isEmpty();
    }

    public int getCacheSize() {
        return cache.size();
    }
}
