package io.hops.metadata.ndb.mysqlserver.dtocache;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Created by antonis on 5/23/16.
 */
public class DTOCache {

    private static final Log LOG = LogFactory.getLog(DTOCache.class);

    private static final Map<Class, CacheEntry> cache =
            new HashMap<Class, CacheEntry>();
    private static final ReentrantLock lock =
            new ReentrantLock(true);

    public DTOCache() {
    }

    // Add DTO type to cache
    public static void registerType(Class type, int cacheLimit) {
        try {
            lock.lock();
            if (!cache.containsKey(type)) {
                cache.put(type, new CacheEntry(cacheLimit));
                LOG.info("Added type: " + type.getName() + " to cache");
            } else {
                LOG.info("Type: " + type.getName() + " already exists in cache");
            }
        } finally {
            lock.unlock();
        }
    }

    // Remove a DTO type from cache
    public static void deregisterType(Class type) {
        try {
            lock.lock();
            if (cache.remove(type) == null) {
                LOG.info("Type: " + type.getName() + " did NOT exist in cache");
            } else {
                LOG.info("Type: " + type.getName() + " removed from cache");
            }
        } finally {
            lock.unlock();
        }
    }

    // For testing
    public static void printCache() {
        try {
            lock.lock();
            for (Map.Entry<Class, CacheEntry> entry : cache.entrySet()) {
                LOG.info("Type: " + entry.getKey().getName() + " has cache size: "
                        + entry.getValue().getCacheSize());
            }
        } finally {
            lock.unlock();
        }
    }

    public static Set<Class> getNotFullTypes() {
        Set<Class> notFullTypes =
                new HashSet<Class>();
        try {
            lock.lock();
            for (Map.Entry<Class, CacheEntry> entry : cache.entrySet()) {
                if (!entry.getValue().isFull()) {
                    notFullTypes.add(entry.getKey());
                }
            }

            return notFullTypes;
        } finally {
            lock.unlock();
        }
    }

    public static <T> void put(T element, Class<T> type) {
        CacheEntry cacheEntry;
        try {
            lock.lock();
            cacheEntry = cache.get(type);
            if (cacheEntry == null) {
                LOG.error("Cache does not exist for type: " + type.getName());
                return;
            }
        } finally {
            lock.unlock();
        }

        try {
            cacheEntry.getLock().lock();
            while (cacheEntry.isFull()) {
                try {
                    cacheEntry.getNotFull().await();
                } catch (InterruptedException ex) {
                    LOG.error(ex, ex);
                }
            }

            cacheEntry.add(element);
            cacheEntry.getNotEmpty().signalAll();
        } finally {
            cacheEntry.getLock().unlock();
        }
    }

    // Get element from cache
    // If cache is empty for that type, return null
    public static <T> T getNonBlock(Class<T> type) {
        CacheEntry<T> cacheEntry;
        try {
            lock.lock();
            cacheEntry = cache.get(type);

            if (cacheEntry == null
                    || cacheEntry.isEmpty()) {
                return null;
            }
        } finally {
            lock.unlock();
        }

        try {
            cacheEntry.getLock().lock();
            T element = cacheEntry.get();
            cacheEntry.getNotFull().signalAll();
            return element;
        } finally {
            cacheEntry.getLock().unlock();
        }
    }

    // Get element from cache
    // If cache is empty for that type, wait until producer generate one
    public static <T> T get(Class<T> type) {
        CacheEntry<T> cacheEntry;
        try {
            lock.lock();
            cacheEntry = cache.get(type);
            if (cacheEntry == null) {
                LOG.error("Cache does not exist for type: " + type.getName());
                return null;
            }
        } finally {
            lock.unlock();
        }

        try {
            cacheEntry.getLock().lock();
            while (cacheEntry.isEmpty()) {
                try {
                    cacheEntry.getNotEmpty().await();
                } catch (InterruptedException ex) {
                    LOG.error(ex, ex);
                }
            }

            T element = cacheEntry.get();
            cacheEntry.getNotFull().signalAll();
            return element;
        } finally {
            cacheEntry.getLock().unlock();
        }
    }
}
