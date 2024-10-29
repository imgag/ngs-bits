#include "IGVInitCache.h"

IGVInitCache::IGVInitCache()
    : location_storage_()
{
}

IGVInitCache& IGVInitCache::instance()
{
    static IGVInitCache igv_init_cache;
    return igv_init_cache;
}

void IGVInitCache::add(FileLocation location, bool checked)
{
    instance().location_storage_.append(IGVInitWindowItem(location, checked));
}

void IGVInitCache::clear()
{
    instance().location_storage_.clear();
}

int IGVInitCache::size()
{
    return instance().location_storage_.size();
}

IGVInitWindowItem IGVInitCache::get(int i)
{
    return instance().location_storage_[i];
}
