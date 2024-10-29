#ifndef IGVINITCACHE_H
#define IGVINITCACHE_H

#include "IGVCacheList.h"
#include "FileLocationList.h"

// Data structure that contains information about FileLocation and if this particular file has
// been selected in a checkbox inside the IGV dialog window that pops up in GSvar
struct IGVInitWindowItem
{
    FileLocation location;
    bool checked;

    IGVInitWindowItem()
        : location()
        , checked(true)
    {
    }

    IGVInitWindowItem(const FileLocation& location_, bool checked_)
        : location(location_)
        , checked(checked_)
    {
    }
};

// Implements a simple cache for FileLocation objects. Information about files essential to initialize IGV is
// loaded in advance while a user opens a sample. It speeds up an initial IGV call, since there is no need
// to wait for the server response, the client app already has this information
class IGVInitCache
{
public:
    static void add(FileLocation location, bool checked);
    static void clear();
    static int size();
    static IGVInitWindowItem get(int i);

protected:
    IGVInitCache();

private:
    static IGVInitCache& instance();
    IGVCacheList<IGVInitWindowItem> location_storage_;
};

#endif // IGVINITCACHE_H
