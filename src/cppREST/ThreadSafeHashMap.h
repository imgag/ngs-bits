#ifndef THREADSAFEHASHMAP_H
#define THREADSAFEHASHMAP_H

#include <QHash>
#include <QReadWriteLock>

template<typename Key, typename Value>
class ThreadSafeHashMap
{
public:
    ThreadSafeHashMap() = default;
    ~ThreadSafeHashMap() = default;

    void insert(const Key &key, const Value &value)
    {
        QWriteLocker locker(&lock);
        hashMap.insert(key, value);
    }

    bool remove(const Key &key)
    {
        QWriteLocker locker(&lock);
        return hashMap.remove(key) > 0;
    }

    bool contains(const Key &key) const
    {
        QReadLocker locker(&lock);
        return hashMap.contains(key);
    }

    Value value(const Key &key, const Value &defaultValue = Value()) const
    {
        QReadLocker locker(&lock);
        return hashMap.value(key, defaultValue);
    }

    QList<Key> keys() const
    {
        QReadLocker locker(&lock);
        return hashMap.keys();
    }

    void clear()
    {
        QWriteLocker locker(&lock);
        hashMap.clear();
    }

    int size() const
    {
        QReadLocker locker(&lock);
        return hashMap.size();
    }

private:
    mutable QReadWriteLock lock;
    QHash<Key, Value> hashMap;
};

#endif // THREADSAFEHASHMAP_H
