package io.hops.metadata.ndb.cache;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.concurrent.ArrayBlockingQueue;

/**
 * Created by antonis on 6/1/16.
 */
public class CacheEntry<T> {
    private final Log LOG = LogFactory.getLog(CacheEntry.class);

    private ArrayBlockingQueue<T> cache;
    private int cacheSize;
    private final int maxCacheSize;
    private final int step;
    private int cacheMiss = 0;

    public CacheEntry(int cacheSize, int maxCacheSize, int step) {
        cache = new ArrayBlockingQueue<T>(cacheSize, true);
        this.cacheSize = cacheSize;
        this.maxCacheSize = maxCacheSize;
        this.step = step;
    }

    public boolean add(T element) {
        return cache.offer(element);
    }

    public T get() {
        return cache.poll();
    }

    public boolean isFull() {
        return cache.size() == cacheSize;
    }

    public boolean isEmpty() {
        return cache.isEmpty();
    }

    public int getCacheSize() {
        return cache.size();
    }

    public void increaseCacheCapacity() {
        if (cacheSize + step <= maxCacheSize) {
            if (++cacheMiss > 1000) {
                cacheMiss = 0;
                cacheSize += step;
                cache = new ArrayBlockingQueue<T>(cacheSize, true);
            }
        }
    }

    public int getCacheTotalCapacity() {
        return cache.size() + cache.remainingCapacity();
    }
}
