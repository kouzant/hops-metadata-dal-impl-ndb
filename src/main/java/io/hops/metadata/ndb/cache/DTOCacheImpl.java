package io.hops.metadata.ndb.cache;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Created by antonis on 6/1/16.
 */
public class DTOCacheImpl implements DTOCache {
    private final Log LOG = LogFactory.getLog(DTOCacheImpl.class);

    private final Map<Class, CacheEntry> cacheMap;

    public DTOCacheImpl() {
        cacheMap = new ConcurrentHashMap<Class, CacheEntry>();
    }

    public void registerType(Class type, int cacheSize) {
        if (!cacheMap.containsKey(type)) {
            cacheMap.put(type, new CacheEntry(cacheSize));
        } else {
            LOG.warn("Cache already contains type: " + type.getName());
        }
    }

    public void deregisterType(Class type) {
        cacheMap.remove(type);
    }

    public <T> boolean put(Class<T> type, T element) {
        CacheEntry cacheEntry = cacheMap.get(type);

        if (cacheEntry == null) {
            LOG.error("Type: " + type.getName() + " is not registered with the cache");
            return false;
        }

        return cacheEntry.add(element);
    }

    public <T> T get(Class<T> type) {
        CacheEntry<T> cacheEntry = cacheMap.get(type);

        if (cacheEntry == null) {
            LOG.error("Type: " + type.getName() + " is not registered with the cache");
            return null;
        }

        return cacheEntry.get();
    }

    public <T> boolean containsType(Class<T> type) {
        return cacheMap.containsKey(type);
    }

    public List<Class> getNotFullTypes() {
        List<Class> notFullTypes =
                new ArrayList<Class>();
        for (Map.Entry<Class, CacheEntry> entry : cacheMap.entrySet()) {
            if (!entry.getValue().isFull()) {
                notFullTypes.add(entry.getKey());
            }
        }

        return notFullTypes;
    }
}
