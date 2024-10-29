#ifndef IGVCACHELIST_H
#define IGVCACHELIST_H

#include <QList>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>

// This class implements a thread safe list to store inforamtion needed to initialize IGV:
// FileLocation objects are stored in advance to speed up an initial call to IGV

template <typename T>
class IGVCacheList : public QList<T>
{
    public:
        IGVCacheList() = default;

        // Adds an item to the list with write lock
        void append(const T& value)
        {
            QWriteLocker locker(&lock);
            QList<T>::append(value);
        }

        // Provides thread-safe access to elements with operator[]
        T operator[](int index) const
        {
            QReadLocker locker(&lock);
            if (index >= 0 && index < QList<T>::size()) {
                return QList<T>::operator[](index);
            }
            throw std::out_of_range("Index out of range");
        }

        // Provides thread-safe access to elements with at()
        T at(int index) const
        {
            return (*this)[index];  // Reuse operator[] implementation
        }

        // Returns the size of the list with read lock
        int size() const
        {
            QReadLocker locker(&lock);
            return QList<T>::size();
        }

        // Custom thread-safe iterator class
        class const_iterator
        {
            public:
                const_iterator(const IGVCacheList* list, int index = 0)
                    : tsList(list), currentIndex(index)
                {
                }

                // Dereference operator
                T operator*() const
                {
                    return tsList->operator[](currentIndex);
                }

                // Pre-increment operator
                const_iterator& operator++()
                {
                    ++currentIndex;
                    return *this;
                }

                // Post-increment operator
                const_iterator operator++(int)
                {
                    const_iterator temp = *this;
                    ++(*this);
                    return temp;
                }

                // Equality comparison operator
                bool operator==(const const_iterator& other) const
                {
                    return currentIndex == other.currentIndex;
                }

                // Inequality comparison operator
                bool operator!=(const const_iterator& other) const
                {
                    return currentIndex != other.currentIndex;
                }

            private:
                const IGVCacheList* tsList;
                int currentIndex;
        };

        // Iterator begin() and end() functions
        const_iterator begin() const
        {
            return const_iterator(this, 0);
        }
        const_iterator end() const
        {
            return const_iterator(this, size());
        }

    private:
        mutable QReadWriteLock lock;
};

#endif // IGVCACHELIST_H
