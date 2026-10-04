#ifndef BAMTRACKDATAMANAGER_H
#define BAMTRACKDATAMANAGER_H

#include "BamTrackData.h"

/* enusres that one file_path only has one BamTrackData instance
 * this class is a necessity at the moment, since BamTrackData is shared b/w coverage and alignment track
 *
 * TODO: In the future it might be better to have a generic TrackData
 * and ensure one file -> one TrackData for all tracks, this would change to TrackDataManager
 * store format: (file_path -> QWeakPtr<TrackData>)
 * even though it stores only weak ptr, it's necessary to switch to LRUCache for a lower memory hog
 */
class CPPVISUALSHARED_EXPORT BamTrackDataManager
{
public:
	// if file_path in cache, returns cached BamTrackData
	static QSharedPointer<BamTrackData> getOrCreate(QString file_path);
	// if BamTrackData associated with file_path still alive, triggers its reload
	static void reload(QString file_path);
};

#endif // BAMTRACKDATAMANAGER_H
