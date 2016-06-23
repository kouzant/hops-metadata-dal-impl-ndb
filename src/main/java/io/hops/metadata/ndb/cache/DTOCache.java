package io.hops.metadata.ndb.cache;

import java.util.List;

/**
 * Created by antonis on 6/1/16.
 */
public interface DTOCache {
    void registerType(Class type, int cacheSize, int maxCacheSize, int step);
    void deregisterType(Class type);
    <T> boolean put(Class<T> type, T element);
    <T> T get(Class<T> type);
    List<Class> getNotFullTypes();
    <T> boolean containsType(Class<T> type);
    <T> void increaseCacheCapacity(Class<T> type);
}
